//
//  application.cpp
//
#include "application.h"
#include "Texture.h"
#include "model.h"
#include "shader.h"
#include "Fileio.h"
#include <assert.h>

Application * g_application = NULL;

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
Application::Initialize
====================================================
*/
void Application::Initialize() {
	InitializeGLFW();
	InitializeVulkan();

	Entity_t entity;
	entity.pos = Vec3d( 0, 0, 0 );
	entity.fwd = Vec3d( 1, 0, 0 );
	entity.up = Vec3d( 0, 0, 1 );
	m_entities.push_back( entity );

	entity.pos = Vec3d( 0, 4, 0 );
	entity.fwd = Vec3d( 1, 0, 0 );
	entity.up = Vec3d( 0, 0, 1 );
	m_entities.push_back( entity );

	entity.pos = Vec3d( 0,-4, 0 );
	entity.fwd = Vec3d( 1, 0, 0 );
	entity.up = Vec3d( 0, 0, 1 );
	m_entities.push_back( entity );
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
Application::CreateSwapChain
====================================================
*/
bool Application::CreateSwapChain( int windowWidth, int windowHeight ) {
	SwapChainSupportDetails swapChainSupport = Application::QuerySwapChainSupport( m_vkPhysicalDevice, m_vkSurface );

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats );
	VkPresentModeKHR presentMode = ChooseSwapPresentMode( swapChainSupport.presentModes );
	m_vkSwapChainExtent = ChooseSwapExtent( swapChainSupport.capabilities, windowWidth, windowHeight );

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
		imageInfo.extent.width = windowWidth;
		imageInfo.extent.height = windowHeight;
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

	return true;
}

