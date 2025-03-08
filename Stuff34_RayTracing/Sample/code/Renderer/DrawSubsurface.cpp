//
//  DrawSubsurface.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawSubsurface.h"
#include "Renderer/BuildSubsurface.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include "Math/Math.h"
#include "Miscellaneous/Fileio.h"
#include <assert.h>
#include <stdio.h>
#include <vector>


Pipeline	g_subsurfacePipeline;
Descriptors	g_subsurfaceDescriptors;

Image g_subsurfaceTableImage;

/*
====================================================
InitSubsurface
====================================================
*/
bool InitSubsurface( DeviceContext * device, int width, int height ) {
	bool result;

	BuildSubsurfaceTable();

	// Load the pre-calculated subsurface scattering table to an image
	{
		unsigned char * data = NULL;
		unsigned int size = 0;
		GetFileData( "generated/subsurfaceJade.subsurface", &data, size );

		subsurfaceHeader_t * header = (subsurfaceHeader_t *)data;
		Image::CreateParms_t parms;
		parms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		parms.depth = 1;
		parms.mipLevels = 1;
		parms.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		parms.width = header->numAngles;
		parms.height = header->numRadii;

		
		Vec3 * colors = (Vec3*)( data + sizeof( subsurfaceHeader_t ) );
		Vec4 * tmp = (Vec4 *)malloc( parms.width * parms.height * sizeof( Vec4 ) );
		assert( tmp );

		for ( int i = 0; i < parms.width * parms.height; i++ ) {
			const Vec3 & c = colors[ i ];
			tmp[ i ] = Vec4( c.x, c.y, c.z, 0 );
		}

		g_subsurfaceTableImage.Create( device, parms );
		g_subsurfaceTableImage.UploadData( device, tmp );

		free( tmp );
	}

	//
	//	subsurface pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		descriptorParms.numSamplerImagesFragment = 4;
		result = g_subsurfaceDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_subsurfaceDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/Subsurface" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthCompare = Pipeline::DEPTH_COMPARE_LEQUAL;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = false;
		result = g_subsurfacePipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	return result;
}

/*
====================================================
CleanupSubsurface
====================================================
*/
void CleanupSubsurface( DeviceContext * device ) {
	g_subsurfacePipeline.Cleanup( device );
	g_subsurfaceDescriptors.Cleanup( device );

	g_subsurfaceTableImage.Cleanup( device );
}


/*
====================================================
DrawSubsurface
====================================================
*/
void DrawSubsurface( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	//
	//	Draw the screen space reflections
	//
	for ( int i = 0; i < numModels; i++ ) {
		const RenderModel & renderModel = renderModels[ i ];
		if ( renderModel.subsurfaceId < 0 ) {
			continue;
		}

		g_subsurfacePipeline.BindPipeline( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_subsurfacePipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );								// bind the camera matrices
		descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 2 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 3 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 4 );

 		descriptor.BindImage( g_subsurfaceTableImage, Samplers::m_samplerStandard, 5 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_subsurfacePipeline );
		renderModel.modelDraw->DrawIndexed( cmdBuffer );
	}
}

