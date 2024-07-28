//
//  Common.cpp
//
#include "Renderer/Common.h"
#include "Models/ModelStatic.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Math.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Miscellaneous/Fileio.h"
#include <algorithm>

FrameBuffer		g_offscreenFrameBuffer;

GFrameBuffer	g_gbuffer;








/*
====================================================
InsertImageMemoryBarrier
====================================================
*/
void InsertImageMemoryBarrier( VkCommandBuffer cmdBuffer, VkImage dstImage, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImageSubresourceRange subResourceRange ) {
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.image = dstImage;
	imageMemoryBarrier.subresourceRange = subResourceRange;

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		cmdBuffer,
		srcQueueFamilyIndex,
		dstQueueFamilyIndex,
		0,
		0, nullptr,
		0, nullptr,
		1,
		&imageMemoryBarrier
	);
}
