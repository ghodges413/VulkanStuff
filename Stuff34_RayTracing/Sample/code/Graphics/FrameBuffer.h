//
//  FrameBuffer.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Graphics/Image.h"

class DeviceContext;

/*
====================================================
FrameBuffer

This frambeuffer handles both color and depth (seperate or together)
====================================================
*/
class FrameBuffer {
public:
	FrameBuffer() {}
	~FrameBuffer() {}

	struct CreateParms_t {
		int width;
		int height;
		bool hasDepth;
		bool hasColor;
		bool HDR;
	};

	bool Create( DeviceContext * device, CreateParms_t & parms );
	void Cleanup( DeviceContext * device );

	void BeginRenderPass( DeviceContext * device, const int cmdBufferIndex, bool clearColor, bool clearDepth );
	void EndRenderPass( DeviceContext * device, const int cmdBufferIndex );

	CreateParms_t			m_parms;

	Image					m_imageDepth;
	Image					m_imageColor;

	VkFramebuffer			m_vkFrameBuffer;
	VkRenderPass			m_vkRenderPass;

private:
	bool CreateRenderPass( DeviceContext * device );
};

/*
====================================================
GFrameBuffer

This is a special frame buffer used for deferred rendering
====================================================
*/
class GFrameBuffer {
public:
	GFrameBuffer() {}
	~GFrameBuffer() {}

	struct CreateParms_t {
		int width;
		int height;
	};

	bool Create( DeviceContext * device, CreateParms_t & parms );
	void Cleanup( DeviceContext * device );

	void BeginRenderPass( DeviceContext * device, const int cmdBufferIndex );
	void EndRenderPass( DeviceContext * device, const int cmdBufferIndex );

	CreateParms_t			m_parms;

	static const int s_numColorImages = 3;

	Image					m_imageDepth;
	Image					m_imageColor[ s_numColorImages ]; // pos, color, normal, tangent (normal and tangent can get packed together)

	VkFramebuffer			m_vkFrameBuffer;
	VkRenderPass			m_vkRenderPass;

private:
	bool CreateRenderPass( DeviceContext * device );
};
