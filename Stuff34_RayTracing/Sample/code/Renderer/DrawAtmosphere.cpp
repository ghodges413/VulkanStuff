//
//  DrawAtmosphere.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawAtmosphere.h"
#include "Renderer/BuildAtmosphere.h"
#include "Models/ModelShape.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Graphics/Targa.h"
#include "Miscellaneous/Fileio.h"
#include "Miscellaneous/Input.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

Pipeline		g_atmosPipeline;
Descriptors		g_atmosDescriptors;
Model			g_atmosModel;			// full screen quad for rendering
Buffer			g_atmosUniforms;

FrameBuffer		g_sunShadowFrameBuffer;
Pipeline		g_sunShadowUpdatePipeline;
Descriptors		g_sunShadowUpdateDescriptors;


Image g_transmittanceImage;
Image g_irradianceImage;
Image g_scatterimage;

struct atmosUniforms_t {
	shadowCamera_t shadowCamera;

	Vec4 camera;
	Vec3 sun;
	float ISun;// = 10.0;

	

	Vec4 dimInScatter;// = vec4( 32, 128, 32, 8 );

	float radiusGround;
	float radiusTop;
	float shadowNear;
	float shadowFar;

	// Rayleigh
	Vec3 betaRayleighScatter;
	float scaleHeightRayleigh;
	Vec4 betaRayleighExtinction;

	// Mie
	Vec3 betaMieScatter;
	float scaleHeightMie;
	Vec3 betaMieExtinction;
	float mieG;
};
atmosUniforms_t g_atmosData;

static bool g_sunUpdateShadow = true;

static float g_sunAngle = 3.1415f / 4.0f;

