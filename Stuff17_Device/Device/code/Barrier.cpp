//
//  Barrier.cpp
//
#include "Barrier.h"
#include "Image.h"
#include <assert.h>
#include <stdio.h>

/*
========================================================================================================

MemoryBarriers

========================================================================================================
*/

/*
====================================================
MemoryBarriers::CreateMemoryBarrier
====================================================
*/
void MemoryBarriers::CreateMemoryBarrier( VkCommandBuffer cmdBuffer ) {
	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	VkMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

	vkCmdPipelineBarrier(
		cmdBuffer,
		sourceStage,
		destinationStage,
		0,
		1,
		&barrier,
		0,
		nullptr,
		0,
		nullptr
	);
}

/*
====================================================
MemoryBarriers::CreateImageMemoryBarrier
====================================================
*/
void MemoryBarriers::CreateImageMemoryBarrier( VkCommandBuffer cmdBuffer, const Image & image, VkImageAspectFlagBits aspectMask ) {
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = image.m_vkImageLayout;
	barrier.newLayout = image.m_vkImageLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image.m_vkImage;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	vkCmdPipelineBarrier(
		cmdBuffer,
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

void MemoryBarriers::CreateImageMemoryBarrier( VkCommandBuffer cmdBuffer, VkImage vkImage, VkAccessFlagBits srcAccess, VkAccessFlagBits dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImageSubresourceRange subresource ) {
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkImage;
	barrier.subresourceRange = subresource;
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;

	VkPipelineStageFlags sourceStage = srcStage;
	VkPipelineStageFlags destinationStage = dstStage;

	vkCmdPipelineBarrier(
		cmdBuffer,
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

/*
====================================================
MemoryBarriers::CreateBufferMemoryBarrier
====================================================
*/
void MemoryBarriers::CreateBufferMemoryBarrier( VkCommandBuffer cmdBuffer, VkBuffer vkBuffer, int offset, int size ) {
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = vkBuffer;
	barrier.offset = offset;
	barrier.size = size;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	vkCmdPipelineBarrier(
		cmdBuffer,
		sourceStage,
		destinationStage,
		0,
		0,
		nullptr,
		1,
		&barrier,
		0,
		nullptr
	);
}