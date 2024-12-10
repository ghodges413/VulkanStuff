//
//  DrawAmbientAO.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawAmbientAO.h"
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

Pipeline		g_ambientAOPipeline;
Descriptors		g_ambientAODescriptors;

Model * g_ambientAODrawFullScreen = NULL;

Image g_ambientAOImageRed;
Image g_ambientAOImageGreen;
Image g_ambientAOImageBlue;

Bounds g_ambientAOBounds = Bounds( Vec3( -100, -100, -100 ), Vec3( 100, 100, 100 ) );

struct aoParms2_t {
	Vec4 boundsMin;
	Vec4 boundsMax;

	float aoeRadius;
	int maxSamples;
	float time;
	float pad;
};

/*
====================================================
InitAmbientAO
====================================================
*/
bool InitAmbientAO( DeviceContext * device, int width, int height ) {
	bool result;

	g_ambientAODrawFullScreen = g_modelManager->AllocateModel();
	FillFullScreenQuad( *g_ambientAODrawFullScreen );
	g_ambientAODrawFullScreen->MakeVBO( device );

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
		g_ambientAOBounds = header->bounds;
		ylms = (legendrePolynomials_t *)( data + sizeof( ambientHeader_t ) );
	}

	Image::CreateParms_t imageParms;
	imageParms.width = numProbesX;
	imageParms.height = numProbesY;
	imageParms.depth = numProbesZ;
	imageParms.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageParms.mipLevels = 1;
	imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	g_ambientAOImageRed.Create( device, imageParms );
	g_ambientAOImageGreen.Create( device, imageParms );
	g_ambientAOImageBlue.Create( device, imageParms );
	

	const int numProbes = numProbesX * numProbesY * numProbesZ;
	Vec4 * texture = (Vec4 *)malloc( sizeof( Vec4 ) * numProbes );

	// Upload the red ylm's to the red texture
	for ( int i = 0; i < numProbes; i++ ) {
		texture[ i ].x = ylms[ i ].mL00.x;
		texture[ i ].y = ylms[ i ].mL11.x;
		texture[ i ].z = ylms[ i ].mL10.x;
		texture[ i ].w = ylms[ i ].mL1n1.x;
	}
	g_ambientAOImageRed.UploadData( device, texture );

	// Upload the green ylm's to the green texture
	for ( int i = 0; i < numProbes; i++ ) {
		texture[ i ].x = ylms[ i ].mL00.y;
		texture[ i ].y = ylms[ i ].mL11.y;
		texture[ i ].z = ylms[ i ].mL10.y;
		texture[ i ].w = ylms[ i ].mL1n1.y;
	}
	g_ambientAOImageGreen.UploadData( device, texture );

	// Upload the blue ylm's to the blue texture
	for ( int i = 0; i < numProbes; i++ ) {
		texture[ i ].x = ylms[ i ].mL00.z;
		texture[ i ].y = ylms[ i ].mL11.z;
		texture[ i ].z = ylms[ i ].mL10.z;
		texture[ i ].w = ylms[ i ].mL1n1.z;
	}
	g_ambientAOImageBlue.UploadData( device, texture );

	free( data );
	free( texture );

	//
	//	Ambient
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 6;	// 3 for ambient colors and 3 for the g-buffer
		result = g_ambientAODescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_ambientAODescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/AmbientAO" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		pipelineParms.pushConstantSize = sizeof( Vec4 ) * 4;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_ambientAOPipeline.Create( device, pipelineParms );
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
CleanupAmbientAO
====================================================
*/
bool CleanupAmbientAO( DeviceContext * device ) {
	g_ambientAOImageRed.Cleanup( device );
	g_ambientAOImageGreen.Cleanup( device );
	g_ambientAOImageBlue.Cleanup( device );

	g_ambientAOPipeline.Cleanup( device );
	g_ambientAODescriptors.Cleanup( device );

	return true;
}

/*
====================================================
DrawAmbientAO
====================================================
*/
void DrawAmbientAO( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	aoParms2_t aoParms;
	aoParms.boundsMin.x = g_ambientAOBounds.mins.x;
	aoParms.boundsMin.y = g_ambientAOBounds.mins.y;
	aoParms.boundsMin.z = g_ambientAOBounds.mins.z;

	aoParms.boundsMax.x = g_ambientAOBounds.maxs.x;
	aoParms.boundsMax.y = g_ambientAOBounds.maxs.y;
	aoParms.boundsMax.z = g_ambientAOBounds.maxs.z;

	aoParms.aoeRadius = 0.5f;
	aoParms.maxSamples = 5;
	aoParms.time = parms.time;
	aoParms.pad = 0;

	//
	//	Draw the ambient
	//
	{
		g_ambientAOPipeline.BindPipeline( cmdBuffer );
		g_ambientAOPipeline.BindPushConstant( cmdBuffer, 0, sizeof( aoParms2_t ), &aoParms );
	
		Descriptor descriptor = g_ambientAOPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );

		descriptor.BindImage( g_ambientAOImageRed, Samplers::m_samplerStandard, 1 );
		descriptor.BindImage( g_ambientAOImageGreen, Samplers::m_samplerStandard, 2 );
		descriptor.BindImage( g_ambientAOImageBlue, Samplers::m_samplerStandard, 3 );

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 4 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 5 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 6 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_ambientAOPipeline );

		g_ambientAODrawFullScreen->DrawIndexed( cmdBuffer );
	}
}

