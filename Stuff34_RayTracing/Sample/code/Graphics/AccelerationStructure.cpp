//
//  AccelerationStructure.cpp
//
#include "Graphics/AccelerationStructure.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/Samplers.h"
#include "Graphics/Pipeline.h"
#include "Models/ModelStatic.h"
#include <vector>
#include <assert.h>

/*
========================================================================================================

AccelerationStructure

========================================================================================================
*/

/*
====================================================
AccelerationStructure::CreateBottomLevel
====================================================
*/
bool AccelerationStructure::CreateBottomLevel( DeviceContext * device, const VkGeometryNV * geometries ) {
	VkResult result;

	VkAccelerationStructureInfoNV accelerationStructureInfo{};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 0;
	accelerationStructureInfo.geometryCount = 1;
	accelerationStructureInfo.pGeometries = geometries;

	VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
	accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCI.info = accelerationStructureInfo;
	result = vfs::vkCreateAccelerationStructureNV( device->m_vkDevice, &accelerationStructureCI, nullptr, &m_bottomLevel.accelerationStructure );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkCreateAccelerationStructureNV!\n" );
		assert( 0 );
		return false;
	}

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_bottomLevel.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2{};
	vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memoryRequirements2 );

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	//VkMemoryRequirements memReqs;
	//vkGetImageMemoryRequirements( deviceContext.m_vkDevice, m_offscreenPass.depth.image, &memReqs);
	//memAlloc.allocationSize = memReqs.size;
	//memAlloc.memoryTypeIndex = deviceContext.FindMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	//	VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	//	memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType( memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	//	memoryAllocateInfo.memoryTypeIndex = m_vkPhysicalDevice.getMemoryType( memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	memoryAllocateInfo.memoryTypeIndex = device->FindMemoryTypeIndex( memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	result = vkAllocateMemory( device->m_vkDevice, &memoryAllocateInfo, nullptr, &m_bottomLevel.memory );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkAllocateMemory!\n" );
		assert( 0 );
		return false;
	}

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_bottomLevel.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_bottomLevel.memory;

	result = vfs::vkBindAccelerationStructureMemoryNV( device->m_vkDevice, 1, &accelerationStructureMemoryInfo );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkBindAccelerationStructureMemoryNV!\n" );
		assert( 0 );
		return false;
	}

	result = vfs::vkGetAccelerationStructureHandleNV( device->m_vkDevice, m_bottomLevel.accelerationStructure, sizeof(uint64_t), &m_bottomLevel.handle );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
		assert( 0 );
		return false;
	}

	return true;
}

/*
====================================================
AccelerationStructure::CreateTopLevel
====================================================
*/
bool AccelerationStructure::CreateTopLevel( DeviceContext * device ) {
	VkResult result;

	VkAccelerationStructureInfoNV accelerationStructureInfo{};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 1;
	accelerationStructureInfo.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
	accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCI.info = accelerationStructureInfo;
	result = vfs::vkCreateAccelerationStructureNV( device->m_vkDevice, &accelerationStructureCI, nullptr, &m_topLevel.accelerationStructure );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
		assert( 0 );
		return false;
	}

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_topLevel.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2{};
	vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memoryRequirements2 );

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	//VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	//memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	memoryAllocateInfo.memoryTypeIndex = device->FindMemoryTypeIndex( memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	result = vkAllocateMemory( device->m_vkDevice, &memoryAllocateInfo, nullptr, &m_topLevel.memory );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
		assert( 0 );
		return false;
	}

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_topLevel.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_topLevel.memory;
	result = vfs::vkBindAccelerationStructureMemoryNV( device->m_vkDevice, 1, &accelerationStructureMemoryInfo );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
		assert( 0 );
		return false;
	}

	result = vfs::vkGetAccelerationStructureHandleNV( device->m_vkDevice, m_topLevel.accelerationStructure, sizeof(uint64_t), &m_topLevel.handle );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
		assert( 0 );
		return false;
	}

	return true;
}

