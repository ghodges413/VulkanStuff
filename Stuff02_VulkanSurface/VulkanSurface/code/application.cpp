//
//  application.cpp
//
#include "application.h"
#include <assert.h>


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
Application::InitializeVulkan
====================================================
*/
bool Application::InitializeVulkan() {
	if ( m_enableLayers && !CheckValidationLayerSupport() ) {
		printf( "validation layers requested, but not available!" );
		assert( 0 );
		return false;
	}

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

	if ( glfwCreateWindowSurface( m_vkInstance, m_glfwWindow, nullptr, &m_vkSurface ) != VK_SUCCESS ) {
		printf( "failed to create window surface!" );
		assert( 0 );
		return false;
	}

	return true;
}

/*
====================================================
Application::Cleanup
====================================================
*/
void Application::Cleanup() {
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
