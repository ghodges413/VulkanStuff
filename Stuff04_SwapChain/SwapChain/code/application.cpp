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
Application::ChooseSwapSurfaceFormat
====================================================
*/
VkSurfaceFormatKHR Application::ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> & availableFormats ) {
	if ( availableFormats.size() == 1 && availableFormats[ 0 ].format == VK_FORMAT_UNDEFINED ) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for ( const VkSurfaceFormatKHR & availableFormat : availableFormats ) {
		if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
			return availableFormat;
		}
	}

	return availableFormats[ 0 ];
}

/*
====================================================
Application::ChooseSwapPresentMode
====================================================
*/
VkPresentModeKHR Application::ChooseSwapPresentMode( const std::vector< VkPresentModeKHR > availablePresentModes ) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for ( const VkPresentModeKHR & availablePresentMode : availablePresentModes ) {
		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
			return availablePresentMode;
		} else if ( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

/*
====================================================
Application::ChooseSwapExtent
====================================================
*/
VkExtent2D Application::ChooseSwapExtent( const VkSurfaceCapabilitiesKHR & capabilities, uint32_t width, uint32_t height ) {
	if ( capabilities.currentExtent.width != std::numeric_limits< uint32_t >::max() ) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = {
			static_cast< uint32_t >( width ),
			static_cast< uint32_t >( height )
		};

		actualExtent.width = std::max( capabilities.minImageExtent.width, std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
		actualExtent.height = std::max( capabilities.minImageExtent.height, std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

		return actualExtent;
	}
}

/*
====================================================
Application::FindMemoryType
====================================================
*/
uint32_t Application::FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &memProperties );

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

	int width;
	int height;
	glfwGetWindowSize( m_glfwWindow, &width, &height );

	//
	//	Create SwapChain
	//
	{
		SwapChainSupportDetails swapChainSupport = Application::QuerySwapChainSupport( m_vkPhysicalDevice, m_vkSurface );

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats );
		VkPresentModeKHR presentMode = ChooseSwapPresentMode( swapChainSupport.presentModes );
		m_vkSwapChainExtent = ChooseSwapExtent( swapChainSupport.capabilities, width, height );

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount ) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_vkSurface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = m_vkSwapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = Application::FindQueueFamilies( m_vkPhysicalDevice, m_vkSurface );
		uint32_t queueFamilyIndices[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentFamily
		};

		if ( indices.graphicsFamily != indices.presentFamily ) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		if ( VK_SUCCESS != vkCreateSwapchainKHR( m_vkDevice, &createInfo, nullptr, &m_vkSwapChain ) ) {
			printf( "failed to create swap chain!" );
			assert( 0 );
			return false;
		}

		vkGetSwapchainImagesKHR( m_vkDevice, m_vkSwapChain, &imageCount, nullptr );
		m_vkSwapChainColorImages.resize( imageCount );
		vkGetSwapchainImagesKHR( m_vkDevice, m_vkSwapChain, &imageCount, m_vkSwapChainColorImages.data() );

		m_vkSwapChainColorImageFormat = surfaceFormat.format;

		//
		//	Create Image Views for swap chain
		//
		{
			m_vkSwapChainImageViews.resize( m_vkSwapChainColorImages.size() );

			for ( uint32_t i = 0; i < m_vkSwapChainColorImages.size(); i++ ) {
				VkImageViewCreateInfo viewInfo = {};
				viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewInfo.image = m_vkSwapChainColorImages[ i ];
				viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewInfo.format = m_vkSwapChainColorImageFormat;
				viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewInfo.subresourceRange.baseMipLevel = 0;
				viewInfo.subresourceRange.levelCount = 1;
				viewInfo.subresourceRange.baseArrayLayer = 0;
				viewInfo.subresourceRange.layerCount = 1;

				if ( VK_SUCCESS != vkCreateImageView( m_vkDevice, &viewInfo, nullptr, &m_vkSwapChainImageViews[ i ] ) ) {
					printf( "failed to create texture image view!\n" );
					assert( 0 );
					return false;
				}
			}
		}

		//
		//	Create Depth Image and Depth Image View for swap chain
		//
		{
			// Choose Depth Image Format
			m_vkSwapChainDepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
			{
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties( m_vkPhysicalDevice, VK_FORMAT_D32_SFLOAT, &props );
				if ( 0 != ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ) ) {
					m_vkSwapChainDepthFormat = VK_FORMAT_D32_SFLOAT;
				}
			}

			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = m_vkSwapChainDepthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if ( VK_SUCCESS != vkCreateImage( m_vkDevice, &imageInfo, nullptr, &m_vkSwapChainDepthImage ) ) {
				printf( "failed to create image!\n" );
				assert( 0 );
				return false;
			}

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements( m_vkDevice, m_vkSwapChainDepthImage, &memRequirements );

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

			if ( VK_SUCCESS != vkAllocateMemory( m_vkDevice, &allocInfo, nullptr, &m_vkSwapChainDepthImageMemory ) ) {
				printf( "failed to allocate image memory!\n" );
				assert( 0 );
				return false;
			}

			vkBindImageMemory( m_vkDevice, m_vkSwapChainDepthImage, m_vkSwapChainDepthImageMemory, 0 );

			//
			//	Create depth image view
			//
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_vkSwapChainDepthImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_vkSwapChainColorImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if ( VK_SUCCESS != vkCreateImageView( m_vkDevice, &viewInfo, nullptr, &m_vkSwapChainDepthImageView ) ) {
				printf( "failed to create texture image view!\n" );
				assert( 0 );
				return false;
			}
		}

		//
		//	Create the render pass for the swap chain
		//
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = m_vkSwapChainColorImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = m_vkSwapChainDepthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			std::array< VkAttachmentDescription, 2 > attachments = {
				colorAttachment,
				depthAttachment
			};
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast< uint32_t >( attachments.size() );
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			if ( VK_SUCCESS != vkCreateRenderPass( m_vkDevice, &renderPassInfo, nullptr, &m_vkRenderPass ) ) {
				printf( "failed to create render pass!\n" );
				assert( 0 );
				return false;
			}
		}

		//
		//	Create Frame Buffers for SwapChain
		//
		{
			m_vkSwapChainFramebuffers.resize( m_vkSwapChainImageViews.size() );

			for ( size_t i = 0; i < m_vkSwapChainImageViews.size(); i++ ) {
				std::array< VkImageView, 2 > attachments = {
					m_vkSwapChainImageViews[ i ],
					m_vkSwapChainDepthImageView
				};

				VkFramebufferCreateInfo framebufferInfo = {};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = m_vkRenderPass;
				framebufferInfo.attachmentCount = static_cast< uint32_t >( attachments.size() );
				framebufferInfo.pAttachments = attachments.data();
				framebufferInfo.width = m_vkSwapChainExtent.width;
				framebufferInfo.height = m_vkSwapChainExtent.height;
				framebufferInfo.layers = 1;

				if ( VK_SUCCESS != vkCreateFramebuffer( m_vkDevice, &framebufferInfo, nullptr, &m_vkSwapChainFramebuffers[ i ] ) ) {
					printf( "failed to create framebuffer!\n" );
					assert( 0 );
					return false;
				}
			}
		}
	}

	return true;
}

/*
====================================================
Application::Cleanup
====================================================
*/
void Application::Cleanup() {
	// Destroy swap chain related data
	vkDestroyImageView( m_vkDevice, m_vkSwapChainDepthImageView, nullptr );
	vkDestroyImage( m_vkDevice, m_vkSwapChainDepthImage, nullptr );
	vkFreeMemory( m_vkDevice, m_vkSwapChainDepthImageMemory, nullptr );
	vkDestroyRenderPass( m_vkDevice, m_vkRenderPass, nullptr );

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
