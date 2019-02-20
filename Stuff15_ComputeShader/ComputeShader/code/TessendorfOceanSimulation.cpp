//
//  TessendorfOceanSimulation.cpp
//
#include "TessendorfOceanSimulation.h"
#include "shader.h"
#include "Fileio.h"
#include "Vector.h"
#include "Targa.h"
#include "application.h"
#include <assert.h>
#include <array>

VkPipelineLayout g_vkPipelineLayoutCompute;
VkPipeline g_vkPipelineCompute;

VkDescriptorPool g_vkDescriptorPoolCompute;
VkDescriptorSetLayout g_vkDescriptorSetLayoutCompute;
static const int g_numComputeDescriptors = 16;
VkDescriptorSet g_vkDescriptorSetsCompute[ g_numComputeDescriptors ];
int g_numComputeDescriptorsUsed = 0;

VkFormat g_vkTextureFormatCompute;
VkImage g_vkTextureImageCompute;
VkImageView g_vkTextureImageViewCompute;
VkDeviceMemory g_vkTextureDeviceMemoryCompute;
VkSampler g_vkTextureSamplerCompute;
VkExtent2D g_vkTextureExtentsCompute;

/*
====================================================
FindMemoryType
====================================================
*/
uint32_t FindMemoryType( VkPhysicalDevice vkPhysicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties ) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( vkPhysicalDevice, &memProperties );

	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ ) {
		if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & properties ) == properties ) {
			return i;
		}
	}

	printf( "failed to find suitable memory type!" );
	assert( 0 );
	return 0;
}

