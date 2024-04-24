//
//  Barrier.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "DeviceContext.h"

class Image;

/*
====================================================
MemoryBarriers
====================================================
*/
class MemoryBarriers {
public:
	static void CreateMemoryBarrier( VkCommandBuffer cmdBuffer );
	static void CreateImageMemoryBarrier( VkCommandBuffer cmdBuffer, const Image & image, VkImageAspectFlagBits aspectMask );
	static void CreateImageMemoryBarrier( VkCommandBuffer cmdBuffer, VkImage vkImage, VkAccessFlagBits srcAccess, VkAccessFlagBits dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImageSubresourceRange subresource );
	static void CreateBufferMemoryBarrier( VkCommandBuffer cmdBuffer, VkBuffer vkBuffer, int offset, int size );
};