/*
====================================================
AccelerationStructure::Finalize
====================================================
*/
bool AccelerationStructure::Finalize( DeviceContext * device, const VkGeometryNV * geometries ) {
	GeometryInstance geometryInstance;
	//geometryInstance.transform.Identity();
	geometryInstance.transform[ 0 ] = 1.0f;
	geometryInstance.transform[ 1 ] = 0.0f;
	geometryInstance.transform[ 2 ] = 0.0f;
	geometryInstance.transform[ 3 ] = 0.0f;

	geometryInstance.transform[ 4 ] = 0.0f;
	geometryInstance.transform[ 5 ] = 1.0f;
	geometryInstance.transform[ 6 ] = 0.0f;
	geometryInstance.transform[ 7 ] = 0.0f;

	geometryInstance.transform[ 8 ] = 0.0f;
	geometryInstance.transform[ 9 ] = 0.0f;
	geometryInstance.transform[ 10] = 1.0f;
	geometryInstance.transform[ 11] = 0.0f;

	geometryInstance.instanceId = 0;
	geometryInstance.mask = 0xff;
	geometryInstance.instanceOffset = 0;
	geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
	geometryInstance.accelerationStructureHandle = m_bottomLevel.handle;

	m_instanceBuffer.Allocate( device, &geometryInstance, sizeof( GeometryInstance ), VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );



#if 1
	//
	// Build the acceleration structure
	//

	// Acceleration structure build requires some scratch space to store temporary information
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkMemoryRequirements2 memReqBottomLevelAS;
	memoryRequirementsInfo.accelerationStructure = m_bottomLevel.accelerationStructure;//g_bottomLevelStructure.accelerationStructure;
	vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memReqBottomLevelAS );

	VkMemoryRequirements2 memReqTopLevelAS;
	memoryRequirementsInfo.accelerationStructure = m_topLevel.accelerationStructure;//g_topLevelStructure.accelerationStructure;
	vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memReqTopLevelAS );

	//const VkDeviceSize scratchBufferSize = std::max( memReqBottomLevelAS.memoryRequirements.size, memReqTopLevelAS.memoryRequirements.size );
	const VkDeviceSize scratchBufferSize = ( memReqBottomLevelAS.memoryRequirements.size > memReqTopLevelAS.memoryRequirements.size ) ? memReqBottomLevelAS.memoryRequirements.size : memReqTopLevelAS.memoryRequirements.size;
	/*
	vks::Buffer scratchBuffer;
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
	VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	&scratchBuffer,
	scratchBufferSize));
	*/
	Buffer scratchBuffer;
	scratchBuffer.Allocate( device, NULL, (int)scratchBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	//VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	//VkCommandBuffer vkCommandBuffer = m_deviceContext.m_vkCommandBuffers[ 0 ];
	VkCommandBuffer vkCommandBuffer = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );

	//
	// Build bottom level acceleration structure
	//
	VkAccelerationStructureInfoNV buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = geometries;// &geometry;

	vfs::vkCmdBuildAccelerationStructureNV(
		vkCommandBuffer,
		&buildInfo,
		VK_NULL_HANDLE,
		0,
		VK_FALSE,
		m_bottomLevel.accelerationStructure,
		VK_NULL_HANDLE,
		scratchBuffer.m_vkBuffer,
		0
	);

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier( vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0 );

	//
	// Build top-level acceleration structure
	//
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = 1;

	vfs::vkCmdBuildAccelerationStructureNV(
		vkCommandBuffer,
		&buildInfo,
		m_instanceBuffer.m_vkBuffer,
		0,
		VK_FALSE,
		m_topLevel.accelerationStructure,
		VK_NULL_HANDLE,
		scratchBuffer.m_vkBuffer,
		0
	);

	vkCmdPipelineBarrier( vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0 );

	//vulkanDevice->flushCommandBuffer( vkCommandBuffer, queue );
	device->FlushCommandBuffer( vkCommandBuffer, device->m_vkGraphicsQueue );

	scratchBuffer.Cleanup( device );
#endif

	return true;
}

/*
====================================================
AccelerationStructure::CreateTopLevel
====================================================
*/
void AccelerationStructure::Cleanup( DeviceContext * device ) {
	// Destroy ray tracing acceleration structures
	vkFreeMemory( device->m_vkDevice, m_bottomLevel.memory, nullptr );
	vkFreeMemory( device->m_vkDevice, m_topLevel.memory, nullptr );

	// Destroy ray tracing acceleration structures
	vfs::vkDestroyAccelerationStructureNV( device->m_vkDevice, m_bottomLevel.accelerationStructure, nullptr );
	vfs::vkDestroyAccelerationStructureNV( device->m_vkDevice, m_topLevel.accelerationStructure, nullptr );

	for ( int i = 0; i < m_bottomLevels.size(); i++ ) {
		vkFreeMemory( device->m_vkDevice, m_bottomLevels[ i ].memory, nullptr );
		vfs::vkDestroyAccelerationStructureNV( device->m_vkDevice, m_bottomLevels[ i ].accelerationStructure, nullptr );
	}

	m_instanceBuffer.Cleanup( device );
}