/*
====================================================
CreateTextureOceanSimulation
====================================================
*/
bool CreateTextureOceanSimulation( VkPhysicalDevice vkPhysicalDevice, VkDevice vkDevice, VkQueue vkGraphicsQueue, VkCommandBuffer vkCommandBuffer ) {
	//
	//	Texture loaded from file
	//
	{
		g_vkTextureFormatCompute = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

		Targa targaImage;
		const bool didLoad = targaImage.Load( "../common/data/images/companionCube.tga", true );
		if ( !didLoad ) {
			printf( "Image did not load!\n" );
			assert( 0 );
			return false;
		}

		{
			VkDeviceSize imageSize = targaImage.GetWidth() * targaImage.GetHeight() * 4;
			g_vkTextureExtentsCompute.width = targaImage.GetWidth();
			g_vkTextureExtentsCompute.height = targaImage.GetHeight();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;

			// Create the staging buffer
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = imageSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VkResult result = vkCreateBuffer( vkDevice, &bufferInfo, nullptr, &stagingBuffer );
				if ( VK_SUCCESS != result ) {
					printf( "failed to create buffer!\n" );
					assert( 0 );
					return false;
				}

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements( vkDevice, stagingBuffer, &memRequirements );

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType( vkPhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

				result = vkAllocateMemory( vkDevice, &allocInfo, nullptr, &stagingBufferMemory );
				if ( VK_SUCCESS != result ) {
					printf( "failed to allocate buffer memory!\n" );
					assert( 0 );
					return false;
				}

				vkBindBufferMemory( vkDevice, stagingBuffer, stagingBufferMemory, 0 );
			}

			// Map the staging buffer and copy the texture data
			void * data = NULL;
			vkMapMemory( vkDevice, stagingBufferMemory, 0, imageSize, 0, &data );
			memcpy( data, targaImage.DataPtr(), static_cast< size_t >( imageSize ) );
			vkUnmapMemory( vkDevice, stagingBufferMemory );

			// Create the image
			{
				VkImageCreateInfo imageInfo = {};
				imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				imageInfo.imageType = VK_IMAGE_TYPE_2D;
				imageInfo.extent.width = g_vkTextureExtentsCompute.width;
				imageInfo.extent.height = g_vkTextureExtentsCompute.height;
				imageInfo.extent.depth = 1;
				imageInfo.mipLevels = 1;
				imageInfo.arrayLayers = 1;
				imageInfo.format = g_vkTextureFormatCompute;
				imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
				imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VkResult result = vkCreateImage( vkDevice, &imageInfo, nullptr, &g_vkTextureImageCompute );
				if ( VK_SUCCESS != result ) {
					printf( "failed to create image!\n" );
					assert( 0 );
					return false;
				}

				VkMemoryRequirements memRequirements;
				vkGetImageMemoryRequirements( vkDevice, g_vkTextureImageCompute, &memRequirements );

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType( vkPhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

				result = vkAllocateMemory( vkDevice, &allocInfo, nullptr, &g_vkTextureDeviceMemoryCompute );
				if ( VK_SUCCESS != result ) {
					printf( "failed to allocate image memory!\n" );
					assert( 0 );
					return false;
				}

				vkBindImageMemory( vkDevice, g_vkTextureImageCompute, g_vkTextureDeviceMemoryCompute, 0 );
			}

			// Transition the image layout
			{
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkResetCommandBuffer( vkCommandBuffer, 0 );
				vkBeginCommandBuffer( vkCommandBuffer, &beginInfo );

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = g_vkTextureImageCompute;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;				

				vkCmdPipelineBarrier(
					vkCommandBuffer,
					sourceStage,
					destinationStage,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&barrier
				);

				vkEndCommandBuffer( vkCommandBuffer );
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &vkCommandBuffer;
				vkQueueSubmit( vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
				vkQueueWaitIdle( vkGraphicsQueue );
			}

			// Copy the buffer into the image
			{
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkResetCommandBuffer( vkCommandBuffer, 0 );
				vkBeginCommandBuffer( vkCommandBuffer, &beginInfo );

				VkBufferImageCopy region = {};
				region.bufferOffset = 0;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = {
					g_vkTextureExtentsCompute.width,
					g_vkTextureExtentsCompute.height,
					1
				};

				vkCmdCopyBufferToImage( vkCommandBuffer, stagingBuffer, g_vkTextureImageCompute, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

				vkEndCommandBuffer( vkCommandBuffer );
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &vkCommandBuffer;
				vkQueueSubmit( vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
				vkQueueWaitIdle( vkGraphicsQueue );
			}

			// Transition the image layout
			{
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkResetCommandBuffer( vkCommandBuffer, 0 );
				vkBeginCommandBuffer( vkCommandBuffer, &beginInfo );

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = g_vkTextureImageCompute;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				vkCmdPipelineBarrier(
					vkCommandBuffer,
					sourceStage,
					destinationStage,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&barrier
				);

				vkEndCommandBuffer( vkCommandBuffer );
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &vkCommandBuffer;
				vkQueueSubmit( vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
				vkQueueWaitIdle( vkGraphicsQueue );
			}

			vkDestroyBuffer( vkDevice, stagingBuffer, nullptr );
			vkFreeMemory( vkDevice, stagingBufferMemory, nullptr );
		}

		// Create Image View
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = g_vkTextureImageCompute;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = g_vkTextureFormatCompute;
			viewInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView( vkDevice, &viewInfo, nullptr, &g_vkTextureImageViewCompute );
			if ( VK_SUCCESS != result ) {
				printf( "failed to create texture image view!\n" );
				assert( 0 );
				return false;
			}
		}

		// Create Sampler
		{
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = 16;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			VkResult result = vkCreateSampler( vkDevice, &samplerInfo, nullptr, &g_vkTextureSamplerCompute );
			if ( VK_SUCCESS != result ) {
				assert( 0 );
				return false;
			}
		}
	}

	return true;
}

/*
====================================================
InitializeOceanSimulation
====================================================
*/
bool InitializeOceanSimulation( VkDevice vkDevice ) {
	//
	//	Shader Modules
	//
	VkShaderModule vkShaderModuleCompute;
	{
		unsigned int sizeComp = 0;
		unsigned char * compCode = NULL;
		if ( !GetFileData( "../common/data/shaders/computeTest.comp", &compCode, sizeComp ) ) {
			return false;
		}

		std::vector< unsigned int > compSpirv;
		if ( !Shader::CompileToSPIRV( Shader::SHADER_STAGE_COMPUTE, (char *)compCode, compSpirv ) ) {
			free( compCode );
			return false;
		}

		vkShaderModuleCompute = Shader::CreateShaderModule( vkDevice, (char *)compSpirv.data(), (int)( compSpirv.size() * sizeof( unsigned int ) ) );
		free( compCode );
	}

	VkDescriptorSetLayoutBinding bindings[ 1 ] = { { 0 } };
	bindings[ 0 ].binding = 0;
	bindings[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[ 0 ].descriptorCount = 1;
	bindings[ 0 ].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = bindings;
	VkResult result = vkCreateDescriptorSetLayout( vkDevice, &descriptorSetLayoutCreateInfo, nullptr, &g_vkDescriptorSetLayoutCompute );
	if ( VK_SUCCESS != result ) {
		printf( "Unable to create descriptor set layout\n" );
		assert( 0 );
		return false;
	}

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &g_vkDescriptorSetLayoutCompute;
	result = vkCreatePipelineLayout( vkDevice, &layoutInfo, nullptr, &g_vkPipelineLayoutCompute );
	if ( VK_SUCCESS != result ) {
		printf( "Unable to create pipeline layout\n" );
		assert( 0 );
		return false;
	}

	VkComputePipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineCreateInfo.stage.module = vkShaderModuleCompute;
	pipelineCreateInfo.stage.pName = "main";
	pipelineCreateInfo.layout = g_vkPipelineLayoutCompute;

	VkPipelineCache pipelineCache = VK_NULL_HANDLE;	// WTF is this for?  And why are we doing NULL for it everywhere?

	result = vkCreateComputePipelines( vkDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &g_vkPipelineCompute );
	if ( VK_SUCCESS != result ) {
		printf( "Unable to create compute pipeline\n" );
		assert( 0 );
		return false;
	}

	// Once the pipeline is created, we no longer need the shader modules
	vkDestroyShaderModule( vkDevice, vkShaderModuleCompute, nullptr );

	//
	// Create Descriptor Pool
	//
	{
		std::array< VkDescriptorPoolSize, 1 > poolSizes = {};
		poolSizes[ 0 ].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[ 0 ].descriptorCount = g_numComputeDescriptors;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast< uint32_t >( poolSizes.size() );
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = g_numComputeDescriptors;

		result = vkCreateDescriptorPool( vkDevice, &poolInfo, nullptr, &g_vkDescriptorPoolCompute );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create descriptor pool!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Create Descriptor Sets
	//
	{
		VkDescriptorSetLayout layouts[ g_numComputeDescriptors ];
		for ( int i = 0; i < g_numComputeDescriptors; i++ ) {
			layouts[ i ] = g_vkDescriptorSetLayoutCompute;
		}
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = g_vkDescriptorPoolCompute;
		allocInfo.descriptorSetCount = g_numComputeDescriptors;
		allocInfo.pSetLayouts = layouts;

		result = vkAllocateDescriptorSets( vkDevice, &allocInfo, g_vkDescriptorSetsCompute );
		if ( VK_SUCCESS != result ) {
			printf( "failed to allocate descriptor set!\n" );
			assert( 0 );
			return false;
		}
	}
	return true;
}

/*
====================================================
CreateComputeDescriptorSets
====================================================
*/
int CreateComputeDescriptorSets( VkDevice vkDevice ) {
	VkDescriptorImageInfo imageInfo = {};
	//	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;	// maybe not optimal?  but I wasn't sure what to use
	imageInfo.imageView = g_vkTextureImageViewCompute;
	imageInfo.sampler = g_vkTextureSamplerCompute;

	std::array< VkWriteDescriptorSet, 1 > descriptorWrites = {};

	descriptorWrites[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[ 0 ].dstSet = g_vkDescriptorSetsCompute[ g_numComputeDescriptorsUsed ];
	descriptorWrites[ 0 ].dstBinding = 0;
	descriptorWrites[ 0 ].dstArrayElement = 0;
	descriptorWrites[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[ 0 ].descriptorCount = 1;
	descriptorWrites[ 0 ].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets( vkDevice, static_cast< uint32_t >( descriptorWrites.size() ), descriptorWrites.data(), 0, nullptr );

	const int indexOut = g_numComputeDescriptorsUsed;
	g_numComputeDescriptorsUsed++;
	return indexOut;
}

/*
====================================================
DispatchCompute
====================================================
*/
struct computeParms_t {
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkPipeline;
	VkDescriptorSet m_vkDescriptorSet;
	VkDevice m_vkDevice;
	VkCommandBuffer m_vkCmdBuffer;
	unsigned int numGroupsX;
	unsigned int numGroupsY;
	unsigned int numGroupsZ;
};
void DispatchCompute( computeParms_t & parms ) {
	// Set the pipeline for this command (aka gpu state)
	vkCmdBindPipeline( parms.m_vkCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, parms.m_vkPipeline );

	// This binds the texture and whatever other buffers to a descriptor set, which we then attach as a command
	const int descriptorIdx = CreateComputeDescriptorSets( parms.m_vkDevice );
	vkCmdBindDescriptorSets( parms.m_vkCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, parms.m_vkPipelineLayout, 0, 1, &parms.m_vkDescriptorSet, 0, nullptr );

	// Now we create the command that dispatches it to run on the gpu
	vkCmdDispatch( parms.m_vkCmdBuffer, parms.numGroupsX, parms.numGroupsY, parms.numGroupsZ );
}

/*
====================================================
UpdateOceanSimulation
====================================================
*/
void UpdateOceanSimulation( VkDevice vkDevice, VkCommandBuffer vkCmdBuffer ) {
	g_numComputeDescriptorsUsed = 0;	// Reset the used compute descriptors used

	computeParms_t parms;
	parms.m_vkPipelineLayout = g_vkPipelineLayoutCompute;
	parms.m_vkPipeline = g_vkPipelineCompute;
	parms.m_vkDescriptorSet = g_vkDescriptorSetsCompute[ 0 ];
	parms.m_vkDevice = vkDevice;
	parms.m_vkCmdBuffer = vkCmdBuffer;
	parms.numGroupsX = g_vkTextureExtentsCompute.width / 16;
	parms.numGroupsY = g_vkTextureExtentsCompute.height / 16;
	parms.numGroupsZ = 1;
	DispatchCompute( parms );
}

/*
====================================================
CleanupOceanSimulation
====================================================
*/
void CleanupOceanSimulation( VkDevice vkDevice ) {
	// Destroy Pipeline State
	vkDestroyPipeline( vkDevice, g_vkPipelineCompute, nullptr );
	vkDestroyPipelineLayout( vkDevice, g_vkPipelineLayoutCompute, nullptr );

	// Destroy Descriptor Sets
	vkFreeDescriptorSets( vkDevice, g_vkDescriptorPoolCompute, g_numComputeDescriptors, g_vkDescriptorSetsCompute );
	vkDestroyDescriptorSetLayout( vkDevice, g_vkDescriptorSetLayoutCompute, nullptr );
	vkDestroyDescriptorPool( vkDevice, g_vkDescriptorPoolCompute, nullptr );

	// Destroy Texture
	vkDestroySampler( vkDevice, g_vkTextureSamplerCompute, nullptr );
	vkDestroyImageView( vkDevice, g_vkTextureImageViewCompute, nullptr );
	vkDestroyImage( vkDevice, g_vkTextureImageCompute, nullptr );
	vkFreeMemory( vkDevice, g_vkTextureDeviceMemoryCompute, nullptr );
}