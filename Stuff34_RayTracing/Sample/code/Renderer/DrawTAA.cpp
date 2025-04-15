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

FrameBuffer g_taaFrameBuffer;
FrameBuffer g_taaVelocityBuffer;

Image g_taaHistoryBuffer;

Pipeline	g_taaVelocityPipeline;
Descriptors	g_taaVelocityDescriptors;

Pipeline	g_taaPipeline;
Descriptors	g_taaDescriptors;

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

	{
		Image::CreateParms_t parms;
		parms.width = width;
		parms.height = height;
		parms.depth = 1;
		parms.mipLevels = 1;
		parms.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		parms.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if ( !g_taaHistoryBuffer.Create( device, parms ) ) {
			printf( "ERROR: Failed to creat TAA history buffer\n" );
			assert( 0 );
			return false;
		}
		g_taaHistoryBuffer.TransitionLayout( device );
	}

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

		{
			result = g_taaFrameBuffer.Create( device, parms );
			if ( !result ) {
				printf( "Unable to make taa framebuffer\n" );
				assert( 0 );
				return false;
			}
		}

		parms.hasDepth = true;
		{
			result = g_taaVelocityBuffer.Create( device, parms );
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
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		result = g_taaVelocityDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_taaVelocityBuffer;
		pipelineParms.descriptors = &g_taaVelocityDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "AntiAliasing/TemporalVelocity" );
		pipelineParms.width = g_taaVelocityBuffer.m_parms.width;
		pipelineParms.height = g_taaVelocityBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_NONE;
		pipelineParms.depthCompare = Pipeline::DEPTH_COMPARE_LEQUAL;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		pipelineParms.pushConstantSize = sizeof( Mat4 ) * 2;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		result = g_taaVelocityPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	taa accumulation pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numSamplerImagesFragment = 3;
		result = g_taaDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_taaFrameBuffer;
		pipelineParms.descriptors = &g_taaDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "AntiAliasing/TemporalAccumulation" );
		pipelineParms.width = g_taaFrameBuffer.m_parms.width;
		pipelineParms.height = g_taaFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_NONE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		pipelineParms.pushConstantSize = sizeof( float ) * 4;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_taaPipeline.Create( device, pipelineParms );
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
	g_taaHistoryBuffer.Cleanup( device );

	g_taaFrameBuffer.Cleanup( device );
	g_taaVelocityBuffer.Cleanup( device );

	g_taaPipeline.Cleanup( device );
 	g_taaDescriptors.Cleanup( device );

	g_taaVelocityPipeline.Cleanup( device );
 	g_taaVelocityDescriptors.Cleanup( device );

	g_taaFullScreenModel.Cleanup();
}

static int g_frameCounter = 0;

Camera_t g_cameraOld;

/*
====================================================
CalculateVelocityBuffer
====================================================
*/
void CalculateVelocityBuffer( DrawParms_t & parms ) {
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

	Vec4 invDims;
	invDims.x = 1.0f / (float)g_taaFrameBuffer.m_parms.width;
	invDims.y = 1.0f / (float)g_taaFrameBuffer.m_parms.height;
	invDims.z = 0;
	invDims.w = 0;

	//
	//	Update descriptors for velocity buffer calculation
	//
	std::vector< Descriptor > velDescriptorList;
	velDescriptorList.reserve( numModels );
	for ( int i = 0; i < numModels; i++ ) {
		const RenderModel & renderModel = renderModels[ i ];
		if ( NULL == renderModel.modelDraw ) {
			continue;
		}

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_taaVelocityPipeline.GetFreeDescriptor();

		descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 0 );		// bind the model matrices
		descriptor.BindBuffer( uniformsOld, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices

		descriptor.UpdateDescriptor( device );
		velDescriptorList.push_back( descriptor );
	}

	//
	//	Calculate velocity buffer
	//
	g_taaVelocityBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_taaVelocityBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	g_taaVelocityBuffer.BeginRenderPass( device, cmdBufferIndex, true, true );

	g_taaVelocityPipeline.BindPipeline( cmdBuffer );
	g_taaVelocityPipeline.BindPushConstant( cmdBuffer, 0, sizeof( Mat4 ) * 2, cameras );

	int idx = 0;
	for ( int i = 0; i < numModels; i++ ) {
		const RenderModel & renderModel = renderModels[ i ];
		if ( NULL == renderModel.modelDraw ) {
			continue;
		}

		velDescriptorList[ idx ].BindDescriptor( cmdBuffer, &g_taaVelocityPipeline );
		renderModel.modelDraw->DrawIndexed( cmdBuffer );
		idx++;
	}
	g_taaVelocityBuffer.EndRenderPass( device, cmdBufferIndex );
	g_taaVelocityBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_taaVelocityBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL );

	g_cameraOld = camera;
	g_frameCounter++;
}

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

// 	if ( 0 == g_frameCounter ) {
// 		g_cameraOld = camera;
// 	}

// 	Mat4 cameras[ 2 ];
// 	cameras[ 0 ] = camera.matProj * camera.matView;
// 	cameras[ 1 ] = g_cameraOld.matProj * g_cameraOld.matView;
// 
// 	cameras[ 0 ] = cameras[ 0 ].Transpose();
// 	cameras[ 1 ] = cameras[ 1 ].Transpose();

	Vec4 invDims;
	invDims.x = 1.0f / (float)g_taaFrameBuffer.m_parms.width;
	invDims.y = 1.0f / (float)g_taaFrameBuffer.m_parms.height;
	invDims.z = 0;
	invDims.w = 0;

	//
	//	Update descriptor for TAA
	//
	Descriptor descriptor = g_taaPipeline.GetFreeDescriptor();
	{
#if defined( ENABLE_RAYTRACING )
		extern Image g_rtxImage;
		extern Image * g_rtxImageOut;
		descriptor.BindImage( g_rtxImage, Samplers::m_samplerStandard, 0 );
		if ( g_rtxImageOut ) {
			descriptor.BindImage( *g_rtxImageOut, Samplers::m_samplerStandard, 0 );
		}
#elif defined( USE_SSR )
		descriptor.BindImage( g_ssrFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
#else
		descriptor.BindImage( g_offscreenFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
#endif
		descriptor.BindImage( g_taaHistoryBuffer, Samplers::m_samplerNearest, 1 );	// History Buffer
		descriptor.BindImage( g_taaVelocityBuffer.m_imageColor, Samplers::m_samplerStandard, 2 );

		descriptor.UpdateDescriptor( device );
	}

	//
	//	Accumulate the TAA
	//
	g_taaFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_taaFrameBuffer.BeginRenderPass( device, cmdBufferIndex, true, true );
	{
		g_taaPipeline.BindPipeline( cmdBuffer );
		g_taaPipeline.BindPushConstant( cmdBuffer, 0, sizeof( float ) * 4, invDims.ToPtr() );

		descriptor.BindDescriptor( cmdBuffer, &g_taaPipeline );
		g_taaFullScreenModel.DrawIndexed( cmdBuffer );
	}
	g_taaFrameBuffer.EndRenderPass( device, cmdBufferIndex );
	
// 	g_cameraOld = camera;
// 	g_frameCounter++;

	g_taaFrameBufferPtr = &g_taaFrameBuffer;
	g_taaVelocityBufferPtr = &g_taaVelocityBuffer;

	//
	//	Copy the taa buffer into the history buffer
	//
	g_taaFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
	g_taaHistoryBuffer.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

	Image::CopyImage( g_taaHistoryBuffer, g_taaFrameBuffer.m_imageColor, cmdBuffer );

 	g_taaFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
 	g_taaHistoryBuffer.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
}