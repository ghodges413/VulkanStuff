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

#pragma optimize( "", off )

Pipeline	g_depthPrePassGBufferPipeline;
Descriptors	g_depthPrePassGBufferDescriptors;

Pipeline	g_deferredPipeline;
Descriptors	g_deferredDescriptors;

static Image g_imageDiffuse;
static Image g_imageNormals;
static Image g_imageRoughness;
static Image g_imageSpecular;

/*
====================================================
InitGBuffer
====================================================
*/
bool InitGBuffer( DeviceContext * device, int width, int height ) {
	bool result = false;

	//
	//	Build the frame buffer to render into
	//
	{
		GFrameBuffer::CreateParms_t frameBufferParms;
		frameBufferParms.width = width;
		frameBufferParms.height = height;
		result = g_gbuffer.Create( device, frameBufferParms );
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
		result = g_depthPrePassGBufferDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = NULL;//&g_gbuffer;
		pipelineParms.renderPass = g_gbuffer.m_vkRenderPass;
		pipelineParms.descriptors = &g_depthPrePassGBufferDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "depthPrePass" );
		pipelineParms.width = g_gbuffer.m_parms.width;
		pipelineParms.height = g_gbuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		result = g_depthPrePassGBufferPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Fill gbuffer
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		descriptorParms.numSamplerImagesFragment = 4;
		result = g_deferredDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = NULL;//&g_gbuffer;
		pipelineParms.renderPass = g_gbuffer.m_vkRenderPass;
		pipelineParms.descriptors = &g_deferredDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "deferred" );
		//pipelineParms.shader = g_shaderManager->GetShader( "ggx" );
		pipelineParms.width = g_gbuffer.m_parms.width;
		pipelineParms.height = g_gbuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		pipelineParms.numColorAttachments = GFrameBuffer::s_numColorImages;
		result = g_deferredPipeline.Create( device, pipelineParms );
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
		Targa targaRoughness;
		Targa targaSpecular;

		targaDiffuse.Load( "../../common/data/images/block_1x1/block_a_color.tga" );
		targaNormals.Load( "../../common/data/images/block_1x1/block_a_normal.tga" );
		targaRoughness.Load( "../../common/data/images/block_1x1/block_a_rough.tga" );
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

		imageParms.width = targaRoughness.GetWidth();
		imageParms.height = targaRoughness.GetHeight();
		g_imageRoughness.Create( device, imageParms );
		g_imageRoughness.UploadData( device, targaRoughness.DataPtr() );

		imageParms.width = targaSpecular.GetWidth();
		imageParms.height = targaSpecular.GetHeight();
		g_imageSpecular.Create( device, imageParms );
		g_imageSpecular.UploadData( device, targaSpecular.DataPtr() );
	}

	return result;
}

/*
====================================================
CleanupGBuffer
====================================================
*/
void CleanupGBuffer( DeviceContext * device ) {
	g_gbuffer.Cleanup( device );

	g_depthPrePassGBufferPipeline.Cleanup( device );
	g_depthPrePassGBufferDescriptors.Cleanup( device );

	g_deferredPipeline.Cleanup( device );
	g_deferredDescriptors.Cleanup( device );

	g_imageDiffuse.Cleanup( device );
	g_imageNormals.Cleanup( device );
	g_imageRoughness.Cleanup( device );
	g_imageSpecular.Cleanup( device );
}

/*
====================================================
DrawGBuffer
====================================================
*/
void DrawGBuffer( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	//
	//	Depth pre-pass
	//
	{
		g_depthPrePassGBufferPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_depthPrePassGBufferPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			
			descriptor.BindDescriptor( device, cmdBuffer, &g_depthPrePassGBufferPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
		}
	}

	//
	//	Fill g-buffer
	//
	{
		g_deferredPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_deferredPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			descriptor.BindImage( g_imageDiffuse, Samplers::m_samplerRepeat, 2 );
			descriptor.BindImage( g_imageNormals, Samplers::m_samplerRepeat, 3 );
			descriptor.BindImage( g_imageRoughness, Samplers::m_samplerRepeat, 4 );
			descriptor.BindImage( g_imageSpecular, Samplers::m_samplerRepeat, 5 );
			
			descriptor.BindDescriptor( device, cmdBuffer, &g_deferredPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
		}
	}
}