/*
====================================================
Application::CreatePipeline
====================================================
*/
bool Application::CreatePipeline( int windowWidth, int windowHeight ) {
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_vkShaderModuleVert;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_vkShaderModuleFrag;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkVertexInputBindingDescription bindingDescription = vert_t::GetBindingDescription();
	std::array< VkVertexInputAttributeDescription, 5 > attributeDescriptions = vert_t::GetAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDescriptions.size() );
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)windowWidth;
	viewport.height = (float)windowHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { (uint32_t)windowWidth, (uint32_t)windowHeight };

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
//	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;	// Vulkan switched its NDC.  It made it right handed, and this flipped the y-direction.  This explains why we had this set
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;	// Vulkan switched its NDC.  It made it right handed, and this flipped the y-direction.  This explains why we had this set
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[ 0 ] = 0.0f;
	colorBlending.blendConstants[ 1 ] = 0.0f;
	colorBlending.blendConstants[ 2 ] = 0.0f;
	colorBlending.blendConstants[ 3 ] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_vkDescriptorSetLayout;

	VkResult result = vkCreatePipelineLayout( m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create pipeline layout!\n" );
		assert( 0 );
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = m_vkPipelineLayout;
	pipelineInfo.renderPass = m_vkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines( m_vkDevice, pipelineCache, 1, &pipelineInfo, nullptr, &m_vkPipeline );
	if ( VK_SUCCESS != result ) {
		printf( "failed to create graphics pipeline!\n" );
		assert( 0 );
		return false;
	}

	return true;
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

	int windowWidth;
	int windowHeight;
	glfwGetWindowSize( m_glfwWindow, &windowWidth, &windowHeight );

	//
	//	Create SwapChain
	//
	{
		if ( !CreateSwapChain( windowWidth, windowHeight ) ) {
			printf( "Failed to correctly build the swapchain" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Uniform Buffer
	//
	{
		m_vkUniformBufferSize = sizeof( float ) * 16 * 128;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_vkUniformBufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if ( VK_SUCCESS != vkCreateBuffer( m_vkDevice, &bufferInfo, nullptr, &m_vkUniformBuffer ) ) {
			printf( "failed to create buffer!\n" );
			assert( 0 );
			return false;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements( m_vkDevice, m_vkUniformBuffer, &memRequirements );

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

		if ( VK_SUCCESS != vkAllocateMemory( m_vkDevice, &allocInfo, nullptr, &m_vkUniformBufferMemory ) ) {
			printf( "failed to allocate buffer memory!\n" );
			assert( 0 );
			return false;
		}

		vkBindBufferMemory( m_vkDevice, m_vkUniformBuffer, m_vkUniformBufferMemory, 0 );
	}

	//
	//	Command Buffers
	//
	{
		// Command Pool
		{
			QueueFamilyIndices queueFamilyIndices = Application::FindQueueFamilies( m_vkPhysicalDevice, m_vkSurface );

			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

			VkResult result = vkCreateCommandPool( m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool );
			if ( VK_SUCCESS != result ) {
				printf( "failed to create graphics command pool!\n" );
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

			VkResult result = vkAllocateCommandBuffers( m_vkDevice, &allocInfo, m_vkCommandBuffers.data() );
			if ( VK_SUCCESS != result ) {
				printf( "failed to allocate command buffers!\n" );
				assert( 0 );
				return false;
			}
		}
	}

	//
	//	Texture loaded from file
	//
	m_texture[ 0 ] = new Texture;
	m_texture[ 1 ] = new Texture;
	m_texture[ 2 ] = new Texture;
	m_texture[ 3 ] = new Texture;
	//m_texture->LoadFromFile( m_vkDevice, m_vkGraphicsQueue, m_vkCommandBuffers[ 0 ], "../common/data/images/companionCube.tga" );
	m_texture[ 0 ]->LoadFromFile( m_vkDevice, m_vkGraphicsQueue, m_vkCommandBuffers[ 0 ], "../common/data/images/shaderBall/paintedMetal_Diffuse.tga" );
	m_texture[ 1 ]->LoadFromFile( m_vkDevice, m_vkGraphicsQueue, m_vkCommandBuffers[ 0 ], "../common/data/images/shaderBall/paintedMetal_Normal.tga" );
	m_texture[ 2 ]->LoadFromFile( m_vkDevice, m_vkGraphicsQueue, m_vkCommandBuffers[ 0 ], "../common/data/images/shaderBall/paintedMetal_Glossiness.tga" );
	m_texture[ 3 ]->LoadFromFile( m_vkDevice, m_vkGraphicsQueue, m_vkCommandBuffers[ 0 ], "../common/data/images/shaderBall/paintedMetal_Specular.tga" );

	//
	//	Model loaded from file
	//
	{
		Model model;
		//const bool didLoad = model.LoadOBJ( "../common/data/meshes/static/companionCube.obj", false );
		const bool didLoad = model.LoadOBJ( "../common/data/meshes/static/shaderBall.obj", true );
		if ( !didLoad ) {
			printf( "Unable to load obj!\n" );
			assert( 0 );
			return false;
		}

		// Create Vertex Buffer
		{
			VkDeviceSize bufferSize = sizeof( model.m_vertices[ 0 ] ) * model.m_vertices.size();

			// Create staging buffers
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if ( VK_SUCCESS != vkCreateBuffer( m_vkDevice, &bufferInfo, nullptr, &stagingBuffer ) ) {
					printf( "failed to create buffer!\n" );
					assert( 0 );
					return false;
				}

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements( m_vkDevice, stagingBuffer, &memRequirements );

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

				if ( VK_SUCCESS != vkAllocateMemory( m_vkDevice, &allocInfo, nullptr, &stagingBufferMemory ) ) {
					printf( "failed to allocate buffer memory!\n" );
					assert( 0 );
					return false;
				}

				vkBindBufferMemory( m_vkDevice, stagingBuffer, stagingBufferMemory, 0 );
			}

			// Copy data into buffers
			void * data = NULL;
			vkMapMemory( m_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data );
			memcpy( data, model.m_vertices.data(), (size_t)bufferSize );
			vkUnmapMemory( m_vkDevice, stagingBufferMemory );

			// Create vertex buffer
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if ( VK_SUCCESS != vkCreateBuffer( m_vkDevice, &bufferInfo, nullptr, &m_vkVertexBuffer ) ) {
					printf( "failed to create buffer!\n" );
					assert( 0 );
					return false;
				}

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements( m_vkDevice, m_vkVertexBuffer, &memRequirements );

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

				if ( VK_SUCCESS != vkAllocateMemory( m_vkDevice, &allocInfo, nullptr, &m_vkVertexBufferMemory ) ) {
					printf( "failed to allocate buffer memory!\n" );
					assert( 0 );
					return false;
				}

				vkBindBufferMemory( m_vkDevice, m_vkVertexBuffer, m_vkVertexBufferMemory, 0 );
			}

			// Copy data from staging buffer into vertex buffer
			{
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkResetCommandBuffer( m_vkCommandBuffers[ 0 ], 0 );
				vkBeginCommandBuffer( m_vkCommandBuffers[ 0 ], &beginInfo );

				VkBufferCopy copyRegion = {};
				copyRegion.size = bufferSize;
				vkCmdCopyBuffer( m_vkCommandBuffers[ 0 ], stagingBuffer, m_vkVertexBuffer, 1, &copyRegion );

				vkEndCommandBuffer( m_vkCommandBuffers[ 0 ] );
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &m_vkCommandBuffers[ 0 ];
				vkQueueSubmit( m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
				vkQueueWaitIdle( m_vkGraphicsQueue );
			}

			vkDestroyBuffer( m_vkDevice, stagingBuffer, nullptr );
			vkFreeMemory( m_vkDevice, stagingBufferMemory, nullptr );
		}

		// Create Index Buffer
		{
			m_numModelIndices = model.m_indices.size();
			VkDeviceSize bufferSize = sizeof( model.m_indices[ 0 ] ) * model.m_indices.size();

			// Create staging buffer
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if ( VK_SUCCESS != vkCreateBuffer( m_vkDevice, &bufferInfo, nullptr, &stagingBuffer ) ) {
					printf( "failed to create buffer!\n" );
					assert( 0 );
					return false;
				}

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements( m_vkDevice, stagingBuffer, &memRequirements );

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

				if ( VK_SUCCESS != vkAllocateMemory( m_vkDevice, &allocInfo, nullptr, &stagingBufferMemory ) ) {
					printf( "failed to allocate buffer memory!\n" );
					assert( 0 );
					return false;
				}

				vkBindBufferMemory( m_vkDevice, stagingBuffer, stagingBufferMemory, 0 );
			}

			// Load data into buffer
			void * data = NULL;
			vkMapMemory( m_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data );
			memcpy( data, model.m_indices.data(), (size_t) bufferSize );
			vkUnmapMemory( m_vkDevice, stagingBufferMemory );

			// Create index buffer
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if ( VK_SUCCESS != vkCreateBuffer( m_vkDevice, &bufferInfo, nullptr, &m_vkIndexBuffer ) ) {
					printf( "failed to create buffer!\n" );
					assert( 0 );
					return false;
				}

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements( m_vkDevice, m_vkIndexBuffer, &memRequirements );

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

				if ( VK_SUCCESS != vkAllocateMemory( m_vkDevice, &allocInfo, nullptr, &m_vkIndexBufferMemory ) ) {
					printf( "failed to allocate buffer memory!\n" );
					assert( 0 );
					return false;
				}

				vkBindBufferMemory( m_vkDevice, m_vkIndexBuffer, m_vkIndexBufferMemory, 0 );
			}

			// Transfer data from staging buffer to index buffer
			{
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkResetCommandBuffer( m_vkCommandBuffers[ 0 ], 0 );
				vkBeginCommandBuffer( m_vkCommandBuffers[ 0 ], &beginInfo );

				VkBufferCopy copyRegion = {};
				copyRegion.size = bufferSize;
				vkCmdCopyBuffer( m_vkCommandBuffers[ 0 ], stagingBuffer, m_vkIndexBuffer, 1, &copyRegion );

				vkEndCommandBuffer( m_vkCommandBuffers[ 0 ] );
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &m_vkCommandBuffers[ 0 ];
				vkQueueSubmit( m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
				vkQueueWaitIdle( m_vkGraphicsQueue );
			}

			vkDestroyBuffer( m_vkDevice, stagingBuffer, nullptr );
			vkFreeMemory( m_vkDevice, stagingBufferMemory, nullptr );
		}
	}

	//
	//	Shader Modules
	//
	{
		const char * name = "textured";
		name = "ggx";

		unsigned char * vertCode = NULL;
		unsigned char * fragCode = NULL;

		unsigned int sizeVert = 0;
		unsigned int sizeFrag = 0;

		char nameVert[ 1024 ];
		char nameFrag[ 1024 ];

		sprintf( nameVert, "../common/data/shaders/%s.vert", name );
		sprintf( nameFrag, "../common/data/shaders/%s.frag", name );

		if ( !GetFileData( nameVert, &vertCode, sizeVert ) ) {
			return false;
		}
		if ( !GetFileData( nameFrag, &fragCode, sizeFrag ) ) {
			return false;
		}

		std::vector< unsigned int > vertSpirv;
		std::vector< unsigned int > fragSpirv;

		if ( !Shader::CompileToSPIRV( Shader::SHADER_STAGE_VERTEX, (char *)vertCode, vertSpirv ) ) {
			free( vertCode );
			return false;
		}
		if ( !Shader::CompileToSPIRV( Shader::SHADER_STAGE_FRAGMENT, (char *)fragCode, fragSpirv ) ) {
			free( fragCode );
			return false;
		}

		m_vkShaderModuleVert = Shader::CreateShaderModule( m_vkDevice, (char *)vertSpirv.data(), (int)( vertSpirv.size() * sizeof( unsigned int ) ) );
		m_vkShaderModuleFrag = Shader::CreateShaderModule( m_vkDevice, (char *)fragSpirv.data(), (int)( fragSpirv.size() * sizeof( unsigned int ) ) );

		free( vertCode );
		free( fragCode );
	}

	//
	//	Descriptor Sets
	//
	{
		// Create Descriptor Pool
		{
			std::array< VkDescriptorPoolSize, 2 > poolSizes = {};
			poolSizes[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[ 0 ].descriptorCount = 2 * m_numDescriptorSets;
			poolSizes[ 1 ].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[ 1 ].descriptorCount = 4 * m_numDescriptorSets;

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast< uint32_t >( poolSizes.size() );
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = 1 * m_numDescriptorSets;

			const VkResult result = vkCreateDescriptorPool( m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool );
			if ( VK_SUCCESS != result ) {
				printf( "failed to create descriptor pool!\n" );
				assert( 0 );
				return false;
			}
		}

		// Create Descriptor Set Layout
		{
			VkDescriptorSetLayoutBinding uboCameraLayoutBinding = {};
			uboCameraLayoutBinding.binding = 0;
			uboCameraLayoutBinding.descriptorCount = 1;
			uboCameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboCameraLayoutBinding.pImmutableSamplers = nullptr;
			uboCameraLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding uboModelLayoutBinding = {};
			uboModelLayoutBinding.binding = 1;
			uboModelLayoutBinding.descriptorCount = 1;
			uboModelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboModelLayoutBinding.pImmutableSamplers = nullptr;
			uboModelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding samplerLayoutBinding[ 4 ] = {};
			for ( int i = 0; i < m_numTextures; i++ ) {
				samplerLayoutBinding[ i ].binding = 2 + i;
				samplerLayoutBinding[ i ].descriptorCount = 1;
				samplerLayoutBinding[ i ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				samplerLayoutBinding[ i ].pImmutableSamplers = nullptr;
				samplerLayoutBinding[ i ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			std::array< VkDescriptorSetLayoutBinding, 2 + 4 > bindings = {
				uboCameraLayoutBinding,
				uboModelLayoutBinding,
				samplerLayoutBinding[ 0 ],
				samplerLayoutBinding[ 1 ],
				samplerLayoutBinding[ 2 ],
				samplerLayoutBinding[ 3 ]
			};
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast< uint32_t >( bindings.size() );
			layoutInfo.pBindings = bindings.data();

			VkResult result = vkCreateDescriptorSetLayout( m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout );
			if ( VK_SUCCESS != result ) {
				printf( "failed to create descriptor set layout!\n" );
				assert( 0 );
				return false;
			}
		}

		// Create Descriptor Sets
		{
			VkDescriptorSetLayout layouts[ m_numDescriptorSets ];
			for ( int i = 0; i < m_numDescriptorSets; i++ ) {
				layouts[ i ] = m_vkDescriptorSetLayout;
			}
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_vkDescriptorPool;
			allocInfo.descriptorSetCount = 1 * m_numDescriptorSets;
			allocInfo.pSetLayouts = layouts;

			VkResult result = vkAllocateDescriptorSets( m_vkDevice, &allocInfo, m_vkDescriptorSets );
			if ( VK_SUCCESS != result ) {
				printf( "failed to allocate descriptor set!\n" );
				assert( 0 );
				return false;
			}
		}
	}

	//
	//	Pipeline State
	//
	{
		if ( !CreatePipeline( windowWidth, windowHeight ) ) {
			printf( "Failed to create pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Semaphores
	//
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkResult result = vkCreateSemaphore( m_vkDevice, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphore );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create semaphores!\n" );
			assert( 0 );
			return false;
		}

		result = vkCreateSemaphore( m_vkDevice, &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphore );
		if ( VK_SUCCESS != result ) {
			printf( "failed to create semaphores!\n" );
			assert( 0 );
			return false;
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
	// Destroy Semaphores
	vkDestroySemaphore( m_vkDevice, m_vkRenderFinishedSemaphore, nullptr );
	vkDestroySemaphore( m_vkDevice, m_vkImageAvailableSemaphore, nullptr );

	// Destroy Pipeline State
	vkDestroyPipeline( m_vkDevice, m_vkPipeline, nullptr );
	vkDestroyPipelineLayout( m_vkDevice, m_vkPipelineLayout, nullptr );

	// Destroy Descriptor Sets
	vkFreeDescriptorSets( m_vkDevice, m_vkDescriptorPool, m_numDescriptorSets, m_vkDescriptorSets );
	vkDestroyDescriptorSetLayout( m_vkDevice, m_vkDescriptorSetLayout, nullptr );
	vkDestroyDescriptorPool( m_vkDevice, m_vkDescriptorPool, nullptr );

	// Destroy Shader Modules
	vkDestroyShaderModule( m_vkDevice, m_vkShaderModuleVert, nullptr );
	vkDestroyShaderModule( m_vkDevice, m_vkShaderModuleFrag, nullptr );

	// Destroy Model
	vkDestroyBuffer( m_vkDevice, m_vkIndexBuffer, nullptr );
	vkFreeMemory( m_vkDevice, m_vkIndexBufferMemory, nullptr );
	vkDestroyBuffer( m_vkDevice, m_vkVertexBuffer, nullptr );
	vkFreeMemory( m_vkDevice, m_vkVertexBufferMemory, nullptr );

	// Destroy Texture
	for ( int i = 0; i < m_numTextures; i++ ) {
		if ( NULL != m_texture[ i ] ) {
			m_texture[ i ]->Cleanup( m_vkDevice );
			delete m_texture[ i ];
			m_texture[ i ] = NULL;
		}
	}

	// Destroy Command Buffers
	vkFreeCommandBuffers( m_vkDevice, m_vkCommandPool, static_cast< uint32_t >( m_vkCommandBuffers.size() ), m_vkCommandBuffers.data() );
	vkDestroyCommandPool( m_vkDevice, m_vkCommandPool, nullptr );

	// Destroy Uniform Buffer Memory
	vkDestroyBuffer( m_vkDevice, m_vkUniformBuffer, nullptr );
	vkFreeMemory( m_vkDevice, m_vkUniformBufferMemory, nullptr );

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
void Application::OnWindowResized( GLFWwindow * window, int windowWidth, int windowHeight ) {
	if ( 0 == windowWidth || 0 == windowHeight ) {
		return;
	}

	Application * application = reinterpret_cast< Application * >( glfwGetWindowUserPointer( window ) );
	application->ResizeWindow( windowWidth, windowHeight );
}

/*
====================================================
Application::ResizeWindow
====================================================
*/
void Application::ResizeWindow( int windowWidth, int windowHeight ) {
	vkDeviceWaitIdle( m_vkDevice );

	// Destroy swapchain
	{
		// Destroy the depth image
		vkDestroyImageView( m_vkDevice, m_vkSwapChainDepthImageView, nullptr );
		vkDestroyImage( m_vkDevice, m_vkSwapChainDepthImage, nullptr );
		vkFreeMemory( m_vkDevice, m_vkSwapChainDepthImageMemory, nullptr );

		for ( VkFramebuffer framebuffer : m_vkSwapChainFramebuffers ) {
			vkDestroyFramebuffer( m_vkDevice, framebuffer, nullptr );
		}

		// Destroy the pipeline
		vkDestroyPipeline( m_vkDevice, m_vkPipeline, nullptr );
		vkDestroyPipelineLayout( m_vkDevice, m_vkPipelineLayout, nullptr );

		// Destroy the renderpass
		vkDestroyRenderPass( m_vkDevice, m_vkRenderPass, nullptr );

		for ( VkImageView imageView : m_vkSwapChainImageViews ) {
			vkDestroyImageView( m_vkDevice, imageView, nullptr );
		}

		vkDestroySwapchainKHR( m_vkDevice, m_vkSwapChain, nullptr );
	}

	CreateSwapChain( windowWidth, windowHeight );
	CreatePipeline( windowWidth, windowHeight );
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
		DrawFrame();
	}
}

/*
===================================
myOrient
* Create an orientation matrix from pos,
* fwd and up vectors
* Only works properly when the default axis is:
* +x = fwd
* +y = left
* +z = up
===================================
*/
void myOrient( Vec3d Pos, Vec3d Fwd, Vec3d Up, float * out ) {
	//    xaxis = Fwd
	//    yaxis = Left
	//    zaxis = Up
	//
	//  For DirectX it should be:
	//    xaxis.x           xaxis.y           xaxis.z   0
	//    yaxis.x           yaxis.y           yaxis.z   0
	//    zaxis.x           zaxis.y           zaxis.z   0
	//    Pos.x             Pos.y             Pos.z     1
	//  Since DirectX handles its matrices as row major...
	//  and OpenGL is column major...
	//  for OpenGL it should be the transverse of this matrix:
	//    xaxis.x           yaxis.x           zaxis.x   Pos.x
	//    xaxis.y           yaxis.y           zaxis.y   Pos.y
	//    xaxis.z           yaxis.z           zaxis.z   Pos.z
	//    0                 0                 0         1

	Up.Normalize();
	Fwd.Normalize();
	Vec3d xaxis = Fwd;
	Vec3d yaxis = Up.Cross( Fwd ); // get left vector
	yaxis.Normalize();
	Vec3d zaxis = Up;

#ifdef DIRECTX
	out[0] = xaxis.x;
	out[1] = yaxis.x;
	out[2] = zaxis.x;
	out[3] = Pos.x;

	out[4] = xaxis.y;
	out[5] = yaxis.y;
	out[6] = zaxis.y;
	out[7] = Pos.y;

	out[8] = xaxis.z;
	out[9] = yaxis.z;
	out[10] = zaxis.z;
	out[11] = Pos.z;

	out[12] = 0;
	out[13] = 0;
	out[14] = 0;
	out[15] = 1;
#else // OPENGL
	out[0] = xaxis.x;
	out[1] = xaxis.y;
	out[2] = xaxis.z;
	out[3] = 0;

	out[4] = yaxis.x;
	out[5] = yaxis.y;
	out[6] = yaxis.z;
	out[7] = 0;

	out[8] = zaxis.x;
	out[9] = zaxis.y;
	out[10] = zaxis.z;
	out[11] = 0;

	out[12] = Pos.x;
	out[13] = Pos.y;
	out[14] = Pos.z;
	out[15] = 1;
#endif
}

/*
===================================
mygluLookAt
===================================
*/
void mygluLookAt( Vec3d pos, Vec3d lookAt, Vec3d up, float * m ) {
	Vec3d lookDir = pos - lookAt;
	lookDir.Normalize();

	Vec3d right = up.Cross( lookDir );
	right.Normalize();

	up = lookDir.Cross( right );
	up.Normalize();

	m[ 0 ] = right[ 0 ];
	m[ 1 ] = up[ 0 ];
	m[ 2 ] = lookDir[ 0 ];
	m[ 3 ] = 0;

	m[ 4 ] = right[ 1 ];
	m[ 5 ] = up[ 1 ];
	m[ 6 ] = lookDir[ 1 ];
	m[ 7 ] = 0;

	m[ 8 ] = right[ 2 ];
	m[ 9 ] = up[ 2 ];
	m[ 10 ] = lookDir[ 2 ];
	m[ 11 ] = 0;

	m[ 12 ] = -pos.DotProduct( right );
	m[ 13 ] = -pos.DotProduct( up );
	m[ 14 ] = -pos.DotProduct( lookDir );
	m[ 15 ] = 1;    
}


/*
====================================
myglMatrixMultiply
====================================
*/
void myglMatrixMultiply( const float * a, const float * b, float * out ) {
	out[ 0 ]  = a[ 0 ]*b[ 0 ] + a[ 1 ]*b[ 4 ] + a[ 2 ]*b[ 8 ]  + a[3 ]*b[12];
	out[ 1 ]  = a[ 0 ]*b[ 1 ] + a[ 1 ]*b[ 5 ] + a[ 2 ]*b[ 9 ]  + a[3 ]*b[13];
	out[ 2 ]  = a[ 0 ]*b[ 2 ] + a[ 1 ]*b[ 6 ] + a[ 2 ]*b[ 10 ] + a[3 ]*b[14];
	out[ 3 ]  = a[ 0 ]*b[ 3 ] + a[ 1 ]*b[ 7 ] + a[ 2 ]*b[ 11 ] + a[3 ]*b[15];

	out[ 4 ]  = a[ 4 ]*b[ 0 ] + a[ 5 ]*b[ 4 ] + a[ 6 ]*b[ 8 ]  + a[7 ]*b[12];
	out[ 5 ]  = a[ 4 ]*b[ 1 ] + a[ 5 ]*b[ 5 ] + a[ 6 ]*b[ 9 ]  + a[7 ]*b[13];
	out[ 6 ]  = a[ 4 ]*b[ 2 ] + a[ 5 ]*b[ 6 ] + a[ 6 ]*b[ 10 ] + a[7 ]*b[14];
	out[ 7 ]  = a[ 4 ]*b[ 3 ] + a[ 5 ]*b[ 7 ] + a[ 6 ]*b[ 11 ] + a[7 ]*b[15];

	out[ 8 ]  = a[ 8 ]*b[ 0 ] + a[ 9 ]*b[ 4 ] + a[ 10 ]*b[ 8 ]  + a[11]*b[12];
	out[ 9 ]  = a[ 8 ]*b[ 1 ] + a[ 9 ]*b[ 5 ] + a[ 10 ]*b[ 9 ]  + a[11]*b[13];
	out[ 10 ] = a[ 8 ]*b[ 2 ] + a[ 9 ]*b[ 6 ] + a[ 10 ]*b[ 10 ] + a[11]*b[14];
	out[ 11 ] = a[ 8 ]*b[ 3 ] + a[ 9 ]*b[ 7 ] + a[ 10 ]*b[ 11 ] + a[11]*b[15];

	out[ 12 ] = a[ 12 ]*b[ 0 ] + a[ 13 ]*b[ 4 ] + a[ 14 ]*b[ 8 ]  + a[15]*b[12];
	out[ 13 ] = a[ 12 ]*b[ 1 ] + a[ 13 ]*b[ 5 ] + a[ 14 ]*b[ 9 ]  + a[15]*b[13];
	out[ 14 ] = a[ 12 ]*b[ 2 ] + a[ 13 ]*b[ 6 ] + a[ 14 ]*b[ 10 ] + a[15]*b[14];
	out[ 15 ] = a[ 12 ]*b[ 3 ] + a[ 13 ]*b[ 7 ] + a[ 14 ]*b[ 11 ] + a[15]*b[15];
}


/*
====================================
mygluPerspectiveOpenGL
* This is my version of glut's convenient perspective
* matrix generation
====================================
*/
void mygluPerspectiveOpenGL( float fovy, float aspect_ratio, float near, float far, float * out ) {
	// This perspective matrix definition was defined in http://www.opengl.org/sdk/docs/man/xhtml/gluPerspective.xml
	const float fovy_radians = fovy * 3.14159f / 180.0f;
	const float f = 1.0f / tanf( fovy_radians * 0.5f );
	const float xscale = f;
	const float yscale = f / aspect_ratio;

	out[ 0 ] = xscale;
	out[ 1 ] = 0;
	out[ 2 ] = 0;
	out[ 3 ] = 0;

	out[ 4 ] = 0;
	out[ 5 ] = yscale;
	out[ 6 ] = 0;
	out[ 7 ] = 0;

	out[ 8 ] = 0;
	out[ 9 ] = 0;
	out[ 10] = ( far + near ) / ( near - far );
	out[ 11] = -1;

	out[ 12] = 0;
	out[ 13] = 0;
	out[ 14] = ( 2.0f * far * near ) / ( near - far );
	out[ 15] = 0;
}
void mygluPerspectiveVulkan( float fovy, float aspect_ratio, float near, float far, float * out ) {
	// Vulkan changed its NDC.  It switch from a left handed coordinate system to a right handed one.
	// +x points to the right, +z points into the screen, +y points down (it used to point in up, in opengl).
	// It also changed the range from [-1,1] to [0,1] for the z.
	// Clip space remains [-1,1] for x and y.
	// Check section 23 of the specification.
#if 1
	float matVulkan[ 16 ];
	matVulkan[ 0 ] = 1;
	matVulkan[ 1 ] = 0;
	matVulkan[ 2 ] = 0;
	matVulkan[ 3 ] = 0;

	matVulkan[ 4 ] = 0;
	matVulkan[ 5 ] = -1;
	matVulkan[ 6 ] = 0;
	matVulkan[ 7 ] = 0;

	matVulkan[ 8 ] = 0;
	matVulkan[ 9 ] = 0;
	matVulkan[ 10] = 0.5f;
	matVulkan[ 11] = 0;

	matVulkan[ 12] = 0;
	matVulkan[ 13] = 0;
	matVulkan[ 14] = 0.5f;
	matVulkan[ 15] = 1;

	float matOpenGL[ 16 ];
	mygluPerspectiveOpenGL( fovy, aspect_ratio, near, far, matOpenGL );

	myglMatrixMultiply( matOpenGL, matVulkan, out );
#else
	const float fovy_radians = fovy * 3.14159f / 180.0f;
	const float f = 1.0f / tanf( fovy_radians * 0.5f );
	const float xscale = f;
	const float yscale = -f / aspect_ratio;

	out[ 0 ] = xscale;
	out[ 1 ] = 0;
	out[ 2 ] = 0;
	out[ 3 ] = 0;

	out[ 4 ] = 0;
	out[ 5 ] = yscale;
	out[ 6 ] = 0;
	out[ 7 ] = 0;

	out[ 8 ] = 0;
	out[ 9 ] = 0;
	out[ 10] = ( far + near ) / ( near - far );
	out[ 11] = -1;

	out[ 12] = 0;
	out[ 13] = 0;
	out[ 14] = ( 2.0f * far * near ) / ( near - far );
	out[ 15] = 0;
#endif
}

/*
====================================
myMatTranspose
====================================
*/
void myMatTranspose( float * data ) {
	const int width = 4;
	for ( int x = 0; x < width; x++ ) {
		for ( int y = x + 1; y < width; y++ ) {
			const int idx0 = x + width * y;
			const int idx1 = y + width * x;
			std::swap( data[ idx0 ], data[ idx1 ] );
		}
	}
}

/*
====================================================
Application::DrawFrame
====================================================
*/
float time = 0;
void Application::DrawFrame() {
	int numDescriptorSetsUsed = 0;
	uint32_t uboByteOffset = 0;

	struct camera_t {
		float matView[ 16 ];
		float matProj[ 16 ];
	};
	camera_t camera;

	// Update the uniform buffer with the camera information
	{
		Vec3d camPos = Vec3d( 10, 0, 5 ) * 1.25f;
		Vec3d camLookAt = Vec3d( 0, 0, 1 );
		Vec3d camUp = Vec3d( 0, 0, 1 );

		int windowWidth;
		int windowHeight;
		glfwGetWindowSize( m_glfwWindow, &windowWidth, &windowHeight );

		const float zNear   = 0.1f;
		const float zFar    = 1000.0f;
		const float fovy	= 45;
		const float aspect	= (float)windowHeight / (float)windowWidth;
		mygluPerspectiveVulkan( fovy, aspect, zNear, zFar, camera.matProj );
		mygluLookAt( camPos, camLookAt, camUp, camera.matView );

		// Update the uniform buffer for the camera matrices
		void * mappedData = NULL;
		vkMapMemory( m_vkDevice, m_vkUniformBufferMemory, uboByteOffset, sizeof( camera ), 0, &mappedData );
		memcpy( mappedData, &camera, sizeof( camera ) );
		vkUnmapMemory( m_vkDevice, m_vkUniformBufferMemory );

		// update offset into the buffer
		uboByteOffset += sizeof( camera );
	}

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR( m_vkDevice, m_vkSwapChain, std::numeric_limits< uint64_t >::max(), m_vkImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );
	if ( VK_SUCCESS != result ) {
		printf( "failed to acquire swap chain image!\n" );
		assert( 0 );
	}

	// Reset the command buffer
	vkResetCommandBuffer( m_vkCommandBuffers[ imageIndex ], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );

	// Record Draw Commands
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer( m_vkCommandBuffers[ imageIndex ], &beginInfo );


		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vkRenderPass;
		renderPassInfo.framebuffer = m_vkSwapChainFramebuffers[ imageIndex ];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_vkSwapChainExtent;

		std::array< VkClearValue, 2 > clearValues = {};
		clearValues[ 0 ].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[ 1 ].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>( clearValues.size() );
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass( m_vkCommandBuffers[ imageIndex ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
		
		time += 0.001f;
		for ( int i = 0; i < m_entities.size(); i++ ) {
			const float x = cosf( time * float( i ) );
			const float y = sinf( time * float( i ) );

			Entity_t & entity = m_entities[ i ];
			entity.fwd = Vec3d( x, y, 0 );
			float matOrient[ 16 ];
			myOrient( entity.pos, entity.fwd, entity.up, matOrient );			

			// Update the uniform buffer with the orientation of this entity
			{
				void * mappedData = NULL;
				vkMapMemory( m_vkDevice, m_vkUniformBufferMemory, uboByteOffset, sizeof( matOrient ), 0, &mappedData );
				memcpy( mappedData, matOrient, sizeof( matOrient ) );
				vkUnmapMemory( m_vkDevice, m_vkUniformBufferMemory );
			}

			// Issue Draw Commands
			{
				vkCmdBindPipeline( m_vkCommandBuffers[ imageIndex ], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline );

				// Create the descriptor set for this draw command
				const int descriptorIdx = numDescriptorSetsUsed;
				{
					VkDescriptorBufferInfo bufferInfoCamera = {};
					bufferInfoCamera.buffer = m_vkUniformBuffer;
					bufferInfoCamera.offset = 0;
					bufferInfoCamera.range = sizeof( camera_t );

					VkDescriptorBufferInfo bufferInfoModel = {};
					bufferInfoModel.buffer = m_vkUniformBuffer;
					bufferInfoModel.offset = uboByteOffset;
					bufferInfoModel.range = sizeof( matOrient );

					VkDescriptorImageInfo imageInfo[ 4 ] = {};
					for ( int i = 0; i < m_numTextures; i++ ) {
						imageInfo[ i ].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfo[ i ].imageView = m_texture[ i ]->m_vkTextureImageView;
						imageInfo[ i ].sampler = m_texture[ i ]->m_vkTextureSampler;
					}

					std::array< VkWriteDescriptorSet, 2 + 4 > descriptorWrites = {};

					descriptorWrites[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[ 0 ].dstSet = m_vkDescriptorSets[ numDescriptorSetsUsed ];
					descriptorWrites[ 0 ].dstBinding = 0;
					descriptorWrites[ 0 ].dstArrayElement = 0;
					descriptorWrites[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrites[ 0 ].descriptorCount = 1;
					descriptorWrites[ 0 ].pBufferInfo = &bufferInfoCamera;

					descriptorWrites[ 1 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[ 1 ].dstSet = m_vkDescriptorSets[ numDescriptorSetsUsed ];
					descriptorWrites[ 1 ].dstBinding = 1;
					descriptorWrites[ 1 ].dstArrayElement = 0;
					descriptorWrites[ 1 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrites[ 1 ].descriptorCount = 1;
					descriptorWrites[ 1 ].pBufferInfo = &bufferInfoModel;

					for ( int i = 0; i < m_numTextures; i++ ) {
						descriptorWrites[ 2 + i ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						descriptorWrites[ 2 + i ].dstSet = m_vkDescriptorSets[ numDescriptorSetsUsed ];
						descriptorWrites[ 2 + i ].dstBinding = 2 + i;
						descriptorWrites[ 2 + i ].dstArrayElement = 0;
						descriptorWrites[ 2 + i ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						descriptorWrites[ 2 + i ].descriptorCount = 1;
						descriptorWrites[ 2 + i ].pImageInfo = &imageInfo[ i ];
					}

					vkUpdateDescriptorSets( m_vkDevice, static_cast< uint32_t >( descriptorWrites.size() ), descriptorWrites.data(), 0, nullptr );
					++numDescriptorSetsUsed;
				}

				// Bind the model
				VkBuffer vertexBuffers[] = { m_vkVertexBuffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers( m_vkCommandBuffers[ imageIndex ], 0, 1, vertexBuffers, offsets );
				vkCmdBindIndexBuffer( m_vkCommandBuffers[ imageIndex ], m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

				// Bind the uniforms
				vkCmdBindDescriptorSets( m_vkCommandBuffers[ imageIndex ], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout, 0, 1, &m_vkDescriptorSets[ descriptorIdx ], 0, nullptr );

				// Issue draw command
				vkCmdDrawIndexed( m_vkCommandBuffers[ imageIndex ], static_cast<uint32_t>( m_numModelIndices ), 1, 0, 0, 0 );

				uboByteOffset += sizeof( matOrient );
			}
		}

		vkCmdEndRenderPass( m_vkCommandBuffers[ imageIndex ] );

		result = vkEndCommandBuffer( m_vkCommandBuffers[ imageIndex ] );
		if ( VK_SUCCESS != result ) {
			printf( "failed to record command buffer!\n" );
			assert( 0 );
		}
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_vkImageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkCommandBuffers[ imageIndex ];

	VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	result = vkQueueSubmit( m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	if ( VK_SUCCESS != result ) {
		printf( "failed to submit draw command buffer!\n" );
		assert( 0 );
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_vkSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR( m_vkPresentQueue, &presentInfo );
	if ( VK_SUCCESS != result ) {
		printf( "failed to present swap chain image!\n" );
		assert( 0 );
	}

	vkQueueWaitIdle( m_vkPresentQueue );
}