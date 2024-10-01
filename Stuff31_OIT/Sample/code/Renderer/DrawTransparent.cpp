//
//  DrawTransparent.cpp
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


Pipeline	g_transparentPipeline;
Descriptors	g_transparentDescriptors;

/*
====================================================
InitTransparent
====================================================
*/
bool InitTransparent( DeviceContext * device, int width, int height ) {
	bool result;

	//
	//	subsurface pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		result = g_transparentDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_transparentDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Transparent/Stochastic" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthCompare = Pipeline::DEPTH_COMPARE_LEQUAL;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		pipelineParms.pushConstantSize = sizeof( Vec4 ) * 2;
		result = g_transparentPipeline.Create( device, pipelineParms );
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
CleanupTransparent
====================================================
*/
void CleanupTransparent( DeviceContext * device ) {
	g_transparentPipeline.Cleanup( device );
	g_transparentDescriptors.Cleanup( device );
}


/*
====================================================
DrawTransparent
====================================================
*/
void DrawTransparent( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	float pushConstantBuffer[ 8 ];
	pushConstantBuffer[ 4 ] = parms.time;
	pushConstantBuffer[ 5 ] = parms.time;
	pushConstantBuffer[ 6 ] = parms.time;
	pushConstantBuffer[ 7 ] = parms.time;

	//
	//	Draw the screen space reflections
	//
	for ( int i = 0; i < numModels; i++ ) {
		const RenderModel & renderModel = renderModels[ i ];
		if ( !renderModel.isTransparent ) {
			continue;
		}

		pushConstantBuffer[ 0 ] = renderModel.transparentColor.x;
		pushConstantBuffer[ 1 ] = renderModel.transparentColor.y;
		pushConstantBuffer[ 2 ] = renderModel.transparentColor.z;
		pushConstantBuffer[ 3 ] = renderModel.transparentColor.w;

		g_transparentPipeline.BindPipeline( cmdBuffer );
		g_transparentPipeline.BindPushConstant( cmdBuffer, 0, sizeof( Vec4 ) * 2, pushConstantBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_transparentPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );								// bind the camera matrices
		descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_transparentPipeline );
		renderModel.modelDraw->DrawIndexed( cmdBuffer );
	}
}

