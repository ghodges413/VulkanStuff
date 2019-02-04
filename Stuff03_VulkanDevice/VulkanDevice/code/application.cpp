//
//  application.cpp
//
#include "application.h"
#include <assert.h>


const std::vector< const char * > Application::m_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector< const char * > Application::m_ValidationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

/*
====================================================
CreateDebugReportCallbackEXT
====================================================
*/
VkResult CreateDebugReportCallbackEXT( VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback ) {
	PFN_vkCreateDebugReportCallbackEXT functor = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr( instance, "vkCreateDebugReportCallbackEXT" );

	if ( functor != nullptr ) {
		return functor( instance, pCreateInfo, pAllocator, pCallback );
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/*
====================================================
DestroyDebugReportCallbackEXT
====================================================
*/
void DestroyDebugReportCallbackEXT( VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator ) {
	PFN_vkDestroyDebugReportCallbackEXT functor = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" );

	if ( functor != nullptr ) {
		functor( instance, callback, pAllocator );
	}
}

/*
========================================================================================================

Application

========================================================================================================
*/

/*
====================================================
Application::Application
====================================================
*/
Application::Application() {
	InitializeGLFW();
	InitializeVulkan();
}

/*
====================================================
Application::~Application
====================================================
*/
Application::~Application() {
	Cleanup();
}

/*
====================================================
Application::InitializeGLFW
====================================================
*/
void Application::InitializeGLFW() {
	glfwInit();

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

	m_glfwWindow = glfwCreateWindow( WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr );

	glfwSetWindowUserPointer( m_glfwWindow, this );
	glfwSetWindowSizeCallback( m_glfwWindow, Application::OnWindowResized );
}

/*
====================================================
Application::CheckValidationLayerSupport
====================================================
*/
bool Application::CheckValidationLayerSupport() const {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

	for ( int i = 0; i < m_ValidationLayers.size(); i++ ) {
		const char * layerName = m_ValidationLayers[ i ];

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
Application::GetGLFWRequiredExtensions
====================================================
*/
std::vector< const char * > Application::GetGLFWRequiredExtensions() const {
	std::vector< const char * > extensions;

	const char ** glfwExtensions;
	uint32_t glfwExtensionCount = 0;
	glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	for ( uint32_t i = 0; i < glfwExtensionCount; i++ ) {
		extensions.push_back( glfwExtensions[ i ] );
	}

	if ( m_enableLayers ) {
		extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
	}

	return extensions;
}


/*
====================================================
Application::CheckDeviceExtensionSupport
====================================================
*/
bool Application::CheckDeviceExtensionSupport( VkPhysicalDevice vkPhysicalDevice ) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &extensionCount, nullptr );

	std::vector<VkExtensionProperties> availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &extensionCount, availableExtensions.data() );

	std::set<std::string> requiredExtensions( m_DeviceExtensions.begin(), m_DeviceExtensions.end() );

	for ( const VkExtensionProperties & extension : availableExtensions ) {
		requiredExtensions.erase( extension.extensionName );
	}

	return requiredExtensions.empty();
}

/*
====================================================
Application::IsDeviceSuitable
====================================================
*/
bool Application::IsDeviceSuitable( VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurface ) {
	QueueFamilyIndices indices = Application::FindQueueFamilies( vkPhysicalDevice, vkSurface );

	bool extensionsSupported = CheckDeviceExtensionSupport( vkPhysicalDevice );

	bool swapChainAdequate = false;
	if ( extensionsSupported ) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( vkPhysicalDevice, vkSurface );
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures( vkPhysicalDevice, &supportedFeatures );

	return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

/*
====================================================
Application::QuerySwapChainSupport
====================================================
*/
SwapChainSupportDetails Application::QuerySwapChainSupport( VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurface ) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vkPhysicalDevice, vkSurface, &details.capabilities );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( vkPhysicalDevice, vkSurface, &formatCount, nullptr );

	if ( 0 != formatCount ) {
		details.formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( vkPhysicalDevice, vkSurface, &formatCount, details.formats.data() );
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( vkPhysicalDevice, vkSurface, &presentModeCount, nullptr );

	if ( 0 != presentModeCount ) {
		details.presentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( vkPhysicalDevice, vkSurface, &presentModeCount, details.presentModes.data() );
	}

	return details;
}

/*
====================================================
Application::FindQueueFamilies
====================================================
*/
QueueFamilyIndices Application::FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR vkSurface ) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

	for ( int i = 0; i < queueFamilies.size(); i++ ) {
		const VkQueueFamilyProperties & queueFamily = queueFamilies[ i ];

		if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR( device, i, vkSurface, &presentSupport );

		if ( queueFamily.queueCount > 0 && presentSupport ) {
			indices.presentFamily = i;
		}

		if ( indices.IsComplete() ) {
			break;
		}
	}

	return indices;
}

