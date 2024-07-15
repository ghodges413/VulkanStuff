//
//  DeviceContext.cpp
//
#include "Graphics/DeviceContext.h"
#include "Graphics/Fence.h"
#include <assert.h>

DeviceContext * g_device = NULL;

/*
================================================================================================

vfs

================================================================================================
*/

PFN_vkCreateDebugReportCallbackEXT					vfs::vkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT					vfs::vkDestroyDebugReportCallbackEXT;

PFN_vkGetPhysicalDeviceFeatures2					vfs::vkGetPhysicalDeviceFeatures2;
PFN_vkGetPhysicalDeviceProperties2					vfs::vkGetPhysicalDeviceProperties2;

PFN_vkCreateAccelerationStructureNV					vfs::vkCreateAccelerationStructureNV;
PFN_vkDestroyAccelerationStructureNV				vfs::vkDestroyAccelerationStructureNV;
PFN_vkGetAccelerationStructureMemoryRequirementsNV	vfs::vkGetAccelerationStructureMemoryRequirementsNV;
PFN_vkBindAccelerationStructureMemoryNV				vfs::vkBindAccelerationStructureMemoryNV;
PFN_vkCmdBuildAccelerationStructureNV				vfs::vkCmdBuildAccelerationStructureNV;
PFN_vkCmdCopyAccelerationStructureNV				vfs::vkCmdCopyAccelerationStructureNV;
PFN_vkCmdTraceRaysNV								vfs::vkCmdTraceRaysNV;
PFN_vkCreateRayTracingPipelinesNV					vfs::vkCreateRayTracingPipelinesNV;
PFN_vkGetRayTracingShaderGroupHandlesNV				vfs::vkGetRayTracingShaderGroupHandlesNV;
PFN_vkGetAccelerationStructureHandleNV				vfs::vkGetAccelerationStructureHandleNV;
PFN_vkCmdWriteAccelerationStructuresPropertiesNV	vfs::vkCmdWriteAccelerationStructuresPropertiesNV;
PFN_vkCompileDeferredNV								vfs::vkCompileDeferredNV;

PFN_vkCmdDrawMeshTasksNV							vfs::vkCmdDrawMeshTasksNV;
PFN_vkCmdDrawMeshTasksIndirectNV					vfs::vkCmdDrawMeshTasksIndirectNV;
PFN_vkCmdDrawMeshTasksIndirectCountNV				vfs::vkCmdDrawMeshTasksIndirectCountNV;

