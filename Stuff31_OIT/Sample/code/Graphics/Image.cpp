//
//  Image.cpp
//
#include "Graphics/Image.h"
#include "Graphics/Buffer.h"
#include "Graphics/Barrier.h"
#include "Miscellaneous/Fileio.h"
#include <assert.h>
#include <stdio.h>


unsigned int GetCubeOffset( const int face, const int mip, const int width, const int numMips ) {
	const int numFaces = 6;

	unsigned int offset = 0;
	for ( int m = 0; m < numMips; m++ ) {
		int w = width >> m;

		for ( int f = 0; f < numFaces; f++ ) {
			if ( mip == m && face == f ) {
				return offset;
			}

			offset += w * w * 4 * sizeof( float );
		}
	}

	return offset;
}


/*
========================================================================================================

Image

========================================================================================================
*/

/*
====================================================
Image::Create
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































// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 11.4 "Image Layout" for details

void setImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = {};//vks::initializers::imageMemoryBarrier();
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		imageMemoryBarrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newImageLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (imageMemoryBarrier.srcAccessMask == 0)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

// Fixed sub resource on first mip level and layer
void setImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}


void TransitionImageLayout( VkCommandBuffer vkCommandBuffer, VkImage image, VkImageLayout layoutPrev, VkImageLayout layoutNew, VkImageSubresourceRange range ) {
	if ( layoutPrev == layoutNew ) {
		return;
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = layoutPrev;
	barrier.newLayout = layoutNew;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
// 	if ( VK_FORMAT_D32_SFLOAT == m_parms.format ) {
// 		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
// 	} else {
// 		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
// 	}
// 	barrier.subresourceRange.baseMipLevel = 0;
// 	barrier.subresourceRange.levelCount = 1;
// 	barrier.subresourceRange.levelCount = m_parms.mipLevels;
// 	barrier.subresourceRange.baseArrayLayer = 0;
// 	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange = range;
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
}

bool ImageCubeMap::Create( DeviceContext * device ) {
	const int width = 256;
	const int height = 256;
	const int numMips = 8;
	const int numFaces = 6;
	const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	VkResult result;

	// Get properties required for using and upload texture data from the ktx texture object
	unsigned int textureSize = 0;
	unsigned char * textureData = NULL;
	GetFileData( "generated/probe.lightprobe", &textureData, textureSize );
	const float * debug_ptr = (float *)textureData;
	float * textureData2 = (float*)malloc( textureSize );
	memcpy( textureData2, textureData, textureSize );

	int offset = 0;
	for ( int mip = 0; mip < numMips; mip++ ) {
		const int w = width >> mip;
		const int h = height >> mip;

		for ( int face = 0; face < 6; face++ ) {
			for ( int v = 0; v < h; v++ ) {
				for ( int u = 0; u < w; u++ ) {
					int idxSrc = u + v * w;
					int idxDst = u + v * w;

					if ( 0 == face ) {	// +x face
						idxDst = v + u * h;
					}
					if ( 1 == face ) { // -x face
						int u2 = w - u;
						int v2 = h - v;
						idxDst = v2 + u2 * h;
					}
					if ( 2 == face ) {	// +y face
						int v2 = h - v;
						idxDst = u + v2 * w;
					}
					if ( 3 == face ) {	// -y face
						int u2 = w - u;
						int v2 = h - v;
						idxDst = u2 + v * w;
					}
					if ( 4 == face ) {	// +z face
						idxDst = v + u * h;
					}
					if ( 5 == face ) {	// -z face
						idxDst = v + u * h;
					}

					textureData2[ offset + idxDst * 4 + 0 ] = debug_ptr[ offset + idxSrc * 4 + 0 ];
					textureData2[ offset + idxDst * 4 + 1 ] = debug_ptr[ offset + idxSrc * 4 + 1 ];
					textureData2[ offset + idxDst * 4 + 2 ] = debug_ptr[ offset + idxSrc * 4 + 2 ];
					textureData2[ offset + idxDst * 4 + 3 ] = debug_ptr[ offset + idxSrc * 4 + 3 ];
				}
			}
			offset += w * h * 4;
		}
	}

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;


	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferCreateInfo.size = textureSize;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(device->m_vkDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	vkGetBufferMemoryRequirements(device->m_vkDevice, stagingBuffer, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	memAllocInfo.memoryTypeIndex = device->FindMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	result = vkAllocateMemory(device->m_vkDevice, &memAllocInfo, nullptr, &stagingMemory);
	result = vkBindBufferMemory(device->m_vkDevice, stagingBuffer, stagingMemory, 0);

	// Copy texture data into staging buffer
	uint8_t *data;
	result = vkMapMemory(device->m_vkDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data);
	memcpy(data, textureData2, textureSize );
	vkUnmapMemory(device->m_vkDevice, stagingMemory);

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = numMips;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageCreateInfo.arrayLayers = 6;
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	result = vkCreateImage(device->m_vkDevice, &imageCreateInfo, nullptr, &m_vkImage);

	vkGetImageMemoryRequirements(device->m_vkDevice, m_vkImage, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = device->FindMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkAllocateMemory(device->m_vkDevice, &memAllocInfo, nullptr, &m_vkDeviceMemory);
	result = vkBindImageMemory(device->m_vkDevice, m_vkImage, m_vkDeviceMemory, 0);

	VkCommandBuffer copyCmd = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Setup buffer copy regions for each face including all of its miplevels
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	//uint32_t offset = 0;

	for (uint32_t face = 0; face < 6; face++) {
		for (uint32_t level = 0; level < numMips; level++) {
			// Calculate offset into staging buffer for the current mip level and face
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width >> level;
			bufferCopyRegion.imageExtent.height = height >> level;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = GetCubeOffset( face, level, width, numMips );
			bufferCopyRegions.push_back(bufferCopyRegion);
		}
	}

	// Image barrier for optimal image (target)
	// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = numMips;
	subresourceRange.layerCount = 6;

	setImageLayout(
		copyCmd,
		m_vkImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	// Copy the cube map faces from the staging buffer to the optimal tiled image
	vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer,
		m_vkImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data()
	);

	// Change texture image layout to shader read after all faces have been copied
	m_vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	setImageLayout(
		copyCmd,
		m_vkImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		m_vkImageLayout,
		subresourceRange,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);

	device->FlushCommandBuffer(copyCmd, device->m_vkGraphicsQueue, true);


	// Create image view
	VkImageViewCreateInfo view = {};
	view.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	view.format = format;
	view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	view.subresourceRange.layerCount = 6;
	view.subresourceRange.levelCount = numMips;
	view.image = m_vkImage;
	vkCreateImageView(device->m_vkDevice, &view, nullptr, &m_vkImageView);

	// Clean up staging resources
	vkFreeMemory(device->m_vkDevice, stagingMemory, nullptr);
	vkDestroyBuffer(device->m_vkDevice, stagingBuffer, nullptr);

	free( textureData );
	free( textureData2 );
	return true;
}



/*
====================================================
ImageCubeMap::Cleanup
====================================================
*/
void ImageCubeMap::Cleanup( DeviceContext * device ) {
	vkDestroyImageView( device->m_vkDevice, m_vkImageView, nullptr );
	vkDestroyImage( device->m_vkDevice, m_vkImage, nullptr );
	vkFreeMemory( device->m_vkDevice, m_vkDeviceMemory, nullptr );
}










