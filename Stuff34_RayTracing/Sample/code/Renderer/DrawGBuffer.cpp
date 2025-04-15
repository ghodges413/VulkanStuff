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

 

Pipeline	g_depthPrePassGBufferPipeline;
Descriptors	g_depthPrePassGBufferDescriptors;

Pipeline	g_deferredPipeline;
Descriptors	g_deferredDescriptors;

Pipeline	g_deferredCheckerBoardPipeline;
Descriptors	g_deferredCheckerBoardDescriptors;

static Image g_imageDiffuse;
static Image g_imageNormals;
static Image g_imageRoughness;
static Image g_imageSpecular;


static Image g_imageSkullDiffuse;
static Image g_imageSkullNormals;
static Image g_imageSkullRoughness;
static Image g_imageSkullSpecular;

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
		pipelineParms.shader = g_shaderManager->GetShader( "depthPrePassGBuffer" );
		pipelineParms.width = g_gbuffer.m_parms.width;
		pipelineParms.height = g_gbuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		pipelineParms.numColorAttachments = 3;
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
		descriptorParms.numUniformsVertex = 3;
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
		pipelineParms.shader = g_shaderManager->GetShader( "deferredModelID" );
		//pipelineParms.shader = g_shaderManager->GetShader( "deferred" );
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
	//	Fill gbuffer checkerboard
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 3;
		result = g_deferredCheckerBoardDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = NULL;//&g_gbuffer;
		pipelineParms.renderPass = g_gbuffer.m_vkRenderPass;
		pipelineParms.descriptors = &g_deferredCheckerBoardDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "deferredCheckerboard" );
		pipelineParms.width = g_gbuffer.m_parms.width;
		pipelineParms.height = g_gbuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		pipelineParms.numColorAttachments = GFrameBuffer::s_numColorImages;
		result = g_deferredCheckerBoardPipeline.Create( device, pipelineParms );
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
		targaDiffuse.GenerateMips();
		targaNormals.GenerateMips();
		targaRoughness.GenerateMips();
		targaSpecular.GenerateMips();

		// TODO: Add support for block compression (this should give us a performance boost)

		Image::CreateParms_t imageParms;
		imageParms.depth = 1;
		imageParms.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageParms.mipLevels = 1;
		imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		imageParms.width = targaDiffuse.GetWidth();
		imageParms.height = targaDiffuse.GetHeight();
		imageParms.mipLevels = targaDiffuse.GetMips();
		g_imageDiffuse.Create( device, imageParms );
		g_imageDiffuse.UploadData( device, targaDiffuse.DataPtr() );

		imageParms.width = targaNormals.GetWidth();
		imageParms.height = targaNormals.GetHeight();
		imageParms.mipLevels = targaNormals.GetMips();
		g_imageNormals.Create( device, imageParms );
		g_imageNormals.UploadData( device, targaNormals.DataPtr() );

		imageParms.width = targaRoughness.GetWidth();
		imageParms.height = targaRoughness.GetHeight();
		imageParms.mipLevels = targaRoughness.GetMips();
		g_imageRoughness.Create( device, imageParms );
		g_imageRoughness.UploadData( device, targaRoughness.DataPtr() );

		imageParms.width = targaSpecular.GetWidth();
		imageParms.height = targaSpecular.GetHeight();
		imageParms.mipLevels = targaSpecular.GetMips();
		g_imageSpecular.Create( device, imageParms );
		g_imageSpecular.UploadData( device, targaSpecular.DataPtr() );
	}
	{
		Targa targaDiffuse;		
		Targa targaNormals;
		Targa targaRoughness;
		Targa targaSpecular;
#if 1
		targaDiffuse.Load( "../../common/data/images/skull/skull_a_color.tga" );
		targaNormals.Load( "../../common/data/images/skull/skull_a_normal.tga" );
		targaRoughness.Load( "../../common/data/images/skull/skull_a_rough.tga" );
		targaSpecular.Load( "../../common/data/images/skull/skull_a_metal.tga" );
#else
		targaDiffuse.Load( "../../common/data/images/skull_wax/skull_a_wax_color.tga" );
		targaNormals.Load( "../../common/data/images/skull_wax/skull_a_wax_normal.tga" );
		targaRoughness.Load( "../../common/data/images/skull_wax/skull_a_wax_rough.tga" );
		targaSpecular.Load( "../../common/data/images/skull_wax/skull_a_wax_metal.tga" );
#endif
		Image::CreateParms_t imageParms;
		imageParms.depth = 1;
		imageParms.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageParms.mipLevels = 1;
		imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		imageParms.width = targaDiffuse.GetWidth();
		imageParms.height = targaDiffuse.GetHeight();
		g_imageSkullDiffuse.Create( device, imageParms );
		g_imageSkullDiffuse.UploadData( device, targaDiffuse.DataPtr() );

		imageParms.width = targaNormals.GetWidth();
		imageParms.height = targaNormals.GetHeight();
		g_imageSkullNormals.Create( device, imageParms );
		g_imageSkullNormals.UploadData( device, targaNormals.DataPtr() );

		imageParms.width = targaRoughness.GetWidth();
		imageParms.height = targaRoughness.GetHeight();
		g_imageSkullRoughness.Create( device, imageParms );
		g_imageSkullRoughness.UploadData( device, targaRoughness.DataPtr() );

		imageParms.width = targaSpecular.GetWidth();
		imageParms.height = targaSpecular.GetHeight();
		g_imageSkullSpecular.Create( device, imageParms );
		g_imageSkullSpecular.UploadData( device, targaSpecular.DataPtr() );
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

	g_deferredCheckerBoardPipeline.Cleanup( device );
	g_deferredCheckerBoardDescriptors.Cleanup( device );

	g_imageDiffuse.Cleanup( device );
	g_imageNormals.Cleanup( device );
	g_imageRoughness.Cleanup( device );
	g_imageSpecular.Cleanup( device );

	g_imageSkullDiffuse.Cleanup( device );
	g_imageSkullNormals.Cleanup( device );
	g_imageSkullRoughness.Cleanup( device );
	g_imageSkullSpecular.Cleanup( device );
}

