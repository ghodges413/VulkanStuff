//
//  Common.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Graphics/Image.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Shader.h"
#include "Graphics/Descriptor.h"
#include "Graphics/FrameBuffer.h"

class Buffer;
struct RenderModel;

struct DrawParms_t {
	DeviceContext * device;
	int cmdBufferIndex;
	Buffer * uniforms;
	RenderModel * renderModels;
	int numModels;
	float time;
	int scissorWidth;
	int scissorHeight;
};


extern FrameBuffer	g_offscreenFrameBuffer;

extern GFrameBuffer g_gbuffer;

extern FrameBuffer	g_debugDrawLightFrameBuffer;



void InsertImageMemoryBarrier( VkCommandBuffer cmdBuffer, VkImage dstImage, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImageSubresourceRange subResourceRange );
