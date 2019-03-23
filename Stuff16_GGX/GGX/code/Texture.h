//
//  Texture.h
//
#include <vulkan/vulkan.h>

/*
====================================================
Texture
====================================================
*/
class Texture {
public:
	Texture() {}
	~Texture() {}

	bool LoadFromFile( VkDevice vkDevice, VkQueue vkGraphicsQueue, VkCommandBuffer vkCommandBuffer, const char * fileName );
	void Cleanup( VkDevice vkDevice );

	VkFormat m_vkTextureFormat;
	VkImage m_vkTextureImage;
	VkImageView m_vkTextureImageView;
	VkDeviceMemory m_vkTextureDeviceMemory;
	VkSampler m_vkTextureSampler;			// We shouldn't need a unique sampler per texture
	VkExtent2D m_vkTextureExtents;
};