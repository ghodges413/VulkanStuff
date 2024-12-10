//
//  DrawSSR.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawSSR.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include "Math/Math.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

 

#define MAX_SHADOWS 4

FrameBuffer g_ssrFrameBuffer;

Pipeline	g_ssrPipeline;
Descriptors	g_ssrDescriptors;

Pipeline	g_ssrBlendPipeline;
Descriptors	g_ssrBlendDescriptors;


Model g_ssrFullScreenModel;

/*
====================================================
InitScreenSpaceReflections
====================================================
*/
bool InitScreenSpaceReflections( DeviceContext * device, int width, int height ) {
	bool result;

	FillFullScreenQuad( g_ssrFullScreenModel );
	g_ssrFullScreenModel.MakeVBO( device );

	//
	//	ssr framebuffer
	//
	{
		FrameBuffer::CreateParms_t parms;
		parms.hasColor = true;
		parms.hasDepth = false;
		parms.width = width;
		parms.height = height;		
		result = g_ssrFrameBuffer.Create( device, parms );
		if ( !result ) {
			printf( "Unable to make ssr framebuffer\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	ssr calculation pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		//descriptorParms.numUniformsVertex = 1;
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 5;
		result = g_ssrDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_ssrFrameBuffer;
		pipelineParms.descriptors = &g_ssrDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/ScreenSpaceReflections" );
		pipelineParms.width = g_ssrFrameBuffer.m_parms.width;
		pipelineParms.height = g_ssrFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = g_ssrPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	ssr blend pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numSamplerImagesFragment = 1;
		result = g_ssrBlendDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_ssrFrameBuffer;
		pipelineParms.descriptors = &g_ssrBlendDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "copy" );
		pipelineParms.width = g_ssrFrameBuffer.m_parms.width;
		pipelineParms.height = g_ssrFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_NONE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = g_ssrBlendPipeline.Create( device, pipelineParms );
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
CleanupScreenSpaceReflections
====================================================
*/
void CleanupScreenSpaceReflections( DeviceContext * device ) {
	g_ssrFrameBuffer.Cleanup( device );

	g_ssrPipeline.Cleanup( device );
	g_ssrDescriptors.Cleanup( device );

	g_ssrBlendPipeline.Cleanup( device );
	g_ssrBlendDescriptors.Cleanup( device );

	g_ssrFullScreenModel.Cleanup();
}


/*
====================================================
BlendColorBuffer
====================================================
*/
void BlendColorBuffer( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	//
	//	Blend the screen space reflections into the offscreen buffer
	//
	g_ssrBlendPipeline.BindPipeline( cmdBuffer );

	// Descriptor is how we bind our buffers and images
	Descriptor descriptor = g_ssrBlendPipeline.GetFreeDescriptor();

	descriptor.BindImage( g_offscreenFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );

	descriptor.BindDescriptor( device, cmdBuffer, &g_ssrBlendPipeline );
	g_ssrFullScreenModel.DrawIndexed( cmdBuffer );
}



/*
====================================================
DrawScreenSpaceReflections
====================================================
*/
void DrawScreenSpaceReflections( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	g_ssrFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_ssrFrameBuffer.BeginRenderPass( device, cmdBufferIndex, true, true );

	// Blend the SSR buffer into the offscreen buffer
	BlendColorBuffer( parms );

	//
	//	Draw the screen space reflections
	//
	{
		g_ssrPipeline.BindPipeline( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_ssrPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );								// bind the camera matrices

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 1 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 2 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 3 );

		descriptor.BindImage( g_offscreenFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 4 );
		descriptor.BindImage( g_lightProbeImage.m_vkImageLayout, g_lightProbeImage.m_vkImageView, Samplers::m_samplerLightProbe, 5 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_ssrPipeline );
		g_ssrFullScreenModel.DrawIndexed( cmdBuffer );
	}

	g_ssrFrameBuffer.EndRenderPass( device, cmdBufferIndex );
	g_ssrFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
}

