//
//  DrawSSAO.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawSSAO.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include "Math/Math.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

 

Pipeline	g_ssaoPipeline;
Descriptors	g_ssaoDescriptors;

Model g_ssaoFullScreenModel;

struct aoParms_t {
	float aoeRadius;
	int maxSamples;
	float time;
	float pad;
};

/*
====================================================
InitSSAO
====================================================
*/
bool InitSSAO( DeviceContext * device, int width, int height ) {
	bool result;

	FillFullScreenQuad( g_ssaoFullScreenModel );
	g_ssaoFullScreenModel.MakeVBO( device );

	//
	//	ssao calculation pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 3;
		result = g_ssaoDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_ssaoDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/SSAO" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_MULTIPLY;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		pipelineParms.pushConstantSize = sizeof( Vec4 ) * 2;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_ssaoPipeline.Create( device, pipelineParms );
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
CleanupSSAO
====================================================
*/
void CleanupSSAO( DeviceContext * device ) {
	g_ssaoPipeline.Cleanup( device );
	g_ssaoDescriptors.Cleanup( device );

	g_ssaoFullScreenModel.Cleanup();
}

/*
====================================================
DrawSSAO
====================================================
*/
void DrawSSAO( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	aoParms_t aoParms;
	aoParms.aoeRadius = 0.5f;
	aoParms.maxSamples = 5;
	aoParms.time = parms.time;
	aoParms.pad = 0;

	//
	//	Draw the screen space reflections
	//
	{
		g_ssaoPipeline.BindPipeline( cmdBuffer );
		g_ssaoPipeline.BindPushConstant( cmdBuffer, 0, sizeof( aoParms_t ), &aoParms );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_ssaoPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );								// bind the camera matrices

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 1 );
 		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 2 );
 		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 3 );

// 		descriptor.BindImage( g_offscreenFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 4 );
// 		descriptor.BindImage( g_lightProbeImage.m_vkImageLayout, g_lightProbeImage.m_vkImageView, Samplers::m_samplerLightProbe, 5 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_ssaoPipeline );
		g_ssaoFullScreenModel.DrawIndexed( cmdBuffer );
	}
}

