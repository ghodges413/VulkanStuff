//
//  DrawOffscreen.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawOffscreen.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

#pragma optimize( "", off )

Pipeline	g_depthPrePassPipeline;
Descriptors	g_depthPrePassDescriptors;

/*
====================================================
InitDepthPrePass
====================================================
*/
bool InitDepthPrePass( DeviceContext * device ) {
	bool result = false;

	//
	//	depth pre pass pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		result = g_depthPrePassDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_depthPrePassDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "depthPrePass" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		result = g_depthPrePassPipeline.Create( device, pipelineParms );
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
CleanupDepthPrePass
====================================================
*/
void CleanupDepthPrePass( DeviceContext * device ) {
	g_depthPrePassPipeline.Cleanup( device );
	g_depthPrePassDescriptors.Cleanup( device );
}

/*
====================================================
DrawDepthPrePass
====================================================
*/
void DrawDepthPrePass( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	//
	//	Draw the models
	//
	{
		g_depthPrePassPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_depthPrePassPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			
			descriptor.BindDescriptor( device, cmdBuffer, &g_depthPrePassPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
		}
	}
}






Pipeline		g_ggxPipeline;
Descriptors		g_ggxDescriptors;

static Image g_imageDiffuse;
static Image g_imageNormals;
static Image g_imageGloss;
static Image g_imageSpecular;

Model * g_modelBlock = NULL;

/*
====================================================
InitOffscreen
====================================================
*/
bool InitOffscreen( DeviceContext * device, int width, int height ) {
	bool result;

	//
	//	Build the frame buffer to render into
	//
	{
		FrameBuffer::CreateParms_t frameBufferParms;
		frameBufferParms.width = width;
		frameBufferParms.height = height;
		frameBufferParms.hasColor = true;
		frameBufferParms.hasDepth = true;
		frameBufferParms.HDR = true;
		result = g_offscreenFrameBuffer.Create( device, frameBufferParms );
		if ( !result ) {
			printf( "Unable to create off screen buffer!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	ggx pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		descriptorParms.numSamplerImagesFragment = 4;
		result = g_ggxDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = NULL;//&g_offscreenFrameBuffer;
		pipelineParms.renderPass = g_offscreenFrameBuffer.m_vkRenderPass;
		pipelineParms.descriptors = &g_ggxDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "ggx" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		result = g_ggxPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Texture Images
	//
	{
		Targa targaDiffuse;		
		Targa targaNormals;
		Targa targaGloss;
		Targa targaSpecular;

		targaDiffuse.Load( "../../common/data/images/block_1x1/block_a_color.tga" );
		targaNormals.Load( "../../common/data/images/block_1x1/block_a_normal.tga" );
		targaGloss.Load( "../../common/data/images/block_1x1/block_a_rough.tga" );
		targaSpecular.Load( "../../common/data/images/block_1x1/block_a_metal.tga" );

		Image::CreateParms_t imageParms;
		imageParms.depth = 1;
		imageParms.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageParms.mipLevels = 1;
		imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		imageParms.width = targaDiffuse.GetWidth();
		imageParms.height = targaDiffuse.GetHeight();
		g_imageDiffuse.Create( device, imageParms );
		g_imageDiffuse.UploadData( device, targaDiffuse.DataPtr() );

		imageParms.width = targaNormals.GetWidth();
		imageParms.height = targaNormals.GetHeight();
		g_imageNormals.Create( device, imageParms );
		g_imageNormals.UploadData( device, targaNormals.DataPtr() );

		imageParms.width = targaGloss.GetWidth();
		imageParms.height = targaGloss.GetHeight();
		g_imageGloss.Create( device, imageParms );
		g_imageGloss.UploadData( device, targaGloss.DataPtr() );

		imageParms.width = targaSpecular.GetWidth();
		imageParms.height = targaSpecular.GetHeight();
		g_imageSpecular.Create( device, imageParms );
		g_imageSpecular.UploadData( device, targaSpecular.DataPtr() );

		g_modelBlock = g_modelManager->GetModel( "../../common/data/meshes/static/block_a_lp.obj" );
	}

	return true;
}

/*
====================================================
CleanupOffscreen
====================================================
*/
bool CleanupOffscreen( DeviceContext * device ) {
	g_offscreenFrameBuffer.Cleanup( device );

	g_ggxPipeline.Cleanup( device );
	g_ggxDescriptors.Cleanup( device );

	g_imageDiffuse.Cleanup( device );
	g_imageNormals.Cleanup( device );
	g_imageGloss.Cleanup( device );
	g_imageSpecular.Cleanup( device );

	return true;
}

/*
====================================================
DrawOffscreen
====================================================
*/
void DrawOffscreen( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	//
	//	Draw the models
	//
	{
		g_ggxPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_ggxPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			descriptor.BindImage( g_imageDiffuse, Samplers::m_samplerRepeat, 2 );
			descriptor.BindImage( g_imageNormals, Samplers::m_samplerRepeat, 3 );
			descriptor.BindImage( g_imageGloss, Samplers::m_samplerRepeat, 4 );
			descriptor.BindImage( g_imageSpecular, Samplers::m_samplerRepeat, 5 );
			
			descriptor.BindDescriptor( device, cmdBuffer, &g_ggxPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
		}
	}
}