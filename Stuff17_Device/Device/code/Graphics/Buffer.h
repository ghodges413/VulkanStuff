//
//  Buffer.h
//
#pragma once
#include "Graphics/DeviceContext.h"

/*
================================================================================================

Buffer

A general buffer

================================================================================================
*/

class Buffer {
public:
	Buffer();

	bool Allocate( DeviceContext * device, const void * data, int allocSize, unsigned int usageFlags, VkMemoryPropertyFlags memoryFlags );
	void Cleanup( DeviceContext * device );
	void * MapBuffer( DeviceContext * device );
	void UnmapBuffer( DeviceContext * device );
	void Flush( DeviceContext * device );

	VkBuffer		m_vkBuffer;
	VkDeviceMemory	m_vkBufferMemory;
	VkDeviceSize	m_vkBufferSize;
	VkMemoryPropertyFlags m_vkMemoryPropertyFlags;
};