//
//  DeviceContext.h
//
#pragma once
#include "SwapChain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <array>
#include <set>

#define ENABLE_RAYTRACING	// uncomment to enable ray tracing

/*
====================================================
Vulkan Extension Functions
====================================================
*/
class vfs {
public:
	static void Link( VkInstance instance );

	static PFN_vkCreateDebugReportCallbackEXT					vkCreateDebugReportCallbackEXT;
	static PFN_vkDestroyDebugReportCallbackEXT					vkDestroyDebugReportCallbackEXT;

	// Extensions
	static PFN_vkGetPhysicalDeviceFeatures2						vkGetPhysicalDeviceFeatures2;
	static PFN_vkGetPhysicalDeviceProperties2					vkGetPhysicalDeviceProperties2;

	// Ray Tracing Related
	static PFN_vkCreateAccelerationStructureNV					vkCreateAccelerationStructureNV;
	static PFN_vkDestroyAccelerationStructureNV					vkDestroyAccelerationStructureNV;
	static PFN_vkGetAccelerationStructureMemoryRequirementsNV	vkGetAccelerationStructureMemoryRequirementsNV;
	static PFN_vkBindAccelerationStructureMemoryNV				vkBindAccelerationStructureMemoryNV;
	static PFN_vkCmdBuildAccelerationStructureNV				vkCmdBuildAccelerationStructureNV;
	static PFN_vkCmdCopyAccelerationStructureNV					vkCmdCopyAccelerationStructureNV;
	static PFN_vkCmdTraceRaysNV									vkCmdTraceRaysNV;
	static PFN_vkCreateRayTracingPipelinesNV					vkCreateRayTracingPipelinesNV;
	static PFN_vkGetRayTracingShaderGroupHandlesNV				vkGetRayTracingShaderGroupHandlesNV;
	static PFN_vkGetAccelerationStructureHandleNV				vkGetAccelerationStructureHandleNV;
	static PFN_vkCmdWriteAccelerationStructuresPropertiesNV		vkCmdWriteAccelerationStructuresPropertiesNV;
	static PFN_vkCompileDeferredNV								vkCompileDeferredNV;

	// Mesh Shader Related
	static PFN_vkCmdDrawMeshTasksNV								vkCmdDrawMeshTasksNV;
	static PFN_vkCmdDrawMeshTasksIndirectNV						vkCmdDrawMeshTasksIndirectNV;
	static PFN_vkCmdDrawMeshTasksIndirectCountNV				vkCmdDrawMeshTasksIndirectCountNV;
};

/*
====================================================
PhysicalDeviceProperties
====================================================
*/
class PhysicalDeviceProperties {
public:
	VkPhysicalDevice						m_vkPhysicalDevice;
	VkPhysicalDeviceProperties				m_vkDeviceProperties;
	VkPhysicalDeviceProperties2				m_vkDeviceProperties2;
	VkPhysicalDeviceMemoryProperties		m_vkMemoryProperties;
	VkPhysicalDeviceFeatures				m_vkFeatures;
	VkPhysicalDeviceFeatures2				m_vkFeatures2;
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT	m_vkDescriptorIndexingFeatures;
	VkPhysicalDeviceMeshShaderFeaturesNV	m_vkMeshShaderFeatures;
	VkPhysicalDeviceMeshShaderPropertiesNV	m_vkMeshShaderProperties;
	VkPhysicalDeviceRayTracingPropertiesNV	m_vkRayTracingProperties;
	VkSurfaceCapabilitiesKHR				m_vkSurfaceCapabilities;
	std::vector< VkSurfaceFormatKHR >		m_vkSurfaceFormats;
	std::vector< VkPresentModeKHR >			m_vkPresentModes;
	std::vector< VkQueueFamilyProperties >	m_vkQueueFamilyProperties;
	std::vector< VkExtensionProperties >	m_vkExtensionProperties;

	bool AcquireProperties( VkPhysicalDevice device, VkSurfaceKHR vkSurface );
	bool HasExtensions( const char ** extensions, const int num ) const;
};

/*
====================================================
DeviceContext
====================================================
*/
class DeviceContext {
public:
	bool CreateInstance( bool enableLayers, const std::vector< const char * > & extensions );
	void Cleanup();

	bool m_enableLayers;
	VkInstance m_vkInstance;
	VkDebugReportCallbackEXT m_vkDebugCallback;

	VkSurfaceKHR m_vkSurface;

	bool CreateDevice();
	bool CreatePhysicalDevice();
	bool CreateLogicalDevice();

	std::vector< PhysicalDeviceProperties >	m_physicalDevices;

	//
	//	Device related
	//
	int m_deviceIndex;
	VkPhysicalDevice m_vkPhysicalDevice;
	VkDevice m_vkDevice;

	int m_graphicsFamilyIdx;
	int m_presentFamilyIdx;

	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;

	uint32_t FindMemoryTypeIndex( uint32_t typeFilter, VkMemoryPropertyFlags properties );

	static const std::vector< const char * > m_deviceExtensions;
	std::vector< const char * > m_validationLayers;

	//
	//	Command Buffers
	//
	bool CreateCommandBuffers();

	VkCommandPool m_vkCommandPool;
	std::vector< VkCommandBuffer > m_vkCommandBuffers;

	VkCommandBuffer CreateCommandBuffer( VkCommandBufferLevel level, bool begin = false );
	void FlushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, bool free = true );

	//
	//	Swap chain related
	//
	SwapChain m_swapChain;
	bool CreateSwapChain( int width, int height ) { return m_swapChain.Create( this, width, height ); }
	void ResizeWindow( int width, int height ) { m_swapChain.Resize( this, width, height ); }

	uint32_t BeginFrame() { return m_swapChain.BeginFrame( this ); }
	void EndFrame() { m_swapChain.EndFrame( this ); }

	void BeginRenderPass() { m_swapChain.BeginRenderPass( this ); }
	void EndRenderPass() { m_swapChain.EndRenderPass( this ); }

	int GetAlignedUniformByteOffset( const int offset ) const;
	int GetAlignedStorageByteOffset( const int offset ) const;
};

extern DeviceContext * g_device;