/*
====================================================
AccelerationStructure::AddGeometry
====================================================
*/
bool AccelerationStructure::AddGeometry( DeviceContext * device, Model * model, const Vec3 & pos, const Quat & orient ) {
	VkGeometryNV geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	geometry.geometry.triangles.vertexData = model->m_vertexBuffer.m_vkBuffer;
	geometry.geometry.triangles.vertexOffset = 0;
	geometry.geometry.triangles.vertexCount = (uint32_t)model->m_vertices.size();
	geometry.geometry.triangles.vertexStride = sizeof( vert_t );
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.indexData = model->m_indexBuffer.m_vkBuffer;
	geometry.geometry.triangles.indexOffset = 0;
	geometry.geometry.triangles.indexCount = (uint32_t)model->m_indices.size();
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
	geometry.geometry.triangles.transformOffset = 0;
	geometry.geometry.aabbs = {};
	geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

	m_geometries.push_back( geometry );

	//
	//	Build a bottom level structure for this geometry
	//
	{
		VkResult result;
		structure blas;

		VkAccelerationStructureInfoNV accelerationStructureInfo{};
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		accelerationStructureInfo.instanceCount = 0;	// bottom level doesn't have any instances, just geo
		accelerationStructureInfo.geometryCount = 1;
		accelerationStructureInfo.pGeometries = &m_geometries[ m_geometries.size() - 1 ];

		VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
		accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		accelerationStructureCI.info = accelerationStructureInfo;
		result = vfs::vkCreateAccelerationStructureNV( device->m_vkDevice, &accelerationStructureCI, nullptr, &blas.accelerationStructure );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkCreateAccelerationStructureNV!\n" );
			assert( 0 );
			return false;
		}

		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
		memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		memoryRequirementsInfo.accelerationStructure = blas.accelerationStructure;

		VkMemoryRequirements2 memoryRequirements2{};
		vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memoryRequirements2 );

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = device->FindMemoryTypeIndex( memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		result = vkAllocateMemory( device->m_vkDevice, &memoryAllocateInfo, nullptr, &blas.memory );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkAllocateMemory!\n" );
			assert( 0 );
			return false;
		}

		VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
		accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		accelerationStructureMemoryInfo.accelerationStructure = blas.accelerationStructure;
		accelerationStructureMemoryInfo.memory = blas.memory;

		result = vfs::vkBindAccelerationStructureMemoryNV( device->m_vkDevice, 1, &accelerationStructureMemoryInfo );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkBindAccelerationStructureMemoryNV!\n" );
			assert( 0 );
			return false;
		}

		result = vfs::vkGetAccelerationStructureHandleNV( device->m_vkDevice, blas.accelerationStructure, sizeof(uint64_t), &blas.handle );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
			assert( 0 );
			return false;
		}

		m_bottomLevels.push_back( blas );
	}

	//
	//	Create an instance for this model
	//
	Mat3 mat = orient.ToMat3();
	mat = mat.Transpose();
	GeometryInstance instance;
	memset( &instance, 0, sizeof( instance ) );
	instance.instanceId = m_instances.size();
	instance.mask = 0xff;
	instance.instanceOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
	instance.accelerationStructureHandle = m_bottomLevels[ m_bottomLevels.size() - 1 ].handle;

	instance.transform[ 0 ] = mat.rows[ 0 ][ 0 ];
	instance.transform[ 1 ] = mat.rows[ 0 ][ 1 ];
	instance.transform[ 2 ] = mat.rows[ 0 ][ 2 ];
	instance.transform[ 3 ] = pos.x;

	instance.transform[ 4 ] = mat.rows[ 1 ][ 0 ];
	instance.transform[ 5 ] = mat.rows[ 1 ][ 1 ];
	instance.transform[ 6 ] = mat.rows[ 1 ][ 2 ];
	instance.transform[ 7 ] = pos.y;

	instance.transform[ 8 ] = mat.rows[ 2 ][ 0 ];
	instance.transform[ 9 ] = mat.rows[ 2 ][ 1 ];
	instance.transform[ 10] = mat.rows[ 2 ][ 2 ];
	instance.transform[ 11] = pos.z;

	m_instances.push_back( instance );







	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkMemoryRequirements2 memReqBottomLevelAS;
	memoryRequirementsInfo.accelerationStructure = m_bottomLevels[ m_bottomLevels.size() - 1 ].accelerationStructure;
	vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memReqBottomLevelAS );
	unsigned int totalMemory = memReqBottomLevelAS.memoryRequirements.size;

	Buffer scratchBuffer;
	scratchBuffer.Allocate( device, NULL, (int)totalMemory, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	//
	//	Create a temporary command buffer for executing inline
	//
	VkCommandBuffer vkCommandBuffer = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );

	//
	//	Build bottom level acceleration structure
	//
	VkAccelerationStructureInfoNV buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &m_geometries[ m_geometries.size() - 1 ];

	vfs::vkCmdBuildAccelerationStructureNV(
		vkCommandBuffer,
		&buildInfo,
		VK_NULL_HANDLE,
		0,
		VK_FALSE,
		m_bottomLevels[ m_bottomLevels.size() - 1 ].accelerationStructure,
		VK_NULL_HANDLE,
		scratchBuffer.m_vkBuffer,
		0
	);

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier( vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0 );

	// Execute the commands
	device->FlushCommandBuffer( vkCommandBuffer, device->m_vkGraphicsQueue );
	scratchBuffer.Cleanup( device );
	return true;
}