/*
====================================================
vfs::Link
====================================================
*/
void vfs::Link( VkInstance instance ) {
	vfs::vkCreateDebugReportCallbackEXT					= (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugReportCallbackEXT" );
	vfs::vkDestroyDebugReportCallbackEXT				= (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" );

	// Extensions
	vfs::vkGetPhysicalDeviceFeatures2					= (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFeatures2" );
	vfs::vkGetPhysicalDeviceProperties2					= (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties2" );

	// Ray Tracing Related
	vfs::vkCreateAccelerationStructureNV				= (PFN_vkCreateAccelerationStructureNV)vkGetInstanceProcAddr( instance, "vkCreateAccelerationStructureNV" );
	vfs::vkDestroyAccelerationStructureNV				= (PFN_vkDestroyAccelerationStructureNV)vkGetInstanceProcAddr( instance, "vkDestroyAccelerationStructureNV" );
	vfs::vkGetAccelerationStructureMemoryRequirementsNV	= (PFN_vkGetAccelerationStructureMemoryRequirementsNV)vkGetInstanceProcAddr( instance, "vkGetAccelerationStructureMemoryRequirementsNV" );
	vfs::vkBindAccelerationStructureMemoryNV			= (PFN_vkBindAccelerationStructureMemoryNV)vkGetInstanceProcAddr( instance, "vkBindAccelerationStructureMemoryNV" );
	vfs::vkCmdBuildAccelerationStructureNV				= (PFN_vkCmdBuildAccelerationStructureNV)vkGetInstanceProcAddr( instance, "vkCmdBuildAccelerationStructureNV" );
	vfs::vkCmdCopyAccelerationStructureNV				= (PFN_vkCmdCopyAccelerationStructureNV)vkGetInstanceProcAddr( instance, "vkCmdCopyAccelerationStructureNV" );
	vfs::vkCmdTraceRaysNV								= (PFN_vkCmdTraceRaysNV)vkGetInstanceProcAddr( instance, "vkCmdTraceRaysNV" );
	vfs::vkCreateRayTracingPipelinesNV					= (PFN_vkCreateRayTracingPipelinesNV)vkGetInstanceProcAddr( instance, "vkCreateRayTracingPipelinesNV" );
	vfs::vkGetRayTracingShaderGroupHandlesNV			= (PFN_vkGetRayTracingShaderGroupHandlesNV)vkGetInstanceProcAddr( instance, "vkGetRayTracingShaderGroupHandlesNV" );
	vfs::vkGetAccelerationStructureHandleNV				= (PFN_vkGetAccelerationStructureHandleNV)vkGetInstanceProcAddr( instance, "vkGetAccelerationStructureHandleNV" );
	vfs::vkCmdWriteAccelerationStructuresPropertiesNV	= (PFN_vkCmdWriteAccelerationStructuresPropertiesNV)vkGetInstanceProcAddr( instance, "vkCmdWriteAccelerationStructuresPropertiesNV" );
	vfs::vkCompileDeferredNV							= (PFN_vkCompileDeferredNV)vkGetInstanceProcAddr( instance, "vkCompileDeferredNV" );

	// Mesh Shader Related
	vfs::vkCmdDrawMeshTasksNV							= (PFN_vkCmdDrawMeshTasksNV)vkGetInstanceProcAddr( instance, "vkCmdDrawMeshTasksNV" );
	vfs::vkCmdDrawMeshTasksIndirectNV					= (PFN_vkCmdDrawMeshTasksIndirectNV)vkGetInstanceProcAddr( instance, "vkCmdDrawMeshTasksIndirectNV" );
	vfs::vkCmdDrawMeshTasksIndirectCountNV				= (PFN_vkCmdDrawMeshTasksIndirectCountNV)vkGetInstanceProcAddr( instance, "vkCmdDrawMeshTasksIndirectCountNV" );
}

/*
================================================================================================

PhysicalDeviceProperties

================================================================================================
*/

/*
====================================================
PhysicalDeviceProperties::AcquireProperties
====================================================
*/
bool PhysicalDeviceProperties::AcquireProperties( VkPhysicalDevice device, VkSurfaceKHR vkSurface ) {
	VkResult result;

	m_vkPhysicalDevice = device;

	vkGetPhysicalDeviceProperties( m_vkPhysicalDevice, &m_vkDeviceProperties );
	vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &m_vkMemoryProperties );		
	vkGetPhysicalDeviceFeatures( m_vkPhysicalDevice, &m_vkFeatures );

	// Get Extended Device Properties
	{
		memset( &m_vkRayTracingProperties, 0, sizeof( m_vkRayTracingProperties ) );
		m_vkRayTracingProperties.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

		memset( &m_vkMeshShaderProperties, 0, sizeof( m_vkMeshShaderProperties ) );
		m_vkMeshShaderProperties.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
		m_vkMeshShaderProperties.pNext = &m_vkRayTracingProperties;

		memset( &m_vkDeviceProperties2, 0, sizeof( m_vkDeviceProperties2 ) );
		m_vkDeviceProperties2.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		m_vkDeviceProperties2.pNext = &m_vkMeshShaderProperties;
		vfs::vkGetPhysicalDeviceProperties2( m_vkPhysicalDevice, &m_vkDeviceProperties2 );
	}

	// Get Extended Device Features
	{
		memset( &m_vkDescriptorIndexingFeatures, 0, sizeof( m_vkDescriptorIndexingFeatures ) );
		m_vkDescriptorIndexingFeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		m_vkDescriptorIndexingFeatures.runtimeDescriptorArray = true;

		memset( &m_vkMeshShaderFeatures, 0, sizeof( m_vkMeshShaderFeatures ) );
		m_vkMeshShaderFeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
		m_vkMeshShaderFeatures.pNext = &m_vkDescriptorIndexingFeatures;

		memset( &m_vkFeatures2, 0, sizeof( m_vkFeatures2 ) );
		m_vkFeatures2.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		m_vkFeatures2.pNext = &m_vkMeshShaderFeatures;
		vfs::vkGetPhysicalDeviceFeatures2( m_vkPhysicalDevice, &m_vkFeatures2 );
	}

	// VkSurfaceCapabilitiesKHR
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_vkPhysicalDevice, vkSurface, &m_vkSurfaceCapabilities );
	if ( VK_SUCCESS != result ) {
		printf( "unable to get physical device surface capabilities\n" );
		assert( 0 );
		return false;
	}

	// VkSurfaceFormatKHR
	{
		uint32_t numFormats;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR( m_vkPhysicalDevice, vkSurface, &numFormats, NULL );
		if ( VK_SUCCESS != result || 0 == numFormats ) {
			printf( "Unable to get surface formats\n" );
			assert( 0 );
			return false;
		}

		m_vkSurfaceFormats.resize( numFormats );
		result = vkGetPhysicalDeviceSurfaceFormatsKHR( m_vkPhysicalDevice, vkSurface, &numFormats, m_vkSurfaceFormats.data() );
		if ( VK_SUCCESS != result || 0 == numFormats ) {
			printf( "Unable to get surface formats\n" );
			assert( 0 );
			return false;
		}
	}

	// VkPresentModeKHR
	{
		uint32_t numPresentModes;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR( m_vkPhysicalDevice, vkSurface, &numPresentModes, NULL );
		if ( VK_SUCCESS != result || 0 == numPresentModes ) {
			printf( "Unable to get surface formats\n" );
			assert( 0 );
			return false;
		}

		m_vkPresentModes.resize( numPresentModes );
		result = vkGetPhysicalDeviceSurfacePresentModesKHR( m_vkPhysicalDevice, vkSurface, &numPresentModes, m_vkPresentModes.data() );
		if ( VK_SUCCESS != result || 0 == numPresentModes ) {
			printf( "Unable to get surface formats\n" );
			assert( 0 );
			return false;
		}
	}

	// VkQueueFamilyProperties
	{
		uint32_t numQueues = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( m_vkPhysicalDevice, &numQueues, NULL );
		if ( 0 == numQueues ) {
			printf( "no devices queues available\n" );
			assert( 0 );
			return false;
		}

		m_vkQueueFamilyProperties.resize( numQueues );
		vkGetPhysicalDeviceQueueFamilyProperties( m_vkPhysicalDevice, &numQueues, m_vkQueueFamilyProperties.data() );
		if ( 0 == numQueues ) {
			printf( "no devices queues available\n" );
			assert( 0 );
			return false;
		}
	}

	// VkExtensionProperties
	{
		uint32_t numExtensions;
		result = vkEnumerateDeviceExtensionProperties( m_vkPhysicalDevice, NULL, &numExtensions, NULL );
		if ( VK_SUCCESS != result || 0 == numExtensions ) {
			printf( "unable to enumerate device extensions\n" );
			assert( 0 );
			return false;
		}

		m_vkExtensionProperties.resize( numExtensions );
		result = vkEnumerateDeviceExtensionProperties( m_vkPhysicalDevice, NULL, &numExtensions, m_vkExtensionProperties.data() );
		if ( VK_SUCCESS != result || 0 == numExtensions ) {
			printf( "unable to enumerate device extensions\n" );
			assert( 0 );
			return false;
		}
	}

	return true;
}

/*
====================================================
PhysicalDeviceProperties::HasExtensions
====================================================
*/
bool PhysicalDeviceProperties::HasExtensions( const char ** extensions, const int num ) const {
	for ( int i = 0; i < num; i++ ) {
		const char * extension = extensions[ i ];

		bool doesExist = false;
		for ( int j = 0; j < m_vkExtensionProperties.size(); j++ ) {
			if ( 0 == strcmp( extension, m_vkExtensionProperties[ j ].extensionName ) ) {
				doesExist = true;
				break;
			}
		}

		if ( !doesExist ) {
			return false;
		}
	}

	return true;
}

/*
================================================================================================

DeviceContext

================================================================================================
*/
const std::vector< const char * > DeviceContext::m_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
 	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
// #if defined ( ENABLE_RAYTRACING )
// 	VK_NV_RAY_TRACING_EXTENSION_NAME,
// #endif
// 	VK_NV_MESH_SHADER_EXTENSION_NAME,
};


// const std::vector< const char * > DeviceContext::m_validationLayers = {
// 	"VK_LAYER_LUNARG_standard_validation"
// };

/*
====================================================
VulkanErrorMessage
====================================================
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanErrorMessage( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData ) {
	printf( "ERROR: Vulkan validation layer message: %s\n", msg );
	assert( 0 );

	return VK_FALSE;
}

/*
====================================================
DeviceContext::CheckValidationLayerSupport
====================================================
*/
bool DeviceContext::CheckValidationLayerSupport() const {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

	for ( int i = 0; i < m_validationLayers.size(); i++ ) {
		const char * layerName = m_validationLayers[ i ];

		bool layerFound = false;

		for ( const VkLayerProperties & layerProperties : availableLayers ) {
			if ( 0 == strcmp( layerName, layerProperties.layerName ) ) {
				layerFound = true;
				break;
			}
		}

		if ( !layerFound ) {
			return false;
		}
	}

	return true;
}

/*
====================================================
DeviceContext::CreateInstance
====================================================
*/
bool DeviceContext::CreateInstance( bool enableLayers, const std::vector< const char * > & extensions_required ) {
	VkResult result;

	m_enableLayers = enableLayers;

	std::vector< const char * > extensions = extensions_required;
	extensions.push_back( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );

	std::vector< const char * > validationLayers;
	if ( m_enableLayers ) {
		validationLayers = m_validationLayers;

		uint32_t numLayers;
		vkEnumerateInstanceLayerProperties( &numLayers, nullptr );

		std::vector< VkLayerProperties > layerProperties( numLayers );
		vkEnumerateInstanceLayerProperties( &numLayers, layerProperties.data() );

		for ( int i = 0; i < numLayers; i++ ) {
			printf( "Layer: %i %s\n", i, layerProperties[ i ].layerName );

			if ( 0 == strcmp( "VK_LAYER_KHRONOS_validation", layerProperties[ i ].layerName ) ) {
				validationLayers.clear();
				validationLayers.push_back( "VK_LAYER_KHRONOS_validation" );
				break;
			}
			if ( 0 == strcmp( "VK_LAYER_LUNARG_standard_validation", layerProperties[ i ].layerName ) ) {
				validationLayers.push_back( "VK_LAYER_LUNARG_standard_validation" );
			}
		}
	}
	m_validationLayers = validationLayers;

	//
	//	Vulkan Instance
	//
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Physics Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
//	appInfo.apiVersion = VK_API_VERSION_1_0;
//	appInfo.apiVersion = VK_HEADER_VERSION_COMPLETE;
	appInfo.apiVersion = VK_MAKE_VERSION( 1, 1, VK_HEADER_VERSION );

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();

	result = vkCreateInstance( &createInfo, nullptr, &m_vkInstance );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create vulkan instance\n" );
		assert( 0 );
		return false;
	}
	vfs::Link( m_vkInstance );

	if ( m_enableLayers ) {
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = VulkanErrorMessage;

		result = vfs::vkCreateDebugReportCallbackEXT( m_vkInstance, &createInfo, nullptr, &m_vkDebugCallback );
		if ( VK_SUCCESS != result ) {
			printf( "ERROR: Failed to create debug callback\n" );
			assert( 0 );
			return false;
		}
	}

	return true;
}

