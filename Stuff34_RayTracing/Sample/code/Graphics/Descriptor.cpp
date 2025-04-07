//
//  Descriptor.cpp
//
#include "Graphics/Descriptor.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/Samplers.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Image.h"
#include "Graphics/AccelerationStructure.h"
#include "Models/ModelStatic.h"
#include <vector>
#include <assert.h>

/*
========================================================================================================

Descriptors

========================================================================================================
*/

/*
====================================================
Descriptors::Create

I think we're starting to see our confusion.

Do we use one global pool for all descriptor layouts?
Do we use a different pool for each unique layout?

Obviously the only way to know for sure is to try both.  And see what works.

I have a feeling that single global pool will work.  However, using a unique
pool for each unique layout, may be easier for us to play around with.

We will need to experiment with both techniques.  Figure out which way we like to do it.
See if one way is faster or more memory efficient than the other.
(We are likely to use whichever method is easiest for us)

For now, we're going to use a unique pool per layout.
====================================================
*/
bool Descriptors::Create( DeviceContext * device, const CreateParms_t & parms ) {
	VkResult result;

	m_parms = parms;

	//
	//	Create the non-global pool
	//
	std::vector< VkDescriptorPoolSize > poolSizes;
	const int numSamplerImages	= parms.numSamplerImagesFragment + parms.numSamplerImagesCompute + parms.numSamplerImagesTask;
	const int numStorageImages	= parms.numStorageImagesVertex + parms.numStorageImagesCompute + parms.numStorageImagesTask + parms.numStorageImagesRayGen;
	const int numStorageBuffers = parms.numStorageBuffersFragment + parms.numStorageBuffersVertex + parms.numStorageBuffersCompute + parms.numStorageBuffersMesh + parms.numStorageBuffersTask + parms.numStorageBuffersClosestHit + parms.numStorageBuffersRayGen;
	const int numUniformBuffers	= parms.numUniformsFragment + parms.numUniformsVertex + parms.numUniformsCompute + parms.numUniformsMesh + parms.numUniformsTask + parms.numUniformsRayGen;
	const int maxDescriptorSets = ( parms.maxDescriptorSets > 0 ) ? parms.maxDescriptorSets : MAX_DESCRIPTOR_SETS;

	if ( numUniformBuffers > 0 ) {
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = numUniformBuffers * maxDescriptorSets;
		poolSizes.push_back( poolSize );
	}
	if ( numSamplerImages > 0 ) {
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = numSamplerImages * maxDescriptorSets;
		poolSizes.push_back( poolSize );
	}
	if ( numStorageImages > 0 ) {
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = numStorageImages * maxDescriptorSets;
		poolSizes.push_back( poolSize );
	}
	if ( numStorageBuffers > 0 ) {
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize.descriptorCount = numStorageBuffers * maxDescriptorSets;
		poolSizes.push_back( poolSize );
	}
	if ( parms.numAccelerationStructures > 0 ) {
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		poolSize.descriptorCount = parms.numAccelerationStructures * maxDescriptorSets;
		poolSizes.push_back( poolSize );
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast< uint32_t >( poolSizes.size() );
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxDescriptorSets;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	result = vkCreateDescriptorPool( device->m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create descriptor pool\n" );
		assert( 0 );
		return false;
	}

	//
	// Create Descriptor Set Layout
	//
	const int totalUniforms = numUniformBuffers + numStorageImages + numSamplerImages + parms.numAccelerationStructures + numStorageBuffers;
	VkDescriptorSetLayoutBinding * uniformBindings = (VkDescriptorSetLayoutBinding *)alloca( sizeof( VkDescriptorSetLayoutBinding ) * ( totalUniforms ) );
	memset( uniformBindings, 0, sizeof( VkDescriptorSetLayoutBinding ) * totalUniforms );

	int idx = 0;
	{
		//
		//	Task Shader
		//
		for ( int i = 0; i < parms.numUniformsTask; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_TASK_BIT_NV;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageBuffersTask; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_TASK_BIT_NV;

			idx++;
		}

		for ( int i = 0; i < parms.numSamplerImagesTask; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_TASK_BIT_NV;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageImagesTask; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_TASK_BIT_NV;

			idx++;
		}

		//
		//	Mesh Shader
		//
		for ( int i = 0; i < parms.numUniformsMesh; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageBuffersMesh; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;

			idx++;
		}

		//
		//	Vertex Shader
		//
		for ( int i = 0; i < parms.numUniformsVertex; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageImagesVertex; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageBuffersVertex; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			idx++;
		}

		//
		//	Fragment Shader
		//
		for ( int i = 0; i < parms.numUniformsFragment; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			idx++;
		}

		for ( int i = 0; i < parms.numSamplerImagesFragment; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageBuffersFragment; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			idx++;
		}

		//
		//	Compute Shader
		//
		for ( int i = 0; i < parms.numUniformsCompute; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			idx++;
		}

		for ( int i = 0; i < parms.numSamplerImagesCompute; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageBuffersCompute; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageImagesCompute; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			idx++;
		}

		//
		//	Ray Gen Shader
		//
		for ( int i = 0; i < parms.numAccelerationStructures; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageImagesRayGen; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

			idx++;
		}

		for ( int i = 0; i < parms.numStorageBuffersRayGen; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

			idx++;
		}

		for ( int i = 0; i < parms.numUniformsRayGen; i++ ) {
			uniformBindings[ idx ].binding = idx;
			uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

			idx++;
		}

		//
		//	Closest Hit Shader
		//
		for ( int i = 0; i < parms.numStorageBuffersClosestHit; i++ ) {
			uniformBindings[ idx ].binding = idx;
			//uniformBindings[ idx ].descriptorCount = 1;
			uniformBindings[ idx ].descriptorCount = parms.numStorageBuffersClosestHitArraySize;
			uniformBindings[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uniformBindings[ idx ].pImmutableSamplers = nullptr;
			uniformBindings[ idx ].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

			idx++;
		}
	}

	assert( totalUniforms == idx );
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = totalUniforms;
	layoutInfo.pBindings = uniformBindings;

	result = vkCreateDescriptorSetLayout( device->m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create descriptor set layout\n" );
		assert( 0 );
		return false;
	}

	//
	//	Create Descriptor Sets
	//
	VkDescriptorSetLayout layouts[ MAX_DESCRIPTOR_SETS ];
	for ( int i = 0; i < MAX_DESCRIPTOR_SETS; i++ ) {
		layouts[ i ] = m_vkDescriptorSetLayout;
	}
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_vkDescriptorPool;
	allocInfo.descriptorSetCount = maxDescriptorSets;
	allocInfo.pSetLayouts = layouts;

	result = vkAllocateDescriptorSets( device->m_vkDevice, &allocInfo, m_vkDescriptorSets );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to allocate descriptor set\n" );
		assert( 0 );
		return false;
	}

	m_wasBuilt = true;
	return true;
}

/*
====================================================
Descriptors::Create
====================================================
*/
void Descriptors::Cleanup( DeviceContext * device ) {
	if ( m_wasBuilt ) {
		const int maxDescriptorSets = ( m_parms.maxDescriptorSets > 0 ) ? m_parms.maxDescriptorSets : MAX_DESCRIPTOR_SETS;

		// Free the descriptor sets
		vkFreeDescriptorSets( device->m_vkDevice, m_vkDescriptorPool, maxDescriptorSets, m_vkDescriptorSets );

		// Destroy descriptor set layout
		vkDestroyDescriptorSetLayout( device->m_vkDevice, m_vkDescriptorSetLayout, nullptr );

		// Destroy Descriptor Pool
		vkDestroyDescriptorPool( device->m_vkDevice, m_vkDescriptorPool, nullptr );
	}
}






/*
========================================================================================================

Descriptor

========================================================================================================
*/

/*
====================================================
Descriptor::Descriptor
====================================================
*/
Descriptor::Descriptor() {
	m_parent = NULL;
	m_id = -1;
	m_numImages = 0;
	m_numBuffers = 0;
	m_numStorageImages = 0;
	m_numStorageBuffers = 0;
	m_numAccelerationStructure = 0;
	memset( m_imageInfo, 0, sizeof( VkDescriptorImageInfo ) * MAX_IMAGEINFO );
	memset( m_bufferInfo, 0, sizeof( VkDescriptorBufferInfo ) * MAX_BUFFERS );
	memset( m_storageImageInfo, 0, sizeof( VkDescriptorImageInfo ) * MAX_STORAGEIMAGEINFO );
	memset( m_storageBufferInfo, 0, sizeof( VkDescriptorBufferInfo ) * MAX_STORAGEBUFFERINFO );
	memset( m_accelerationStructureInfo, 0, sizeof( VkWriteDescriptorSetAccelerationStructureNV ) * MAX_ACCELERATIONSTRUCTURES );
}

/*
====================================================
Descriptor::BindImage
====================================================
*/
void Descriptor::BindImage( const Image & image, VkSampler sampler, int slot ) {
	BindImage( image.m_vkImageLayout, image.m_vkImageView, sampler, slot );
}
void Descriptor::BindImage( VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler, int slot ) {
	assert( slot < MAX_IMAGEINFO );
	assert( m_numImages < MAX_IMAGEINFO );
	m_imageInfo[ m_numImages ].imageLayout = imageLayout;
	m_imageInfo[ m_numImages ].imageView = imageView;
	m_imageInfo[ m_numImages ].sampler = sampler;
	m_imageSlots[ m_numImages ] = slot;
	m_numImages++;
}

/*
====================================================
Descriptor::BindStorageImage
====================================================
*/
void Descriptor::BindStorageImage( const Image & image, VkSampler sampler, int slot ) {
	BindStorageImage( image.m_vkImageLayout, image.m_vkImageView, sampler, slot );
}
void Descriptor::BindStorageImage( VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler, int slot ) {
	assert( slot < MAX_STORAGEIMAGEINFO );
	assert( m_numStorageImages < MAX_STORAGEIMAGEINFO );
	m_storageImageInfo[ m_numStorageImages ].imageLayout = imageLayout;
	m_storageImageInfo[ m_numStorageImages ].imageView = imageView;
	m_storageImageInfo[ m_numStorageImages ].sampler = sampler;
	m_storageImageSlots[ m_numStorageImages ] = slot;
	m_numStorageImages++;
}

/*
====================================================
Descriptor::BindBuffer
====================================================
*/
void Descriptor::BindBuffer( Buffer * uniformBuffer, int offset, int size, int slot ) {
	assert( slot < MAX_BUFFERS );
	assert( m_numBuffers < MAX_BUFFERS );
	m_bufferInfo[ m_numBuffers ].buffer	= uniformBuffer->m_vkBuffer;
	m_bufferInfo[ m_numBuffers ].offset	= offset;
	m_bufferInfo[ m_numBuffers ].range	= size;
	m_bufferSlots[ m_numBuffers ] = slot;
	m_numBuffers++;
}

/*
====================================================
Descriptor::BindStorageBuffer
====================================================
*/
void Descriptor::BindStorageBuffer( Buffer * storageBuffer, int offset, int size, int slot ) {
	assert( slot < MAX_STORAGEBUFFERINFO );
	assert( m_numStorageBuffers < MAX_STORAGEBUFFERINFO );
	m_storageBufferInfo[ m_numStorageBuffers ].buffer	= storageBuffer->m_vkBuffer;
	m_storageBufferInfo[ m_numStorageBuffers ].offset	= offset;
	m_storageBufferInfo[ m_numStorageBuffers ].range	= size;
	m_storageBufferSlots[ m_numStorageBuffers ] = slot;
	m_numStorageBuffers++;
}

/*
====================================================
Descriptor::BindRenderModelsRTX
====================================================
*/
void Descriptor::BindRenderModelsRTX( RenderModel * models, int num, int slotVertices, int slotIndices ) {
	//
	//	Vertex Buffer
	//
	m_rtxVertexDescriptors.clear();
	for ( int i = 0; i < num; i++ ) {
		VkDescriptorBufferInfo vertexDescriptor;
		vertexDescriptor = {};
		vertexDescriptor.buffer	= models[ i ].modelDraw->m_vertexBuffer.m_vkBuffer;
		vertexDescriptor.offset	= 0;
		vertexDescriptor.range = models[ i ].modelDraw->m_vertexBuffer.m_vkBufferSize;
		m_rtxVertexDescriptors.push_back( vertexDescriptor );
	}
	m_rtxVertexDescriptorsSlot = slotVertices;

	//
	//	Index Buffer
	//
	m_rtxIndexDescriptors.clear();
	for ( int i = 0; i < num; i++ ) {
		VkDescriptorBufferInfo indexxDescriptor;
		indexxDescriptor = {};
		indexxDescriptor.buffer	= models[ i ].modelDraw->m_indexBuffer.m_vkBuffer;
		indexxDescriptor.offset	= 0;
		indexxDescriptor.range = models[ i ].modelDraw->m_indexBuffer.m_vkBufferSize;
		m_rtxIndexDescriptors.push_back( indexxDescriptor );
	}
	m_rtxIndexDescriptorsSlot = slotIndices;
}

/*
====================================================
Descriptor::BindAccelerationStructure
====================================================
*/
void Descriptor::BindAccelerationStructure( AccelerationStructure * accelerationStructure, int slot ) {
	assert( slot < MAX_ACCELERATIONSTRUCTURES );
	assert( m_numAccelerationStructure < MAX_ACCELERATIONSTRUCTURES );
	m_accelerationStructureInfo[ m_numAccelerationStructure ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	m_accelerationStructureInfo[ m_numAccelerationStructure ].accelerationStructureCount = 1;
	m_accelerationStructureInfo[ m_numAccelerationStructure ].pAccelerationStructures = &accelerationStructure->m_topLevel.accelerationStructure;
	m_accelerationStructureSlots[ m_numAccelerationStructure ] = slot;
	m_numAccelerationStructure++;
}

/*
====================================================
Descriptor::BindDescriptor
====================================================
*/
void Descriptor::BindDescriptor( DeviceContext * device, VkCommandBuffer vkCommandBuffer, Pipeline * pso ) {
	UpdateDescriptor( device );
	BindDescriptor( vkCommandBuffer, pso );
}


/*
====================================================
Descriptor::UpdateDescriptor
====================================================
*/
void Descriptor::UpdateDescriptor( DeviceContext * device ) {
	const int rtxNumVertexDescriptors = ( m_rtxVertexDescriptors.size() > 0 && m_rtxIndexDescriptors.size() > 0 ) ? 2 : 0;
	const int numDescriptors = m_numImages + m_numBuffers + m_numStorageImages + m_numStorageBuffers + m_numAccelerationStructure + rtxNumVertexDescriptors;
	const int allocationSize = sizeof( VkWriteDescriptorSet ) * numDescriptors;
	VkWriteDescriptorSet * descriptorWrites = (VkWriteDescriptorSet *)alloca( allocationSize );
	memset( descriptorWrites, 0, allocationSize );

	int idx = 0;
	
	// Write Acceleration Structures
	for ( int i = 0; i < m_numAccelerationStructure; i++ ) {
		VkWriteDescriptorSet accelerationStructureWrite{};
		descriptorWrites[ idx ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ idx ].dstSet = m_parent->m_vkDescriptorSets[ m_id ];
		descriptorWrites[ idx ].dstBinding = m_accelerationStructureSlots[ i ];
		descriptorWrites[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		descriptorWrites[ idx ].descriptorCount = 1;
		descriptorWrites[ idx ].pNext = &m_accelerationStructureInfo[ i ];	// The specialized acceleration structure descriptor has to be chained

		idx++;
	}

	// Write Uniform Buffers
	for ( int i = 0; i < m_numBuffers; i++ ) {
		descriptorWrites[ idx ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ idx ].dstSet = m_parent->m_vkDescriptorSets[ m_id ];
		descriptorWrites[ idx ].dstBinding = m_bufferSlots[ i ];
		descriptorWrites[ idx ].dstArrayElement = 0;
		descriptorWrites[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[ idx ].descriptorCount = 1;
		descriptorWrites[ idx ].pBufferInfo = &m_bufferInfo[ i ];

		idx++;
	}

	// Write Sampled Images
	for ( int i = 0; i < m_numImages; i++ ) {
		descriptorWrites[ idx ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ idx ].dstSet = m_parent->m_vkDescriptorSets[ m_id ];
		descriptorWrites[ idx ].dstBinding = m_imageSlots[ i ];
		descriptorWrites[ idx ].dstArrayElement = 0;
		descriptorWrites[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[ idx ].descriptorCount = 1;
		descriptorWrites[ idx ].pImageInfo = &m_imageInfo[ i ];

		idx++;
	}

	// Write Storage Buffers
	for ( int i = 0; i < m_numStorageBuffers; i++ ) {
		descriptorWrites[ idx ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ idx ].dstSet = m_parent->m_vkDescriptorSets[ m_id ];
		descriptorWrites[ idx ].dstBinding = m_storageBufferSlots[ i ];
		descriptorWrites[ idx ].dstArrayElement = 0;
		descriptorWrites[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[ idx ].descriptorCount = 1;
		descriptorWrites[ idx ].pBufferInfo = &m_storageBufferInfo[ i ];

		idx++;
	}

	// Write Storage Buffers RTX - Vertices
	if ( m_rtxVertexDescriptors.size() > 0 ) {
		descriptorWrites[ idx ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ idx ].dstSet = m_parent->m_vkDescriptorSets[ m_id ];
		descriptorWrites[ idx ].dstBinding = m_rtxVertexDescriptorsSlot;
		descriptorWrites[ idx ].dstArrayElement = 0;
		descriptorWrites[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[ idx ].descriptorCount = m_rtxVertexDescriptors.size();
		descriptorWrites[ idx ].pBufferInfo = m_rtxVertexDescriptors.data();

		idx++;
	}

	// Write Storage Buffers RTX - Indices
	if ( m_rtxIndexDescriptors.size() > 0 ) {
		descriptorWrites[ idx ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ idx ].dstSet = m_parent->m_vkDescriptorSets[ m_id ];
		descriptorWrites[ idx ].dstBinding = m_rtxIndexDescriptorsSlot;
		descriptorWrites[ idx ].dstArrayElement = 0;
		descriptorWrites[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[ idx ].descriptorCount = m_rtxIndexDescriptors.size();
		descriptorWrites[ idx ].pBufferInfo = m_rtxIndexDescriptors.data();

		idx++;
	}

	// Write Storage Images
	for ( int i = 0; i < m_numStorageImages; i++ ) {
		descriptorWrites[ idx ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ idx ].dstSet = m_parent->m_vkDescriptorSets[ m_id ];
		descriptorWrites[ idx ].dstBinding = m_storageImageSlots[ i ];
		descriptorWrites[ idx ].dstArrayElement = 0;
		descriptorWrites[ idx ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[ idx ].descriptorCount = 1;
		descriptorWrites[ idx ].pImageInfo = &m_storageImageInfo[ i ];

		idx++;
	}

	// Bind the uniforms
	vkUpdateDescriptorSets( device->m_vkDevice, (uint32_t)numDescriptors, descriptorWrites, 0, nullptr );
}


/*
====================================================
Descriptor::BindDescriptor
====================================================
*/
void Descriptor::BindDescriptor( VkCommandBuffer vkCommandBuffer, Pipeline * pso ) {
	VkPipelineBindPoint bindPoint;
	switch ( m_parent->m_parms.type ) {
		default:
		case Descriptors::TYPE_GRAPHICS: { bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; } break;
		case Descriptors::TYPE_COMPUTE: { bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE; } break;
		case Descriptors::TYPE_RAYTRACE: { bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_NV; } break;
	}
	vkCmdBindDescriptorSets( vkCommandBuffer, bindPoint, pso->m_vkPipelineLayout, 0, 1, &m_parent->m_vkDescriptorSets[ m_id ], 0, nullptr );
}