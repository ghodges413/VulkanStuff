//
//  DrawAmbient.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawAmbient.h"
#include "Renderer/BuildAmbient.h"
#include "Models/ModelShape.h"
#include "Models/ModelManager.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Graphics/Targa.h"
#include "Miscellaneous/Fileio.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

Pipeline		g_ambientPipeline;
Descriptors		g_ambientDescriptors;

Model * g_ambientDrawFullScreen = NULL;

Image g_ambientImageRed;
Image g_ambientImageGreen;
Image g_ambientImageBlue;

Bounds g_ambientBounds = Bounds( Vec3( -100, -100, -100 ), Vec3( 100, 100, 100 ) );

/*
====================================================
InitAmbient
====================================================
*/
bool InitAmbient( DeviceContext * device, int width, int height ) {
	bool result;

	g_ambientDrawFullScreen = g_modelManager->AllocateModel();
	FillFullScreenQuad( *g_ambientDrawFullScreen );
	g_ambientDrawFullScreen->MakeVBO( device );

	legendrePolynomials_t * ylms = NULL;

	int numProbesX = AMBIENT_NODES_X;
	int numProbesY = AMBIENT_NODES_Y;
	int numProbesZ = AMBIENT_NODES_Z;

	unsigned int fileSize = 0;
	unsigned char * data = NULL;
	GetFileData( "generated/ambient.shambient", &data, fileSize );
	ambientHeader_t * header = (ambientHeader_t *)data;

	if ( NULL == data || header->version != AMBIENT_VERSION ) {
		// This is a valid event when building ambient lighting
		numProbesX = 1;
		numProbesY = 1;
		numProbesZ = 1;
		int size = sizeof( legendrePolynomials_t ) * numProbesX * numProbesY * numProbesZ;
		data = (unsigned char *)malloc( size );
		memset( data, 0, size );

		ylms = (legendrePolynomials_t *)data;
	} else {
		numProbesX = header->numX;
		numProbesY = header->numY;
		numProbesZ = header->numZ;
		g_ambientBounds = header->bounds;
		ylms = (legendrePolynomials_t *)( data + sizeof( ambientHeader_t ) );
	}

	Image::CreateParms_t imageParms;
	imageParms.width = numProbesX;
	imageParms.height = numProbesY;
	imageParms.depth = numProbesZ;
	imageParms.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageParms.mipLevels = 1;
	imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	g_ambientImageRed.Create( device, imageParms );
	g_ambientImageGreen.Create( device, imageParms );
	g_ambientImageBlue.Create( device, imageParms );
	

	const int numProbes = numProbesX * numProbesY * numProbesZ;
	Vec4 * texture = (Vec4 *)malloc( sizeof( Vec4 ) * numProbes );

	// Upload the red ylm's to the red texture
	for ( int i = 0; i < numProbes; i++ ) {
		texture[ i ].x = ylms[ i ].mL00.x;
		texture[ i ].y = ylms[ i ].mL11.x;
		texture[ i ].z = ylms[ i ].mL10.x;
		texture[ i ].w = ylms[ i ].mL1n1.x;
	}
	g_ambientImageRed.UploadData( device, texture );

	// Upload the green ylm's to the green texture
	for ( int i = 0; i < numProbes; i++ ) {
		texture[ i ].x = ylms[ i ].mL00.y;
		texture[ i ].y = ylms[ i ].mL11.y;
		texture[ i ].z = ylms[ i ].mL10.y;
		texture[ i ].w = ylms[ i ].mL1n1.y;
	}
	g_ambientImageGreen.UploadData( device, texture );

	// Upload the blue ylm's to the blue texture
	for ( int i = 0; i < numProbes; i++ ) {
		texture[ i ].x = ylms[ i ].mL00.z;
		texture[ i ].y = ylms[ i ].mL11.z;
		texture[ i ].z = ylms[ i ].mL10.z;
		texture[ i ].w = ylms[ i ].mL1n1.z;
	}
	g_ambientImageBlue.UploadData( device, texture );

	free( data );
	free( texture );

	//
	//	Ambient
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		//descriptorParms.numUniformsVertex = 1;
		descriptorParms.numSamplerImagesFragment = 6;	// 3 for ambient colors and 3 for the g-buffer
		result = g_ambientDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_ambientDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/Ambient" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		pipelineParms.pushConstantSize = sizeof( Vec4 ) * 2;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_ambientPipeline.Create( device, pipelineParms );
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
CleanupAmbient
====================================================
*/
bool CleanupAmbient( DeviceContext * device ) {
	g_ambientImageRed.Cleanup( device );
	g_ambientImageGreen.Cleanup( device );
	g_ambientImageBlue.Cleanup( device );

	g_ambientPipeline.Cleanup( device );
	g_ambientDescriptors.Cleanup( device );

	return true;
}

Vec4 g_ambientBounds2[ 2 ];

/*
====================================================
DrawAmbient
====================================================
*/
void DrawAmbient( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	g_ambientBounds2[ 0 ].x = g_ambientBounds.mins.x;
	g_ambientBounds2[ 0 ].y = g_ambientBounds.mins.y;
	g_ambientBounds2[ 0 ].z = g_ambientBounds.mins.z;

	g_ambientBounds2[ 1 ].x = g_ambientBounds.maxs.x;
	g_ambientBounds2[ 1 ].y = g_ambientBounds.maxs.y;
	g_ambientBounds2[ 1 ].z = g_ambientBounds.maxs.z;

	//
	//	Draw the ambient
	//
	{
		g_ambientPipeline.BindPipeline( cmdBuffer );
		g_ambientPipeline.BindPushConstant( cmdBuffer, 0, sizeof( Vec4 ) * 2, g_ambientBounds2 );
	
		Descriptor descriptor = g_ambientPipeline.GetFreeDescriptor();

		descriptor.BindImage( g_ambientImageRed, Samplers::m_samplerStandard, 0 );
		descriptor.BindImage( g_ambientImageGreen, Samplers::m_samplerStandard, 1 );
		descriptor.BindImage( g_ambientImageBlue, Samplers::m_samplerStandard, 2 );

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 3 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 4 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 5 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_ambientPipeline );

		g_ambientDrawFullScreen->DrawIndexed( cmdBuffer );
	}
}