/*
====================================================
DeviceContext::Cleanup
====================================================
*/
void DeviceContext::Cleanup() {
	m_swapChain.Cleanup( this );

	// Destroy Command Buffers
	vkFreeCommandBuffers( m_vkDevice, m_vkCommandPool, static_cast< uint32_t >( m_vkCommandBuffers.size() ), m_vkCommandBuffers.data() );
	vkDestroyCommandPool( m_vkDevice, m_vkCommandPool, nullptr );

	vkDestroyDevice( m_vkDevice, nullptr );	

	if ( m_enableLayers ) {
		vfs::vkDestroyDebugReportCallbackEXT( m_vkInstance, m_vkDebugCallback, nullptr );
	}
	vkDestroySurfaceKHR( m_vkInstance, m_vkSurface, nullptr );
	vkDestroyInstance( m_vkInstance, nullptr );
}

/*
====================================================
DeviceContext::CreateDevice
====================================================
*/
bool DeviceContext::CreateDevice() {
	if ( !CreatePhysicalDevice() ) {
		printf( "ERROR: Failed to create physical device\n" );
		assert( 0 );
		return false;
	}

	if ( !CreateLogicalDevice() ) {
		printf( "ERROR: Failed to create logical device\n" );
		assert( 0 );
		return false;
	}

	return true;
}

