//
//  Descriptor.h
//
#pragma once
#include <vulkan/vulkan.h>

class DeviceContext;
class Texture;
class Buffer;
class Pipeline;
class Image;
class AccelerationStructure;
struct RenderModel;

/*
====================================================
Descriptor
====================================================
*/
class Descriptor {
public:
	Descriptor();
	~Descriptor() {}

	void BindImage( const Image & image, VkSampler sampler, int slot );
	void BindStorageImage( const Image & image, VkSampler sampler, int slot );

	void BindBuffer( Buffer * uniformBuffer, int offset, int size, int slot );
	void BindStorageBuffer( Buffer * storageBuffer, int offset, int size, int slot );

	void BindAccelerationStructure( AccelerationStructure * accelerationStructure, int slot );

	void UpdateDescriptor( DeviceContext * device );
	void BindDescriptor( VkCommandBuffer vkCommandBuffer, Pipeline * pso );
	void BindDescriptor( DeviceContext * device, VkCommandBuffer vkCommandBuffer, Pipeline * pso );

	friend class Descriptors;

	// Sometimes we need to specify a special imageView in order to access a particular subresource of an image, such as a specific mip level
	void BindImage( VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler, int slot );
	void BindStorageImage( VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler, int slot );

private:
	Descriptors * m_parent;

	int m_id;	// the id of the descriptor set to be used

	// Temporarily used
	int m_numBuffers;
	static const int MAX_BUFFERS = 16;
	VkDescriptorBufferInfo m_bufferInfo[ MAX_BUFFERS ];
	int m_bufferSlots[ MAX_BUFFERS ];

	int m_numImages;
	static const int MAX_IMAGEINFO = 16;
	VkDescriptorImageInfo m_imageInfo[ MAX_IMAGEINFO ];
	int m_imageSlots[ MAX_IMAGEINFO ];

	int m_numStorageImages;
	static const int MAX_STORAGEIMAGEINFO = 16;
	VkDescriptorImageInfo m_storageImageInfo[ MAX_STORAGEIMAGEINFO ];
	int m_storageImageSlots[ MAX_STORAGEIMAGEINFO ];

	int m_numStorageBuffers;
	static const int MAX_STORAGEBUFFERINFO = 16;
	VkDescriptorBufferInfo m_storageBufferInfo[ MAX_STORAGEBUFFERINFO ];
	int m_storageBufferSlots[ MAX_STORAGEBUFFERINFO ];

	int m_numAccelerationStructure;
	static const int MAX_ACCELERATIONSTRUCTURES = 16;
	VkWriteDescriptorSetAccelerationStructureNV m_accelerationStructureInfo[ MAX_ACCELERATIONSTRUCTURES ];
	int m_accelerationStructureSlots[ MAX_ACCELERATIONSTRUCTURES ];
};

/*
====================================================
DescriptorSets
====================================================
*/
class Descriptors {
public:
	Descriptors() : m_numDescriptorUsed( 0 ), m_wasBuilt( false ) {}
	~Descriptors() {}

	enum pipelineType_t {
		TYPE_GRAPHICS = 0,	// Good 'ol fashioned rasterization
		TYPE_COMPUTE,		// compute pipeline
		TYPE_RAYTRACE,		// ray tracing pipeline :)

		TYPE_NUM
	};

	// This structure could effectively create the layout
	struct CreateParms_t {
		pipelineType_t type;
		int maxDescriptorSets;

		int numUniformsTask;
		int numStorageBuffersTask;
		int numSamplerImagesTask;
		int numStorageImagesTask;

		int numUniformsMesh;
		int numStorageBuffersMesh;

		int numUniformsVertex;
		int numStorageImagesVertex;
		int numStorageBuffersVertex;

		int numUniformsFragment;
		int numSamplerImagesFragment;
		int numStorageBuffersFragment;
		
		int numUniformsCompute;
		int numSamplerImagesCompute;
		int numStorageImagesCompute;
		int numStorageBuffersCompute;

		int numAccelerationStructures;
		int numUniformsRayGen;
		int numStorageImagesRayGen;
		int numStorageBuffersRayGen;
		int numStorageBuffersClosestHit;		
	};

	bool Create( DeviceContext * device, const CreateParms_t & parms );
	void Cleanup( DeviceContext * device );


	CreateParms_t m_parms;
	bool m_wasBuilt;

	static const int MAX_DESCRIPTOR_SETS = 4096 * 3;

	// The pool is the object that allocates all our descriptor sets
	VkDescriptorPool m_vkDescriptorPool;

	// The layout tells vulkan, exactly that
	VkDescriptorSetLayout m_vkDescriptorSetLayout;

	// The individual descriptor sets
	int m_numDescriptorUsed;
	VkDescriptorSet m_vkDescriptorSets[ MAX_DESCRIPTOR_SETS ];


	Descriptor GetFreeDescriptor() {
		const int maxDescriptorSets = ( m_parms.maxDescriptorSets > 0 ) ? m_parms.maxDescriptorSets : MAX_DESCRIPTOR_SETS;

		Descriptor descriptor;
		descriptor.m_parent = this;
		descriptor.m_id = m_numDescriptorUsed % maxDescriptorSets;
		m_numDescriptorUsed++;
		return descriptor;
	}

	Descriptor GetRTXDescriptor() {
		Descriptor descriptor;
		descriptor.m_parent = this;
		descriptor.m_id = 0;
		m_numDescriptorUsed++;
		return descriptor;
	}
};


