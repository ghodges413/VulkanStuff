//
//  Texture.cpp
//
#include "Texture.h"
#include "Samplers.h"
#include "Targa.h"
#include "Fileio.h"
#include "application.h"
#include <assert.h>
#include <stdio.h>

/*
====================================================
Texture::Cleanup
====================================================
*/
void Texture::Cleanup( DeviceContext & deviceContext ) {
	vkDestroyImageView( deviceContext.m_vkDevice, m_vkTextureImageView, nullptr );
	vkDestroyImage( deviceContext.m_vkDevice, m_vkTextureImage, nullptr );
	vkFreeMemory( deviceContext.m_vkDevice, m_vkTextureDeviceMemory, nullptr );
}

/*
====================================================
Texture::LoadFromFile
====================================================
*/
bool Texture::LoadFromFile( DeviceContext & deviceContext, const char * fileName ) {
	m_vkTextureFormat = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	VkCommandBuffer & vkCommandBuffer = deviceContext.m_vkCommandBuffers[ 0 ];

	Targa targaImage;
	const bool didLoad = targaImage.Load( fileName, true );
	if ( !didLoad ) {
		printf( "Image did not load!\n" );
		assert( 0 );
		return false;
	}

	{
		VkDeviceSize imageSize = targaImage.GetWidth() * targaImage.GetHeight() * 4;
		m_vkTextureExtents.width = targaImage.GetWidth();
		m_vkTextureExtents.height = targaImage.GetHeight();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// Create the staging buffer
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = imageSize;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkResult result = vkCreateBuffer( deviceContext.m_vkDevice, &bufferInfo, nullptr, &stagingBuffer );
			if ( VK_SUCCESS != result ) {
				printf( "failed to create buffer!\n" );
				assert( 0 );
				return false;
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements( deviceContext.m_vkDevice, stagingBuffer, &memRequirements );

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = deviceContext.FindMemoryTypeIndex( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

			result = vkAllocateMemory( deviceContext.m_vkDevice, &allocInfo, nullptr, &stagingBufferMemory );
			if ( VK_SUCCESS != result ) {
				printf( "failed to allocate buffer memory!\n" );
				assert( 0 );
				return false;
			}

			vkBindBufferMemory( deviceContext.m_vkDevice, stagingBuffer, stagingBufferMemory, 0 );
		}

		// Map the staging buffer and copy the texture data
		void * data = NULL;
		vkMapMemory( deviceContext.m_vkDevice, stagingBufferMemory, 0, imageSize, 0, &data );
		memcpy( data, targaImage.DataPtr(), static_cast< size_t >( imageSize ) );
		vkUnmapMemory( deviceContext.m_vkDevice, stagingBufferMemory );

		// Create the image
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = m_vkTextureExtents.width;
			imageInfo.extent.height = m_vkTextureExtents.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = m_vkTextureFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkResult result = vkCreateImage( deviceContext.m_vkDevice, &imageInfo, nullptr, &m_vkTextureImage );
			if ( VK_SUCCESS != result ) {
				printf( "failed to create image!\n" );
				assert( 0 );
				return false;
			}

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements( deviceContext.m_vkDevice, m_vkTextureImage, &memRequirements );

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = deviceContext.FindMemoryTypeIndex( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

			result = vkAllocateMemory( deviceContext.m_vkDevice, &allocInfo, nullptr, &m_vkTextureDeviceMemory );
			if ( VK_SUCCESS != result ) {
				printf( "failed to allocate image memory!\n" );
				assert( 0 );
				return false;
			}

			vkBindImageMemory( deviceContext.m_vkDevice, m_vkTextureImage, m_vkTextureDeviceMemory, 0 );
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
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_vkTextureImage;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
			vkQueueSubmit( deviceContext.m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
			vkQueueWaitIdle( deviceContext.m_vkGraphicsQueue );
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
				m_vkTextureExtents.width,
				m_vkTextureExtents.height,
				1
			};

			vkCmdCopyBufferToImage( vkCommandBuffer, stagingBuffer, m_vkTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

			vkEndCommandBuffer( vkCommandBuffer );
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &vkCommandBuffer;
			vkQueueSubmit( deviceContext.m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
			vkQueueWaitIdle( deviceContext.m_vkGraphicsQueue );
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
			barrier.image = m_vkTextureImage;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
			vkQueueSubmit( deviceContext.m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
			vkQueueWaitIdle( deviceContext.m_vkGraphicsQueue );
		}

		vkDestroyBuffer( deviceContext.m_vkDevice, stagingBuffer, nullptr );
		vkFreeMemory( deviceContext.m_vkDevice, stagingBufferMemory, nullptr );
	}

	// Create Image View
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_vkTextureImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_vkTextureFormat;
		viewInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView( deviceContext.m_vkDevice, &viewInfo, nullptr, &m_vkTextureImageView );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create texture image view!\n" );
			assert( 0 );
			return false;
		}
	}

	return true;
}

/*
====================================================
Texture::SetImageLayout
====================================================
*/
void Texture::SetImageLayout(
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