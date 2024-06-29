//
//  Image.cpp
//
#include "Graphics/Image.h"
#include "Graphics/Buffer.h"
#include "Graphics/Barrier.h"
#include <assert.h>
#include <stdio.h>

/*
========================================================================================================

Image

========================================================================================================
*/

/*
====================================================
Image::Cleanup
====================================================
*/
bool Image::Create( DeviceContext * device, const CreateParms_t & parms ) {
	VkResult result;

	m_parms = parms;

	//
	//	Create the Image
	//

	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_1D;
	if ( m_parms.height > 1 ) {
		image.imageType = VK_IMAGE_TYPE_2D;
	}
	if ( m_parms.depth > 1 ) {
		image.imageType = VK_IMAGE_TYPE_3D;
	}
	
	image.extent.width = m_parms.width;
	image.extent.height = m_parms.height;
	image.extent.depth = m_parms.depth;
	image.mipLevels = m_parms.mipLevels;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = m_parms.format;
	if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	} else {
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	image.usage = parms.usageFlags;

	result = vkCreateImage( device->m_vkDevice, &image, nullptr, &m_vkImage );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create image\n" );
		assert( 0 );
		return false;
	}

	//
	//	Allocate memory on the GPU and attach it to the image
	//

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements( device->m_vkDevice, m_vkImage, &memReqs );
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = device->FindMemoryTypeIndex( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	result = vkAllocateMemory( device->m_vkDevice, &memAlloc, nullptr, &m_vkDeviceMemory );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to allocate memory\n" );
		assert( 0 );
		return false;
	}

	result = vkBindImageMemory( device->m_vkDevice, m_vkImage, m_vkDeviceMemory, 0 );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to bind image memory\n" );
		assert( 0 );
		return false;
	}

	//
	//	Create the image view
	//

	VkImageViewCreateInfo imageView = {};
	imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageView.viewType = VK_IMAGE_VIEW_TYPE_1D;
	if ( m_parms.height > 1 ) {
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	}
	if ( m_parms.depth > 1 ) {
		imageView.viewType = VK_IMAGE_VIEW_TYPE_3D;
	}

	imageView.format = m_parms.format;
	imageView.subresourceRange = {};
	if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
		imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else {
		imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	imageView.subresourceRange.baseMipLevel = 0;
	imageView.subresourceRange.levelCount = m_parms.mipLevels;
	imageView.subresourceRange.baseArrayLayer = 0;
	imageView.subresourceRange.layerCount = 1;
	imageView.image = m_vkImage;

	result = vkCreateImageView( device->m_vkDevice, &imageView, nullptr, &m_vkImageView );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create image view\n" );
		assert( 0 );
		return false;
	}

	//
	//	Create Mip Chain Image views
	//
	for ( int i = 0; i < m_parms.mipLevels && i < maxMipViews; i++ ) {
		VkImageViewCreateInfo imageView = {};
		imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageView.viewType = VK_IMAGE_VIEW_TYPE_1D;
		if ( m_parms.height > 1 ) {
			imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		}
		if ( m_parms.depth > 1 ) {
			imageView.viewType = VK_IMAGE_VIEW_TYPE_3D;
		}

		imageView.format = m_parms.format;
		imageView.subresourceRange = {};
		if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
			imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		} else {
			imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		imageView.subresourceRange.baseMipLevel = i;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = m_vkImage;

		result = vkCreateImageView( device->m_vkDevice, &imageView, nullptr, &m_vkImageMipChainViews[ i ] );
		if ( VK_SUCCESS != result ) {
			printf( "ERROR: Failed to create image view\n" );
			assert( 0 );
			return false;
		}
	}

	printf( "Create Image: 0x%04x\n", (unsigned int)m_vkImage );

	return true;
}