static std::vector< Descriptor > g_depthDescriptorList;
static std::vector< Descriptor > g_descriptorList;

/*
====================================================
UpdateGBufferDescriptors
====================================================
*/
void UpdateGBufferDescriptors( DrawParms_t & parms ) {
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
		g_depthDescriptorList.reserve( numModels );
		g_depthDescriptorList.clear();

		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_depthPrePassGBufferPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			
			descriptor.UpdateDescriptor( device );
			g_depthDescriptorList.push_back( descriptor );
		}
	}

	//
	//	Fill g-buffer
	//
	{
		g_descriptorList.reserve( numModels );
		g_descriptorList.clear();

		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_deferredPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			descriptor.BindBuffer( parms.uniformsModelIDs, renderModel.uboByteOffsetModelID, renderModel.uboByteSizeModelID, 2 );
			if ( i < numModels - 1 ) {
				descriptor.BindImage( g_imageDiffuse, Samplers::m_samplerRepeat, 3 );
				descriptor.BindImage( g_imageNormals, Samplers::m_samplerRepeat, 4 );
				descriptor.BindImage( g_imageRoughness, Samplers::m_samplerRepeat, 5 );
				descriptor.BindImage( g_imageSpecular, Samplers::m_samplerRepeat, 6 );
			} else {
				descriptor.BindImage( g_imageSkullDiffuse, Samplers::m_samplerRepeat, 3 );
				descriptor.BindImage( g_imageSkullNormals, Samplers::m_samplerRepeat, 4 );
				descriptor.BindImage( g_imageSkullRoughness, Samplers::m_samplerRepeat, 5 );
				descriptor.BindImage( g_imageSkullSpecular, Samplers::m_samplerRepeat, 6 );
			}
			
			descriptor.UpdateDescriptor( device );
			g_descriptorList.push_back( descriptor );
		}
	}
}


/*
====================================================
UpdateGBufferCheckerBoardDescriptors
====================================================
*/
void UpdateGBufferCheckerBoardDescriptors( DrawParms_t & parms ) {
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
		g_depthDescriptorList.reserve( numModels );
		g_depthDescriptorList.clear();

		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_depthPrePassGBufferPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			
			descriptor.UpdateDescriptor( device );
			g_depthDescriptorList.push_back( descriptor );
		}
	}

	//
	//	Fill g-buffer
	//
	{
		g_descriptorList.reserve( numModels );
		g_descriptorList.clear();

		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_deferredCheckerBoardPipeline.GetFreeDescriptor();
			descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			descriptor.BindBuffer( parms.uniformsModelIDs, renderModel.uboByteOffsetModelID, renderModel.uboByteSizeModelID, 2 );
			descriptor.UpdateDescriptor( device );
			g_descriptorList.push_back( descriptor );
		}
	}
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
		int idx = 0;
		g_depthPrePassGBufferPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			g_depthDescriptorList[ idx ].BindDescriptor( cmdBuffer, &g_depthPrePassGBufferPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
			idx++;
		}
	}

	//
	//	Fill g-buffer
	//
	{
		int idx = 0;
		g_deferredPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			g_descriptorList[ idx ].BindDescriptor( cmdBuffer, &g_deferredPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
			idx++;
		}
	}
}


/*
====================================================
DrawGBufferCheckerBoard
====================================================
*/
void DrawGBufferCheckerBoard( DrawParms_t & parms ) {
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
		int idx = 0;
		g_depthPrePassGBufferPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			g_depthDescriptorList[ idx ].BindDescriptor( cmdBuffer, &g_depthPrePassGBufferPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
			idx++;
		}
	}

	//
	//	Fill g-buffer
	//
	{
		int idx = 0;
		g_deferredCheckerBoardPipeline.BindPipeline( cmdBuffer );
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}
			if ( renderModel.isTransparent ) {
				continue;
			}

			g_descriptorList[ idx ].BindDescriptor( cmdBuffer, &g_deferredCheckerBoardPipeline );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
			idx++;
		}
	}
}