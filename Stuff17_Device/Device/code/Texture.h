//
//  Texture.h
//
#include <vulkan/vulkan.h>
#include "DeviceContext.h"

/*
====================================================
Texture
====================================================
*/
class Texture {
public:
	Texture() {}
	~Texture() {}

	bool LoadFromFile( DeviceContext & deviceContext, const char * fileName );
	void Cleanup( DeviceContext & deviceContext );

	VkFormat		m_vkTextureFormat;
	VkImage			m_vkTextureImage;
	VkImageView		m_vkTextureImageView;
	VkDeviceMemory	m_vkTextureDeviceMemory;
	VkExtent2D		m_vkTextureExtents;

	static void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
};