const char * VendorStr( unsigned int vendorID ) {
	if ( 0x1002 == vendorID ) {
		return "AMD";
	}
	if ( 0x1010 == vendorID ) {
		return "ImgTec";
	}
	if ( 0x10DE == vendorID ) {
		return "NVIDIA";
	}
	if ( 0x13B5 == vendorID ) {
		return "ARM";
	}
	if ( 0x5143 == vendorID ) {
		return "Qualcomm";
	}
	if ( 0x8086 == vendorID ) {
		return "INTEL";
	}
	return "";
}

/*
====================================================
DeviceContext::CreatePhysicalDevice
====================================================
*/
bool DeviceContext::CreatePhysicalDevice() {
	VkResult result;

	//
	//	Enumerate physical devices
	//
	{
		// Get the number of devices
		uint32_t numDevices = 0;
		result = vkEnumeratePhysicalDevices( m_vkInstance, &numDevices, NULL );
		if ( VK_SUCCESS != result || 0 == numDevices ) {
			printf( "ERROR: Failed to enumerate devices\n" );
			assert( 0 );
			return false;
		}

		// Query a list of devices
		std::vector< VkPhysicalDevice > physicalDevices( numDevices );
		result = vkEnumeratePhysicalDevices( m_vkInstance, &numDevices, physicalDevices.data() );
		if ( VK_SUCCESS != result || 0 == numDevices ) {
			printf( "ERROR: Failed to enumerate devices\n" );
			assert( 0 );
			return false;
		}

		// Acquire the properties of each device
		m_physicalDevices.resize( physicalDevices.size() );
		for ( uint32_t i = 0; i < physicalDevices.size(); i++ ) {
			m_physicalDevices[ i ].AcquireProperties( physicalDevices[ i ], m_vkSurface );
		}
	}	

	//
	//	Select a physical device
	//
	//for ( int i = 0; i < m_physicalDevices.size(); i++ ) {
	for ( int i = m_physicalDevices.size() - 1; i >= 0; i-- ) {
		const PhysicalDeviceProperties & deviceProperties = m_physicalDevices[ i ];

		printf( "===========================================\n" );
		printf( "Potential Device: %i\n", i );
		printf( "Physical Device %s\n", deviceProperties.m_vkDeviceProperties.deviceName );
// 		printf( "API Version: %i.%i.%i\n", apiMajor, apiMinor, apiPatch );
// 		printf( "Driver Version: %i.%i.%i\n", driverMajor, driverMinor, driverPatch );
		printf( "Vendor ID: %i  %s\n", deviceProperties.m_vkDeviceProperties.vendorID, VendorStr( deviceProperties.m_vkDeviceProperties.vendorID ) );
		printf( "Device ID: %i\n", deviceProperties.m_vkDeviceProperties.deviceID );
		printf( "Device Max Work Group Size %i %i %i\n",
			deviceProperties.m_vkDeviceProperties.limits.maxComputeWorkGroupSize[ 0 ],
			deviceProperties.m_vkDeviceProperties.limits.maxComputeWorkGroupSize[ 1 ],
			deviceProperties.m_vkDeviceProperties.limits.maxComputeWorkGroupSize[ 2 ]
		);
		printf( "===========================================\n" );

		if ( 0 != i ) {	// HACK: this is to prevent us from picking our amd gpu
			continue;
		}

		// Ignore non-drawing devices
		if ( 0 == deviceProperties.m_vkPresentModes.size() ) {
			printf( "Device: NO present modes\n" );
			continue;
		}
		if ( 0 == deviceProperties.m_vkSurfaceFormats.size() ) {
			printf( "Device: NO surface formats\n" );
			continue;
		}

		// Verify required extension support
		if ( !deviceProperties.HasExtensions( (const char **)m_deviceExtensions.data(), (int)m_deviceExtensions.size() ) ) {
			printf( "Device: NO extensions\n" );
			continue;
		}

		//
		//	Find graphics queue family
		//
		int graphicsIdx = -1;
		for ( int j = 0; j < deviceProperties.m_vkQueueFamilyProperties.size(); ++j ) {
			const VkQueueFamilyProperties & props = deviceProperties.m_vkQueueFamilyProperties[ j ];

			if ( props.queueCount == 0 ) {
				continue;
			}

			if ( props.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
				graphicsIdx = j;
				break;
			}
		}

		if ( graphicsIdx < 0 ) {
			// Device does not support graphics
			printf( "Device: NO graphics support\n" );
			continue;
		}

		//
		//	Find present queue family
		//
		int presentIdx = -1;
		for ( int j = 0; j < deviceProperties.m_vkQueueFamilyProperties.size(); ++j ) {
			const VkQueueFamilyProperties & props = deviceProperties.m_vkQueueFamilyProperties[ j ];

			if ( props.queueCount == 0 ) {
				continue;
			}

			VkBool32 supportsPresentQueue = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( deviceProperties.m_vkPhysicalDevice, j, m_vkSurface, &supportsPresentQueue );
			if ( supportsPresentQueue ) {
				presentIdx = j;
				break;
			}
		}

		if ( presentIdx < 0 ) {
			// Device does not support presentation
			printf( "Device: NO presentation supported\n" );
			continue;
		}

		//
		//	Use first device that supports graphics and presentation
		//
		m_graphicsFamilyIdx = graphicsIdx;
		m_presentFamilyIdx = presentIdx;
		m_vkPhysicalDevice = deviceProperties.m_vkPhysicalDevice;
		m_deviceIndex = i;

		uint32_t apiMajor = VK_VERSION_MAJOR( deviceProperties.m_vkDeviceProperties.apiVersion );
		uint32_t apiMinor = VK_VERSION_MINOR( deviceProperties.m_vkDeviceProperties.apiVersion );
		uint32_t apiPatch = VK_VERSION_PATCH( deviceProperties.m_vkDeviceProperties.apiVersion );

		uint32_t driverMajor = VK_VERSION_MAJOR( deviceProperties.m_vkDeviceProperties.driverVersion );
		uint32_t driverMinor = VK_VERSION_MINOR( deviceProperties.m_vkDeviceProperties.driverVersion );
		uint32_t driverPatch = VK_VERSION_PATCH( deviceProperties.m_vkDeviceProperties.driverVersion );

		printf( "Physical Device Chosen: %s\n", deviceProperties.m_vkDeviceProperties.deviceName );
		printf( "API Version: %i.%i.%i\n", apiMajor, apiMinor, apiPatch );
		printf( "Driver Version: %i.%i.%i\n", driverMajor, driverMinor, driverPatch );
		printf( "Vendor ID: %i  %s\n", deviceProperties.m_vkDeviceProperties.vendorID, VendorStr( deviceProperties.m_vkDeviceProperties.vendorID ) );
		printf( "Device ID: %i\n", deviceProperties.m_vkDeviceProperties.deviceID );
		printf( "Device Max Work Group Size %i %i %i\n",
			deviceProperties.m_vkDeviceProperties.limits.maxComputeWorkGroupSize[ 0 ],
			deviceProperties.m_vkDeviceProperties.limits.maxComputeWorkGroupSize[ 1 ],
			deviceProperties.m_vkDeviceProperties.limits.maxComputeWorkGroupSize[ 2 ]
		);
		return true;
	}

	printf( "ERROR: Failed to create physical device\n" );
	assert( 0 );
	return false;
}