/*
====================================================
InitAtmosphere
====================================================
*/
bool InitAtmosphere( DeviceContext * device, int width, int height ) {
	atmosData_t parms;

	unsigned char * data = NULL;
	unsigned int size = 0;
	if ( !GetFileData( "generated/atmosphere.atmos", &data, size ) ) {
		BuildAtmosphereTables( parms );

		if ( !GetFileData( "generated/atmosphere.atmos", &data, size ) ) {
			return false;
		}
	}

	FillFullScreenQuad( g_atmosModel );
	g_atmosModel.MakeVBO( device );

	atmosHeader_t * header = (atmosHeader_t *)data;
	assert( header->magic == ATMOS_MAGIC );
	assert( header->version == ATMOS_VERSION );
	const unsigned char * transmittanceData = data + sizeof( atmosHeader_t );
	const unsigned char * irradianceData = transmittanceData + header->data.transmittance_width * header->data.transmittance_height * 4 * sizeof( float );
	const unsigned char * scatterData = irradianceData + header->data.irradiance_width * header->data.irradiance_height * 4 * sizeof( float );

	// Fill out the g_atmosData structure
	g_atmosData.camera = Vec4( 0.0f );
	//g_atmosData.sun = Vec3( cosf( g_sunAngle ), 0.0f, sinf( g_sunAngle ) );
	g_atmosData.sun = Vec3( cosf( g_sunAngle ), 1.0f, sinf( g_sunAngle ) );
	g_atmosData.sun.Normalize();
	g_atmosData.ISun = 10;
	g_atmosData.shadowCamera.matProj.Identity();
	g_atmosData.shadowCamera.matView.Identity();
	g_atmosData.dimInScatter = header->data.scatter_dims;
	g_atmosData.radiusGround = header->data.radius_ground;
	g_atmosData.radiusTop = header->data.radius_top;
	g_atmosData.shadowNear = 1;
	g_atmosData.shadowNear = 500;
	g_atmosData.betaRayleighScatter = header->data.beta_rayleigh_scatter;
	g_atmosData.scaleHeightRayleigh = header->data.scale_height_rayleigh;
	g_atmosData.betaRayleighExtinction.x = header->data.beta_rayleigh_extinction.x;
	g_atmosData.betaRayleighExtinction.y = header->data.beta_rayleigh_extinction.y;
	g_atmosData.betaRayleighExtinction.z = header->data.beta_rayleigh_extinction.z;
	g_atmosData.betaRayleighExtinction.w = 0;
	g_atmosData.betaMieScatter = header->data.beta_mie_scatter;
	g_atmosData.scaleHeightMie = header->data.scale_height_mie;
	g_atmosData.betaMieExtinction = header->data.beta_mie_extinction;
	g_atmosData.mieG = header->data.mie_g;

	g_atmosData.camera *= 0.001f; // convert from meters to kilometers
	g_atmosData.camera.z += g_atmosData.radiusGround + 1.0f;

	g_atmosUniforms.Allocate( device, &g_atmosData, sizeof( g_atmosData ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		
	Image::CreateParms_t imageParms;
	imageParms.width = header->data.transmittance_width;
	imageParms.height = header->data.transmittance_height;
	imageParms.depth = 1;
	imageParms.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageParms.mipLevels = 1;
	imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	g_transmittanceImage.Create( device, imageParms );
	g_transmittanceImage.UploadData( device, transmittanceData );

	imageParms.width = header->data.irradiance_width;
	imageParms.height = header->data.irradiance_height;
	imageParms.depth = 1;
	imageParms.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageParms.mipLevels = 1;
	imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	g_irradianceImage.Create( device, imageParms );
	g_irradianceImage.UploadData( device, irradianceData );

	imageParms.width = header->data.scatter_mu_s_size * header->data.scatter_nu_size;
	imageParms.height = header->data.scatter_mu_size;
	imageParms.depth = header->data.scatter_r_size;
	imageParms.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageParms.mipLevels = 1;
	imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	g_scatterimage.Create( device, imageParms );
	g_scatterimage.UploadData( device, scatterData );

	free( data );

	//
	//	Sky
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 1;
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 7;
		bool result = g_atmosDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_atmosDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Skybox/SkyBruneton" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = g_atmosPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Shadow frame buffer
	//
	{
		FrameBuffer::CreateParms_t parms;
		parms.hasColor = false;
		parms.hasDepth = true;
		parms.height = 2048;
		parms.width = 2048;
		g_sunShadowFrameBuffer.Create( device, parms );
	}

	//
	//	Shadow update pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		bool result = g_sunShadowUpdateDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_sunShadowFrameBuffer;
		pipelineParms.descriptors = &g_sunShadowUpdateDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "depthOnly" );
		pipelineParms.width = g_sunShadowFrameBuffer.m_parms.width;
		pipelineParms.height = g_sunShadowFrameBuffer.m_parms.height;
		//pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.cullMode = Pipeline::CULL_MODE_FRONT;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		result = g_sunShadowUpdatePipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	return true;
}

/*
====================================================
CleanupAtmosphere
====================================================
*/
bool CleanupAtmosphere( DeviceContext * device ) {
	g_atmosPipeline.Cleanup( device );
	g_atmosDescriptors.Cleanup( device );
	g_atmosModel.Cleanup();
	g_atmosUniforms.Cleanup( device );

	g_sunShadowFrameBuffer.Cleanup( device );
	g_sunShadowUpdatePipeline.Cleanup( device );
	g_sunShadowUpdateDescriptors.Cleanup( device );
	
	g_transmittanceImage.Cleanup( device );
	g_irradianceImage.Cleanup( device );
	g_scatterimage.Cleanup( device );

	return true;
}

/*
====================================================
UpdateAtmosphere
====================================================
*/
void UpdateAtmosphere( DeviceContext * device, const Camera_t & camera, float dt_sec, Bounds mapBounds ) {
	Vec4 camPos;
	camPos.x = camera.matView.rows[ 0 ].w;
	camPos.y = camera.matView.rows[ 1 ].w;
	camPos.z = camera.matView.rows[ 2 ].w;
	camPos.w = 0;

	g_atmosData.camera = camPos * 0.001f; // convert from meters to kilometers
	g_atmosData.camera.z += g_atmosData.radiusGround + 0.1f;

	float dangle = 2.5f / 3.1415f * dt_sec;
	if ( keyState_t::KeyState_PRESS == g_Input->GetKeyState( keyboard_t::key_u ) ) {
		g_sunAngle += dangle;
		g_sunUpdateShadow = true;
	}
	if ( keyState_t::KeyState_PRESS == g_Input->GetKeyState( keyboard_t::key_o ) ) {
		g_sunAngle -= dangle;
		g_sunUpdateShadow = true;
	}
	if ( keyState_t::KeyState_PRESS == g_Input->GetKeyState( keyboard_t::key_i ) ) {
		g_atmosData.ISun += 0.01f;
		if ( g_atmosData.ISun > 10.0f ) {
			g_atmosData.ISun = 0.0f;
		}
	}
	//g_atmosData.sun = Vec3( cosf( g_sunAngle ), 0.0f, sinf( g_sunAngle ) );
	g_atmosData.sun = Vec3( cosf( g_sunAngle ), 1.0f, sinf( g_sunAngle ) );
	g_atmosData.sun.Normalize();

	// Update the shadow matrices
	float radius = mapBounds.GetRadius();
	Vec3 center = mapBounds.Center();
	Vec3 pos = center + g_atmosData.sun * radius;
	Vec3 up = g_atmosData.sun.Cross( Vec3( 0, 1, 0 ) );
	g_atmosData.shadowNear = 1.0f;
	g_atmosData.shadowFar = 2.0f * radius;
	g_atmosData.shadowCamera.matProj.OrthoVulkan( -radius, radius, -radius, radius, g_atmosData.shadowNear, g_atmosData.shadowFar );
	g_atmosData.shadowCamera.matView.LookAt( pos, center, up );

	g_atmosData.shadowCamera.matProj = g_atmosData.shadowCamera.matProj.Transpose();
	g_atmosData.shadowCamera.matView = g_atmosData.shadowCamera.matView.Transpose();

	atmosUniforms_t * mappedData = (atmosUniforms_t *)g_atmosUniforms.MapBuffer( device );
	memcpy( mappedData, &g_atmosData, sizeof( atmosUniforms_t ) );
	g_atmosUniforms.UnmapBuffer( device );
}

static std::vector< Descriptor > g_descriptorList;

/*
====================================================
UpdateSunShadowDescriptors
====================================================
*/
void UpdateSunShadowDescriptors( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.notCulledRenderModels;
	const int numModels = parms.numNotCulledRenderModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	if ( g_descriptorList.size() == 0 ) {
		g_descriptorList.reserve( numModels );

		int camOffset = 0;
		int camSize = sizeof( shadowCamera_t );

		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_sunShadowUpdatePipeline.GetFreeDescriptor();
			descriptor.BindBuffer( &g_atmosUniforms, camOffset, camSize, 0 );							// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			
			descriptor.UpdateDescriptor( device );
			g_descriptorList.push_back( descriptor );
		}
	}
}