/*
====================================================
Application::InitializeVulkan
====================================================
*/
bool Application::InitializeVulkan() {
	if ( m_enableLayers && !CheckValidationLayerSupport() ) {
		printf( "validation layers requested, but not available!" );
		assert( 0 );
		return false;
	}

	//
	//	Vulkan Instance
	//
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Stuff";
		appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.pEngineName = "Experimental";
		appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		std::vector< const char * > extensions = GetGLFWRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast< uint32_t >( extensions.size() );
		createInfo.ppEnabledExtensionNames = extensions.data();

		if ( m_enableLayers ) {
			createInfo.enabledLayerCount = static_cast< uint32_t >( m_ValidationLayers.size() );
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if ( vkCreateInstance( &createInfo, nullptr, &m_vkInstance ) != VK_SUCCESS ) {
			printf( "failed to create instance!" );
			assert( 0 );
			return false;
		}

		if ( m_enableLayers ) {
			VkDebugReportCallbackCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			createInfo.pfnCallback = DebugCallback;

			if ( CreateDebugReportCallbackEXT( m_vkInstance, &createInfo, nullptr, &m_vkDebugCallback ) != VK_SUCCESS ) {
				printf( "failed to set up debug callback!" );
				assert( 0 );
				return false;
			}
		}
	}

	//
	//	Vulkan Surface for GLFW Window
	//
	if ( glfwCreateWindowSurface( m_vkInstance, m_glfwWindow, nullptr, &m_vkSurface ) != VK_SUCCESS ) {
		printf( "failed to create window surface!" );
		assert( 0 );
		return false;
	}

	//
	//	Vulkan Physical Device
	//
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices( m_vkInstance, &deviceCount, nullptr );

		if ( 0 == deviceCount ) {
			printf( "failed to find GPUs with Vulkan support!\n" );
			assert( 0 );
			return false;
		}

		std::vector< VkPhysicalDevice > devices( deviceCount );
		vkEnumeratePhysicalDevices( m_vkInstance, &deviceCount, devices.data() );

		for ( const VkPhysicalDevice & device : devices ) {
			if ( IsDeviceSuitable( device, m_vkSurface ) ) {
				m_vkPhysicalDevice = device;
				break;
			}
		}

		if ( VK_NULL_HANDLE == m_vkPhysicalDevice ) {
			printf( "failed to find a suitable GPU\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Vulkan Logical Device
	//
	{
		QueueFamilyIndices indices = FindQueueFamilies( m_vkPhysicalDevice, m_vkSurface );

		std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
		std::set< int > uniqueQueueFamilies = {
			indices.graphicsFamily,
			indices.presentFamily
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

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>( m_DeviceExtensions.size() );
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		if ( m_enableLayers ) {
			createInfo.enabledLayerCount = static_cast<uint32_t>( m_ValidationLayers.size() );
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if ( VK_SUCCESS != vkCreateDevice( m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice ) ) {
			printf( "failed to create logical device!" );
			assert( 0 );
			return false;
		}

		vkGetDeviceQueue( m_vkDevice, indices.graphicsFamily, 0, &m_vkGraphicsQueue );
		vkGetDeviceQueue( m_vkDevice, indices.presentFamily, 0, &m_vkPresentQueue );
	}

	return true;
}

/*
====================================================
Application::Cleanup
====================================================
*/
void Application::Cleanup() {
	// Destroy Vulkan Device
	vkDestroyDevice( m_vkDevice, nullptr );

	// Destroy Vulkan Instance
	DestroyDebugReportCallbackEXT( m_vkInstance, m_vkDebugCallback, nullptr );
	vkDestroySurfaceKHR( m_vkInstance, m_vkSurface, nullptr );
	vkDestroyInstance( m_vkInstance, nullptr );

	// Destroy GLFW
	glfwDestroyWindow( m_glfwWindow );
	glfwTerminate();
}

/*
====================================================
Application::OnWindowResized
====================================================
*/
void Application::OnWindowResized( GLFWwindow * window, int width, int height ) {
	if ( 0 == width || 0 == height ) {
		return;
	}

//	Application * app = reinterpret_cast< Application * >( glfwGetWindowUserPointer( window ) );
}

/*
====================================================
Application::DebugCallback
====================================================
*/
VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData ) {
	printf( "Validation layer: %s\n", msg );
	assert( 0 );

	return VK_FALSE;
}

/*
====================================================
Application::MainLoop
====================================================
*/
void Application::MainLoop() {
	while ( !glfwWindowShouldClose( m_glfwWindow ) ) {
		glfwPollEvents();
	}
}