/*
====================================================
DeviceContext::CreateLogicalDevice
====================================================
*/
bool DeviceContext::CreateLogicalDevice() {
	VkResult result;

	std::vector< const char * > validationLayers;
	if ( m_enableLayers ) {
		validationLayers = m_validationLayers;
	}

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< int > uniqueQueueFamilies = {
		m_graphicsFamilyIdx,
		m_presentFamilyIdx
	};

	float queuePriority = 1.0f;
	for ( int queueFamily : uniqueQueueFamilies ) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back( queueCreateInfo );
	}

	const PhysicalDeviceProperties & deviceProperties = m_physicalDevices[ m_deviceIndex ];

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures = deviceProperties.m_vkFeatures;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	//createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.pEnabledFeatures = NULL;	// Use the features2 pNext chain instead of the old way (this allows us to enable all extension features)
	createInfo.pNext = &deviceProperties.m_vkFeatures2;
	createInfo.enabledExtensionCount = (uint32_t)m_deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
	createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();

	result = vkCreateDevice( m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create logical device\n" );
		assert( 0 );
		return false;
	}

	vkGetDeviceQueue( m_vkDevice, m_graphicsFamilyIdx, 0, &m_vkGraphicsQueue );
	vkGetDeviceQueue( m_vkDevice, m_presentFamilyIdx, 0, &m_vkPresentQueue );

	return true;
}