/*
====================================================
AccelerationStructure::Build
====================================================
*/
bool AccelerationStructure::Build( DeviceContext * device ) {
	//
	//	Allocate the instance buffer and upload the instances
	//
	m_instanceBuffer.Allocate( device, m_instances.data(), sizeof( GeometryInstance ) * m_instances.size(), VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );


	//
	//	Build the top level acceleration structure
	//
	{
		VkResult result;

		VkAccelerationStructureInfoNV accelerationStructureInfo{};
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		accelerationStructureInfo.instanceCount = m_instances.size();
		accelerationStructureInfo.geometryCount = 0;	// Top level doesn't have any actual geo in it

		VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
		accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		accelerationStructureCI.info = accelerationStructureInfo;
		result = vfs::vkCreateAccelerationStructureNV( device->m_vkDevice, &accelerationStructureCI, nullptr, &m_topLevel.accelerationStructure );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
			assert( 0 );
			return false;
		}

		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
		memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		memoryRequirementsInfo.accelerationStructure = m_topLevel.accelerationStructure;

		VkMemoryRequirements2 memoryRequirements2{};
		vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memoryRequirements2 );

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = device->FindMemoryTypeIndex( memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		result = vkAllocateMemory( device->m_vkDevice, &memoryAllocateInfo, nullptr, &m_topLevel.memory );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
			assert( 0 );
			return false;
		}

		VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
		accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		accelerationStructureMemoryInfo.accelerationStructure = m_topLevel.accelerationStructure;
		accelerationStructureMemoryInfo.memory = m_topLevel.memory;
		result = vfs::vkBindAccelerationStructureMemoryNV( device->m_vkDevice, 1, &accelerationStructureMemoryInfo );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
			assert( 0 );
			return false;
		}

		result = vfs::vkGetAccelerationStructureHandleNV( device->m_vkDevice, m_topLevel.accelerationStructure, sizeof(uint64_t), &m_topLevel.handle );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create vkGetAccelerationStructureHandleNV!\n" );
			assert( 0 );
			return false;
		}
	}


	//
	// Acceleration structure build requires some scratch space to store temporary information
	//
	Buffer scratchBuffer;
	{
		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
		memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

		VkMemoryRequirements2 memReqTopLevelAS;
		memoryRequirementsInfo.accelerationStructure = m_topLevel.accelerationStructure;
		vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memReqTopLevelAS );
		unsigned int totalMemory = memReqTopLevelAS.memoryRequirements.size;
	
		scratchBuffer.Allocate( device, NULL, (int)totalMemory, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	}


	//
	//	Create a temporary command buffer for executing inline
	//
	VkCommandBuffer vkCommandBuffer = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );

	//
	//	Build bottom level acceleration structure
	//
	/*
	VkAccelerationStructureInfoNV buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.geometryCount = m_geometries.size();
	buildInfo.pGeometries = m_geometries.data();

	vfs::vkCmdBuildAccelerationStructureNV(
		vkCommandBuffer,
		&buildInfo,
		VK_NULL_HANDLE,
		0,
		VK_FALSE,
		m_bottomLevel.accelerationStructure,
		VK_NULL_HANDLE,
		scratchBuffer.m_vkBuffer,
		0
	);

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier( vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0 );
	*/

	//
	//	Build top-level acceleration structure
	//
	VkAccelerationStructureInfoNV buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = m_instances.size();

	vfs::vkCmdBuildAccelerationStructureNV(
		vkCommandBuffer,
		&buildInfo,
		m_instanceBuffer.m_vkBuffer,
		0,
		VK_FALSE,
		m_topLevel.accelerationStructure,
		VK_NULL_HANDLE,
		scratchBuffer.m_vkBuffer,
		0
	);

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier( vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0 );

	// Exectue the commmands
	device->FlushCommandBuffer( vkCommandBuffer, device->m_vkGraphicsQueue );

	scratchBuffer.Cleanup( device );
	return true;
}

