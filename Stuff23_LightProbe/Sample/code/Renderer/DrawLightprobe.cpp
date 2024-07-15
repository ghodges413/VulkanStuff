//
//  DrawSkybox.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawLightProbe.h"
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

#include <assert.h>
#include <stdio.h>
#include <vector>

Pipeline		g_lightProbePipeline;
Descriptors		g_lightProbeDescriptors;

Model * g_lightProbeDrawFullScreen = NULL;

ImageCubeMap g_lightProbeImage;

/*
====================================================
InitLightProbe
====================================================
*/
bool InitLightProbe( DeviceContext * device, int width, int height ) {
	bool result;

	g_lightProbeDrawFullScreen = g_modelManager->AllocateModel();
	FillFullScreenQuad( *g_lightProbeDrawFullScreen );
	g_lightProbeDrawFullScreen->MakeVBO( device );

	g_lightProbeImage.Create( device );

	//
	//	LightProbe
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 1;
		descriptorParms.numSamplerImagesFragment = 4;
		result = g_lightProbeDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_lightProbeDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/LightProbe" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = g_lightProbePipeline.Create( device, pipelineParms );
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
CleanupLightProbe
====================================================
*/
bool CleanupLightProbe( DeviceContext * device ) {
	g_lightProbeImage.Cleanup( device );

	g_lightProbePipeline.Cleanup( device );
	g_lightProbeDescriptors.Cleanup( device );

	return true;
}

/*
====================================================
DrawLightProbe
====================================================
*/
void DrawLightProbe( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	//
	//	Draw the light probe
	//
	{
		g_lightProbePipeline.BindPipeline( cmdBuffer );
	
		Descriptor descriptor = g_lightProbePipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );

		descriptor.BindImage( g_lightProbeImage.m_vkImageLayout, g_lightProbeImage.m_vkImageView, Samplers::m_samplerLightProbe, 1 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 2 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 3 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 4 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_lightProbePipeline );

		g_lightProbeDrawFullScreen->DrawIndexed( cmdBuffer );
	}
}