/*
====================================================
DeviceContext::FindMemoryType
====================================================
*/
uint32_t DeviceContext::FindMemoryTypeIndex( uint32_t typeFilter, VkMemoryPropertyFlags properties ) {
	VkPhysicalDeviceMemoryProperties memProperties = m_physicalDevices[ m_deviceIndex ].m_vkMemoryProperties;

	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ ) {
		if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & properties ) == properties ) {
			return i;
		}
	}

	printf( "ERROR: Failed to find memory type index\n" );
	assert( 0 );
	return 0;
}

/*
====================================================
DeviceContext::CreateCommandBuffers
====================================================
*/
bool DeviceContext::CreateCommandBuffers() {
	VkResult result;

	// Command Pool
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_graphicsFamilyIdx;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		result = vkCreateCommandPool( m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool );
		if ( VK_SUCCESS != result ) {
			printf( "ERROR: Failed to create command pool\n" );
			assert( 0 );
			return false;
		}
	}

	// Command Buffers
	{
		const int numBuffers = 16;
		m_vkCommandBuffers.resize( numBuffers );

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_vkCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

		result = vkAllocateCommandBuffers( m_vkDevice, &allocInfo, m_vkCommandBuffers.data() );
		if ( VK_SUCCESS != result ) {
			printf( "ERROR: Failed to allocate command buffers\n" );
			assert( 0 );
			return false;
		}
	}

	return true;
}