/*
====================================================
Image::GetByteSize
====================================================
*/
int Image::GetByteSize() const {
	int byteSize = -1;

	switch ( m_parms.format ) {
		default: { byteSize = -1; } break;
		case VkFormat::VK_FORMAT_R8G8B8A8_UNORM: {
			byteSize = sizeof( unsigned char ) * 4 * m_parms.width * m_parms.height * m_parms.depth;
		} break;
		case VkFormat::VK_FORMAT_R32G32_SFLOAT: { byteSize = sizeof( float ) * 2 * m_parms.width * m_parms.height * m_parms.depth; } break;
		case VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT: { byteSize = sizeof( float ) * 4 * m_parms.width * m_parms.height * m_parms.depth; } break;
		case VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT: { byteSize = sizeof( unsigned short ) * 4 * m_parms.width * m_parms.height * m_parms.depth; } break;
	};

	return byteSize;
}

/*
====================================================
Image::UploadData
====================================================
*/
bool Image::UploadData( DeviceContext * device, const void * data ) {
	const int byteSize = GetByteSize();
	if ( byteSize <= 0 ) {
		printf( "Warning: Image format not supported for uploading data!\n" );
		return false;
	}

	//
	//	Create the staging buffer
	//
	Buffer stagingBuffer;
	stagingBuffer.Allocate( device, data, byteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	//
	//	Now we need to transition the image on the gpu
	//
	VkCommandBuffer & vkCommandBuffer = device->m_vkCommandBuffers[ 0 ];

	// Transition the image layout
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkResetCommandBuffer( vkCommandBuffer, 0 );
		vkBeginCommandBuffer( vkCommandBuffer, &beginInfo );

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_vkImage;
		if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		} else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;				

		vkCmdPipelineBarrier(
			vkCommandBuffer,
			sourceStage,
			destinationStage,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		vkEndCommandBuffer( vkCommandBuffer );
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vkCommandBuffer;
		vkQueueSubmit( device->m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
		vkQueueWaitIdle( device->m_vkGraphicsQueue );
	}

	// Copy the buffer into the image
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkResetCommandBuffer( vkCommandBuffer, 0 );
		vkBeginCommandBuffer( vkCommandBuffer, &beginInfo );

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			(uint32_t)m_parms.width,
			(uint32_t)m_parms.height,
			(uint32_t)m_parms.depth
		};

		vkCmdCopyBufferToImage( vkCommandBuffer, stagingBuffer.m_vkBuffer, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

		vkEndCommandBuffer( vkCommandBuffer );
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vkCommandBuffer;
		vkQueueSubmit( device->m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
		vkQueueWaitIdle( device->m_vkGraphicsQueue );
	}

	// Transition the image layout
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkResetCommandBuffer( vkCommandBuffer, 0 );
		vkBeginCommandBuffer( vkCommandBuffer, &beginInfo );

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_vkImage;
		if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		} else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkCmdPipelineBarrier(
			vkCommandBuffer,
			sourceStage,
			destinationStage,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		vkEndCommandBuffer( vkCommandBuffer );
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vkCommandBuffer;
		vkQueueSubmit( device->m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
		vkQueueWaitIdle( device->m_vkGraphicsQueue );
	}

	stagingBuffer.Cleanup( device );
	
	m_vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	return true;
}

/*
====================================================
Image::GenerateMipMaps
====================================================
*/
void Image::GenerateMipMaps( DeviceContext * device, VkCommandBuffer vkCommandBuffer ) {
	// Get device properites for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties( device->m_vkPhysicalDevice, m_parms.format, &formatProperties );

	// Mip-chain generation requires support for blit source and destination
	assert( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT );
	assert( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT );
	if ( ( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT ) == 0 ) {
		printf( "Can't do it\n" );
		return;
	}
	if ( ( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) == 0 ) {
		printf( "Can't do it\n" );
		return;
	}

	//
	//	Prepare Base Mip Level
	//
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	MemoryBarriers::CreateImageMemoryBarrier(
		vkCommandBuffer,
		m_vkImage,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		subresourceRange
	);

	//
	//	Generate mip-chain
	//
	int numLevels = m_parms.mipLevels;
	for ( int32_t i = 1; i < numLevels; i++ ) {
		VkImageBlit imageBlit{};				

		// Source
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.layerCount = 1;
		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[ 0 ].x = 0;
		imageBlit.srcOffsets[ 0 ].y = 0;
		imageBlit.srcOffsets[ 0 ].z = 0;
		imageBlit.srcOffsets[ 1 ].x = int32_t( m_parms.width >> ( i - 1 ) );
		imageBlit.srcOffsets[ 1 ].y = int32_t( m_parms.height >> ( i - 1 ) );
		imageBlit.srcOffsets[ 1 ].z = 1;

		// Destination
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.layerCount = 1;
		imageBlit.dstSubresource.mipLevel = i;
		imageBlit.dstOffsets[ 0 ].x = 0;
		imageBlit.dstOffsets[ 0 ].y = 0;
		imageBlit.dstOffsets[ 0 ].z = 0;
		imageBlit.dstOffsets[ 1 ].x = int32_t( m_parms.width >> i );
		imageBlit.dstOffsets[ 1 ].y = int32_t( m_parms.height >> i );
		imageBlit.dstOffsets[ 1 ].z = 1;

		VkImageSubresourceRange mipSubRange = {};
		mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		mipSubRange.baseMipLevel = i;
		mipSubRange.levelCount = 1;
		mipSubRange.layerCount = 1;

		// Prepare current mip level as image blit destination
		MemoryBarriers::CreateImageMemoryBarrier(
			vkCommandBuffer,
			m_vkImage,
			(VkAccessFlagBits)0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			mipSubRange
		);

		vkCmdBlitImage(
			vkCommandBuffer,
			m_vkImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_vkImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlit,
			VK_FILTER_LINEAR
		);

		// Prepare current mip level as image blit source for next level
		MemoryBarriers::CreateImageMemoryBarrier(
			vkCommandBuffer,
			m_vkImage,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			mipSubRange
		);
	}

	// Transition all mip levels to the proper usage flags
	subresourceRange.levelCount = m_parms.mipLevels;
	MemoryBarriers::CreateImageMemoryBarrier(
		vkCommandBuffer,
		m_vkImage,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,//VK_IMAGE_LAYOUT_GENERAL
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		subresourceRange
	);
}

/*
====================================================
Image::Cleanup
====================================================
*/
void Image::Cleanup( DeviceContext * device ) {
	for ( int i = 0; i < m_parms.mipLevels && i < maxMipViews; i++ ) {
		vkDestroyImageView( device->m_vkDevice, m_vkImageMipChainViews[ i ], nullptr );
	}
	vkDestroyImageView( device->m_vkDevice, m_vkImageView, nullptr );
	vkDestroyImage( device->m_vkDevice, m_vkImage, nullptr );
	vkFreeMemory( device->m_vkDevice, m_vkDeviceMemory, nullptr );
}

/*
====================================================
Image::TransitionLayout
====================================================
*/
void Image::TransitionLayout( DeviceContext * device ) {
	// Transition the image layout
	VkCommandBuffer vkCommandBuffer = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = m_vkImageLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_vkImage;
	if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_parms.mipLevels;	// Transition all the mip levels
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;				

	vkCmdPipelineBarrier(
		vkCommandBuffer,
		sourceStage,
		destinationStage,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);

	device->FlushCommandBuffer( vkCommandBuffer, device->m_vkGraphicsQueue );

	m_vkImageLayout = VK_IMAGE_LAYOUT_GENERAL;
}

/*
====================================================
Image::TransitionLayout
====================================================
*/
void Image::TransitionLayout( VkCommandBuffer vkCommandBuffer, VkImageLayout newLayout ) {
	if ( m_vkImageLayout == newLayout ) {
		return;
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = m_vkImageLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_vkImage;
	if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.levelCount = m_parms.mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;				

	vkCmdPipelineBarrier(
		vkCommandBuffer,
		sourceStage,
		destinationStage,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);

	m_vkImageLayout = newLayout;
}

/*
====================================================
Image::CalculateMipLevels
====================================================
*/
int Image::CalculateMipLevels( int width, int height ) {
	int maxLength = std::max( width, height );
	int mipLevels = floor( log2( maxLength ) ) + 1;
	return mipLevels;
}