//
//  DrawGBuffer.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawGBuffer.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

Pipeline	g_preDepthPipeline;
Descriptors	g_preDepthDescriptors;

FrameBuffer g_preDepthFrameBuffer;


/*
====================================================
InitPreDepth
====================================================
*/
bool InitPreDepth( DeviceContext * device, int width, int height ) {
	bool result = false;

	//
	//	Build the frame buffer to render into
	//
	{
		FrameBuffer::CreateParms_t parms;
		parms.width = width;
		parms.height = height;
		parms.HDR = false;
		parms.hasColor = false;
		parms.hasDepth = true;
		result = g_preDepthFrameBuffer.Create( device, parms );
		if ( !result ) {
			printf( "Unable to create off screen buffer!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	depth pre pass pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		result = g_preDepthDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_preDepthFrameBuffer;
		pipelineParms.renderPass = g_preDepthFrameBuffer.m_vkRenderPass;
		pipelineParms.descriptors = &g_preDepthDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "depthPrePass" );
		pipelineParms.width = g_preDepthFrameBuffer.m_parms.width;
		pipelineParms.height = g_preDepthFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		result = g_preDepthPipeline.Create( device, pipelineParms );
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
CleanupPreDepth
====================================================
*/
void CleanupPreDepth( DeviceContext * device ) {
	g_preDepthPipeline.Cleanup( device );
	g_preDepthDescriptors.Cleanup( device );

	g_preDepthFrameBuffer.Cleanup( device );
}

/*
====================================================
DrawPreDepth
====================================================
*/
void DrawPreDepth( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	//g_preDepthFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_preDepthFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	g_preDepthFrameBuffer.BeginRenderPass( device, cmdBufferIndex, true, true );

	//
	//	Depth pre-pass
	//
	{
		g_preDepthPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_preDepthPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			
			descriptor.BindDescriptor( device, cmdBuffer, &g_preDepthPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
		}
	}

	g_preDepthFrameBuffer.EndRenderPass( device, cmdBufferIndex );
	//g_preDepthFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_preDepthFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL );
}