/*
====================================================
DeviceContext::CreateCommandBuffer
====================================================
*/
VkCommandBuffer DeviceContext::CreateCommandBuffer( VkCommandBufferLevel level, bool begin ) {
	VkResult result;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	result = vkAllocateCommandBuffers( m_vkDevice, &allocInfo, &cmdBuffer );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create command buffer\n" );
		assert( 0 );
	}

	// If requested, also start recording for the new command buffer
	if ( begin ) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		result = vkBeginCommandBuffer( cmdBuffer, &beginInfo );
		if ( VK_SUCCESS != result ) {
			printf( "ERROR: Failed to begin command buffer\n" );
			assert( 0 );
		}
	}
	return cmdBuffer;
}

/*
====================================================
DeviceContext::FlushCommandBuffer
====================================================
*/
void DeviceContext::FlushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, bool free ) {
	if ( VK_NULL_HANDLE == commandBuffer ) {
		return;
	}

	vkEndCommandBuffer( commandBuffer );

	Fence fence;
	fence.Create( this );
	{
		// Submit to the queue
		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit( queue, 1, &submitInfo, fence.m_vkFence );
	}
	fence.Wait( this );

	if ( free ) {
		vkFreeCommandBuffers( m_vkDevice, m_vkCommandPool, 1, &commandBuffer );
	}
}

/*
====================================================
DeviceContext::GetAlignedUniformByteOffset
Devices have a minimum byte alignment for their offsets.
This function converts the incoming byte offset to a proper byte alignment.
====================================================
*/
int DeviceContext::GetAlignedUniformByteOffset( const int offset ) const {
	const PhysicalDeviceProperties & deviceProperties = m_physicalDevices[ m_deviceIndex ];
	const int minByteOffsetAlignment = deviceProperties.m_vkDeviceProperties.limits.minUniformBufferOffsetAlignment;

	const int n = ( offset + minByteOffsetAlignment - 1 ) / minByteOffsetAlignment;
	const int alignedOffset = n * minByteOffsetAlignment;

	//const int alignedOffset = ( offset + minByteOffsetAlignment - 1 ) & ~minByteOffsetAlignment;	// This will work if minByteOffsetAlignment is always power of two
	return alignedOffset;
}

/*
====================================================
DeviceContext::GetAlignedStorageByteOffset
Devices have a minimum byte alignment for their offsets.
This function converts the incoming byte offset to a proper byte alignment.
====================================================
*/
int DeviceContext::GetAlignedStorageByteOffset( const int offset ) const {
	const PhysicalDeviceProperties & deviceProperties = m_physicalDevices[ m_deviceIndex ];
	const int minByteOffsetAlignment = deviceProperties.m_vkDeviceProperties.limits.minStorageBufferOffsetAlignment;

	const int n = ( offset + minByteOffsetAlignment - 1 ) / minByteOffsetAlignment;
	const int alignedOffset = n * minByteOffsetAlignment;

	//const int alignedOffset = ( offset + minByteOffsetAlignment - 1 ) & ~minByteOffsetAlignment;	// This will work if minByteOffsetAlignment is always power of two
	return alignedOffset;
}