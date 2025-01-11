//
//  Image.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Graphics/DeviceContext.h"

/*
====================================================
Image
====================================================
*/
class Image {
public:
	Image() {}
	~Image() {}

	struct CreateParms_t {
		VkImageUsageFlags usageFlags;
		VkFormat format;
		int width;
		int height;
		int depth;
		int mipLevels;
	};

	bool Create( DeviceContext * device, const CreateParms_t & parms );
	bool UploadData( DeviceContext * device, const void * data );
	void GenerateMipMaps( DeviceContext * device, VkCommandBuffer vkCommandBuffer );

	void Cleanup( DeviceContext * device );
	void TransitionLayout( DeviceContext * device );
	void TransitionLayout( VkCommandBuffer cmdBuffer, VkImageLayout newLayout );

	static int CalculateMipLevels( int width, int height );
	int GetByteSize() const;

	static void CopyImage( Image dstImage, Image srcImage, VkCommandBuffer cmdBuffer );
	static void BlitImage( Image dstImage, Image srcImage, VkCommandBuffer cmdBuffer );

	CreateParms_t	m_parms;
	VkImage			m_vkImage;
	VkImageView		m_vkImageView;	// We might need to create an image view for every mip in order to access each mip in a compute shader
	VkDeviceMemory	m_vkDeviceMemory;

	static const int maxMipViews = 8;
	VkImageView		m_vkImageMipChainViews[ maxMipViews ];	// 8 max mip levels?

	VkImageLayout	m_vkImageLayout;
};


class ImageCubeMap {
public:
	ImageCubeMap() {}
	~ImageCubeMap() {}

	VkImage			m_vkImage;
	VkDeviceMemory	m_vkDeviceMemory;
	VkImageLayout	m_vkImageLayout;
	VkImageView		m_vkImageView;

	bool Create( DeviceContext * device );
	void Cleanup( DeviceContext * device );
};

