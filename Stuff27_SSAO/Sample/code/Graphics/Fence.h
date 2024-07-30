//
//  Fence.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Graphics/DeviceContext.h"

/*
====================================================
Fence
====================================================
*/
class Fence {
public:
	bool Create( DeviceContext * device );
	bool Wait( DeviceContext * device );

	VkFence m_vkFence;
};