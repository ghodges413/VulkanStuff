//
//  DrawTAA.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawTAA.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include "Math/Math.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

FrameBuffer * g_taaFrameBufferPtr = NULL;
FrameBuffer * g_taaVelocityBufferPtr = NULL;

FrameBuffer g_taaFrameBuffer[ 2 ];
FrameBuffer g_taaVelocityBuffer[ 2 ];

Pipeline	g_taaVelocityPipeline[ 2 ];
Descriptors	g_taaVelocityDescriptors[ 2 ];

Pipeline	g_taaPipeline[ 2 ];
Descriptors	g_taaDescriptors[ 2 ];

Model g_taaFullScreenModel;

/*
====================================================
InitTemporalAntiAliasing
====================================================
*/
bool InitTemporalAntiAliasing( DeviceContext * device, int width, int height ) {
	bool result = true;

	FillFullScreenQuad( g_taaFullScreenModel );
	g_taaFullScreenModel.MakeVBO( device );

	//
	//	taa framebuffer
	//
	{
		FrameBuffer::CreateParms_t parms;
		parms.hasColor = true;
		parms.hasDepth = false;
		parms.HDR = true;
		parms.width = width;
		parms.height = height;

		for ( int i = 0; i < 2; i++ ) {
			result = g_taaFrameBuffer[ i ].Create( device, parms );
			if ( !result ) {
				printf( "Unable to make taa framebuffer\n" );
				assert( 0 );
				return false;
			}
		}

		parms.hasDepth = true;
		for ( int i = 0; i < 2; i++ ) {
			result = g_taaVelocityBuffer[ i ].Create( device, parms );
			if ( !result ) {
				printf( "Unable to make taa framebuffer\n" );
				assert( 0 );
				return false;
			}
		}
	}

	//
	//	taa velocity pipeline
	//
	for ( int i = 0; i < 2; i++ ) {
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		result = g_taaVelocityDescriptors[ i ].Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_taaVelocityBuffer[ i ];
		pipelineParms.descriptors = &g_taaVelocityDescriptors[ i ];
		pipelineParms.shader = g_shaderManager->GetShader( "AntiAliasing/TemporalVelocity" );
		pipelineParms.width = g_taaVelocityBuffer[ i ].m_parms.width;
		pipelineParms.height = g_taaVelocityBuffer[ i ].m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_NONE;
		pipelineParms.depthCompare = Pipeline::DEPTH_COMPARE_LEQUAL;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		pipelineParms.pushConstantSize = sizeof( Mat4 ) * 2;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_taaVelocityPipeline[ i ].Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	taa accumulation pipeline
	//
	for ( int i = 0; i < 2; i++ ) {
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numSamplerImagesFragment = 5;
		result = g_taaDescriptors[ i ].Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_taaFrameBuffer[ i ];
		pipelineParms.descriptors = &g_taaDescriptors[ i ];
		pipelineParms.shader = g_shaderManager->GetShader( "AntiAliasing/TemporalAccumulation" );
		pipelineParms.width = g_taaFrameBuffer[ i ].m_parms.width;
		pipelineParms.height = g_taaFrameBuffer[ i ].m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_NONE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		pipelineParms.pushConstantSize = sizeof( float ) * 4;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_taaPipeline[ i ].Create( device, pipelineParms );
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
CleanupTemporalAntiAliasing
====================================================
*/
void CleanupTemporalAntiAliasing( DeviceContext * device ) {
	for ( int i = 0; i < 2; i++ ) {
		g_taaFrameBuffer[ i ].Cleanup( device );
		g_taaVelocityBuffer[ i ].Cleanup( device );

		g_taaPipeline[ i ].Cleanup( device );
 		g_taaDescriptors[ i ].Cleanup( device );

		g_taaVelocityPipeline[ i ].Cleanup( device );
 		g_taaVelocityDescriptors[ i ].Cleanup( device );
	}
	g_taaFullScreenModel.Cleanup();
}

static int g_frameCounter = 0;

Camera_t g_cameraOld;

/*
====================================================
DrawTemporalAntiAliasing
====================================================
*/
void DrawTemporalAntiAliasing( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	Buffer * uniformsOld = parms.uniformsOld;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];
	const Camera_t & camera = parms.camera;

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	if ( 0 == g_frameCounter ) {
		g_cameraOld = camera;
	}

	Mat4 cameras[ 2 ];
	cameras[ 0 ] = camera.matProj * camera.matView;
	cameras[ 1 ] = g_cameraOld.matProj * g_cameraOld.matView;

	cameras[ 0 ] = cameras[ 0 ].Transpose();
	cameras[ 1 ] = cameras[ 1 ].Transpose();

	const int idx0 = ( g_frameCounter + 0 ) % 2;
	const int idx1 = ( g_frameCounter + 1 ) % 2;

	Vec4 invDims;
	invDims.x = 1.0f / (float)g_taaFrameBuffer[ 0 ].m_parms.width;
	invDims.y = 1.0f / (float)g_taaFrameBuffer[ 0 ].m_parms.height;
	invDims.z = 0;
	invDims.w = 0;


	//
	//	Calculate velocity buffer
	//
	g_taaVelocityBuffer[ idx0 ].m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_taaVelocityBuffer[ idx0 ].m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	g_taaVelocityBuffer[ idx0 ].BeginRenderPass( device, cmdBufferIndex, true, true );

	g_taaVelocityPipeline[ idx0 ].BindPipeline( cmdBuffer );
	g_taaVelocityPipeline[ idx0 ].BindPushConstant( cmdBuffer, 0, sizeof( Mat4 ) * 2, cameras );
	for ( int i = 0; i < numModels; i++ ) {
		const RenderModel & renderModel = renderModels[ i ];
		if ( NULL == renderModel.modelDraw ) {
			continue;
		}

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_taaVelocityPipeline[ idx0 ].GetFreeDescriptor();

		descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 0 );		// bind the model matrices
		descriptor.BindBuffer( uniformsOld, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices

		descriptor.BindDescriptor( device, cmdBuffer, &g_taaVelocityPipeline[ idx0 ] );
		renderModel.modelDraw->DrawIndexed( cmdBuffer );
	}
	g_taaVelocityBuffer[ idx0 ].EndRenderPass( device, cmdBufferIndex );
	g_taaVelocityBuffer[ idx0 ].m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_taaVelocityBuffer[ idx0 ].m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL );


	//
	//	Accumulate the TAA
	//
	g_taaFrameBuffer[ idx0 ].m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_taaFrameBuffer[ idx0 ].BeginRenderPass( device, cmdBufferIndex, true, true );
	{
		g_taaPipeline[ idx0 ].BindPipeline( cmdBuffer );
		g_taaPipeline[ idx0 ].BindPushConstant( cmdBuffer, 0, sizeof( float ) * 4, invDims.ToPtr() );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_taaPipeline[ idx0 ].GetFreeDescriptor();

#if defined( USE_SSR )
		descriptor.BindImage( g_ssrFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
#else
		descriptor.BindImage( g_offscreenFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
#endif
		descriptor.BindImage( g_taaFrameBuffer[ idx1 ].m_imageColor, Samplers::m_samplerNearest, 1 );	// History Buffer
		descriptor.BindImage( g_taaVelocityBuffer[ idx0 ].m_imageColor, Samplers::m_samplerStandard, 2 );
		descriptor.BindImage( g_taaVelocityBuffer[ idx1 ].m_imageDepth, Samplers::m_samplerStandard, 3 ); // History Depth
		descriptor.BindImage( g_taaVelocityBuffer[ idx0 ].m_imageDepth, Samplers::m_samplerStandard, 4 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_taaPipeline[ idx0 ] );
		g_taaFullScreenModel.DrawIndexed( cmdBuffer );
	}
	g_taaFrameBuffer[ idx0 ].EndRenderPass( device, cmdBufferIndex );
	g_taaFrameBuffer[ idx0 ].m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );

	g_cameraOld = camera;
	g_frameCounter++;

	g_taaFrameBufferPtr = &g_taaFrameBuffer[ idx0 ];
	g_taaVelocityBufferPtr = &g_taaVelocityBuffer[ idx0 ];
}