/*
====================================================
AccelerationStructure::UpdateInstances
====================================================
*/
void AccelerationStructure::UpdateInstances( DeviceContext * device, const RenderModel * models ) {
	// Update the cpu instances
	for ( int i = 0; i < m_instances.size(); i++ ) {
		Vec3 pos = models[ i ].pos;
		Quat orient = models[ i ].orient;

		Mat3 mat = orient.ToMat3();
		mat = mat.Transpose();
		GeometryInstance & instance = m_instances[ i ];
		instance.transform[ 0 ] = mat.rows[ 0 ][ 0 ];
		instance.transform[ 1 ] = mat.rows[ 0 ][ 1 ];
		instance.transform[ 2 ] = mat.rows[ 0 ][ 2 ];
		instance.transform[ 3 ] = pos.x;

		instance.transform[ 4 ] = mat.rows[ 1 ][ 0 ];
		instance.transform[ 5 ] = mat.rows[ 1 ][ 1 ];
		instance.transform[ 6 ] = mat.rows[ 1 ][ 2 ];
		instance.transform[ 7 ] = pos.y;

		instance.transform[ 8 ] = mat.rows[ 2 ][ 0 ];
		instance.transform[ 9 ] = mat.rows[ 2 ][ 1 ];
		instance.transform[ 10] = mat.rows[ 2 ][ 2 ];
		instance.transform[ 11] = pos.z;
	}

	// Map the gpu instances and update them
	GeometryInstance * gpuInstances = (GeometryInstance *)m_instanceBuffer.MapBuffer( device );
	for ( int i = 0; i < m_instances.size(); i++ ) {
		for ( int j = 0; j < 12; j++ ) {
			gpuInstances[ i ].transform[ j ] = m_instances[ i ].transform[ j ];
		}
	}
	m_instanceBuffer.UnmapBuffer( device );


	// We need to manually rebuild the acceleration structure.
	// This can be done in an async compute shader, which should help hide the cost.
	// But doing it this way seems to be fast enough for our needs.

	//
	// Acceleration structure build requires some scratch space to store temporary information
	//
	Buffer scratchBuffer;
	{
		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
		memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

		VkMemoryRequirements2 memReqTopLevelAS;
		memoryRequirementsInfo.accelerationStructure = m_topLevel.accelerationStructure;
		vfs::vkGetAccelerationStructureMemoryRequirementsNV( device->m_vkDevice, &memoryRequirementsInfo, &memReqTopLevelAS );
		unsigned int totalMemory = memReqTopLevelAS.memoryRequirements.size;
	
		scratchBuffer.Allocate( device, NULL, (int)totalMemory, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	}

	//
	//	Create a temporary command buffer for executing inline
	//
	VkCommandBuffer vkCommandBuffer = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );

	//
	//	Build top-level acceleration structure
	//
	VkAccelerationStructureInfoNV buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = m_instances.size();

	vfs::vkCmdBuildAccelerationStructureNV(
		vkCommandBuffer,
		&buildInfo,
		m_instanceBuffer.m_vkBuffer,
		0,
		VK_FALSE,
		m_topLevel.accelerationStructure,
		VK_NULL_HANDLE,
		scratchBuffer.m_vkBuffer,
		0
	);

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier( vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0 );

	// Exectue the commmands
	device->FlushCommandBuffer( vkCommandBuffer, device->m_vkGraphicsQueue );

	scratchBuffer.Cleanup( device );
}