/*
====================================================
UpdateSunShadow
====================================================
*/
void UpdateSunShadow( DrawParms_t & parms ) {
	if ( !g_sunUpdateShadow ) {
		return;
	}

	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.notCulledRenderModels;
	const int numModels = parms.numNotCulledRenderModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	//
	//	Draw the models
	//
	{
		g_sunShadowFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
		g_sunShadowFrameBuffer.BeginRenderPass( device, cmdBufferIndex, true, true );

		g_sunShadowUpdatePipeline.BindPipeline( cmdBuffer );

		int idx = 0;
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			g_descriptorList[ idx ].BindDescriptor( cmdBuffer, &g_sunShadowUpdatePipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
			idx++;
		}

		g_sunShadowFrameBuffer.EndRenderPass( device, cmdBufferIndex );
		g_sunShadowFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL );
	}

	g_sunUpdateShadow = false;
}

/*
====================================================
DrawAtmosphere
====================================================
*/
void DrawAtmosphere( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	const int timeOffset = camOffset + camSize;
	const int timeSize = sizeof( Vec4 );
	
	//
	//	Draw the sky
	//
	{
		g_atmosPipeline.BindPipeline( cmdBuffer );
	
		Descriptor descriptor = g_atmosPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );
		descriptor.BindBuffer( &g_atmosUniforms, 0, sizeof( atmosUniforms_t ), 1 );

		descriptor.BindImage( g_transmittanceImage, Samplers::m_samplerStandard, 2 );
		descriptor.BindImage( g_irradianceImage, Samplers::m_samplerStandard, 3 );
		descriptor.BindImage( g_scatterimage, Samplers::m_samplerStandard, 4 );

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 5 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 6 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 7 );

		descriptor.BindImage( g_sunShadowFrameBuffer.m_imageDepth, Samplers::m_samplerStandard, 8 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_atmosPipeline );

		g_atmosModel.DrawIndexed( cmdBuffer );
	}
}

