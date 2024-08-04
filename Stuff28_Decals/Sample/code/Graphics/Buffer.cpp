//
//  Buffer.cpp
//
#include "Graphics/Buffer.h"
#include <assert.h>
#include <string.h>

/*
================================================================================================

Buffer

================================================================================================
*/

/*
====================================================
Buffer::Buffer
====================================================
*/
Buffer::Buffer() :
	m_vkBufferSize( 0 ) {
}

/*
====================================================
Buffer::Allocate
====================================================
*/
bool Buffer::Allocate( DeviceContext * device, const void * data, int allocSize, unsigned int usageFlags, VkMemoryPropertyFlags memoryFlags ) {
	VkResult result;

	m_vkBufferSize = allocSize;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_vkBufferSize;
	bufferInfo.usage = usageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer( device->m_vkDevice, &bufferInfo, nullptr, &m_vkBuffer );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create buffer\n" );
		assert( 0 );
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( device->m_vkDevice, m_vkBuffer, &memRequirements );

	m_vkMemoryPropertyFlags = memoryFlags;

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device->FindMemoryTypeIndex( memRequirements.memoryTypeBits, m_vkMemoryPropertyFlags );

	result = vkAllocateMemory( device->m_vkDevice, &allocInfo, nullptr, &m_vkBufferMemory );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to allocate buffer memory\n" );
		assert( 0 );
		return false;
	}

	if ( NULL != data ) {
		void * memory = MapBuffer( device );
		memcpy( memory, data, allocSize );

		Flush( device );

		UnmapBuffer( device );
	}

	vkBindBufferMemory( device->m_vkDevice, m_vkBuffer, m_vkBufferMemory, 0 );
	return true;
}

/*
====================================================
Buffer::Cleanup
====================================================
*/
void Buffer::Cleanup( DeviceContext * device ) {
	if ( m_vkBufferSize > 0 ) {
		vkDestroyBuffer( device->m_vkDevice, m_vkBuffer, nullptr );
		vkFreeMemory( device->m_vkDevice, m_vkBufferMemory, nullptr );
		m_vkBufferSize = 0;
	}
}

/*
====================================================
Buffer::MapBuffer
====================================================
*/
void * Buffer::MapBuffer( DeviceContext * device ) {
	void * mapped_ptr = NULL;
	uint32_t byteOffset = 0;
	vkMapMemory( device->m_vkDevice, m_vkBufferMemory, byteOffset, m_vkBufferSize, 0, &mapped_ptr );
	return mapped_ptr;
}

/*
====================================================
Buffer::UnmapBuffer
====================================================
*/
void Buffer::UnmapBuffer( DeviceContext * device ) {
	vkUnmapMemory( device->m_vkDevice, m_vkBufferMemory );
}

/*
====================================================
Buffer::Flush
====================================================
*/
void Buffer::Flush( DeviceContext * device ) {
	// Only manually flush if the memory isn't mapped coherently
	if ( m_vkMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) {
		return;
	}

	VkMappedMemoryRange mappedRange = {};
	mappedRange.memory = m_vkBufferMemory;
	mappedRange.offset = 0;
	mappedRange.size = m_vkBufferSize;
	vkFlushMappedMemoryRanges( device->m_vkDevice, 1, &mappedRange );
}

