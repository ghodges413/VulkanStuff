//
//  Fence.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "DeviceContext.h"

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