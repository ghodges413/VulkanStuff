//
//  application.h
//
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


/*
====================================================
SwapChainSupportDetails
====================================================
*/
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector< VkSurfaceFormatKHR > formats;
	std::vector< VkPresentModeKHR > presentModes;
};

/*
====================================================
QueueFamilyIndices
====================================================
*/
struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;	// Check that it's a device that can both and draw and present it to a monitor?
	}
};


/*
====================================================
Application
====================================================
*/
class Application {
public:
	Application();
	~Application();

	void MainLoop();

private:
	GLFWwindow * m_glfwWindow;

	VkInstance m_vkInstance;
	VkDebugReportCallbackEXT m_vkDebugCallback;

	VkSurfaceKHR m_vkSurface;

	VkPhysicalDevice m_vkPhysicalDevice;
	VkDevice m_vkDevice;

	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;

	bool CheckValidationLayerSupport() const;
	std::vector< const char * > GetGLFWRequiredExtensions() const;

	void InitializeGLFW();
	bool InitializeVulkan();
	void Cleanup();

	// Window or Instance Related
	static void OnWindowResized( GLFWwindow * window, int width, int height );
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData );

	// Device related
	static bool CheckDeviceExtensionSupport( VkPhysicalDevice vkPhysicalDevice );
	static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurface );
	static bool IsDeviceSuitable( VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurface );
	static SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR vkSurface );


	static const int WINDOW_WIDTH = 1200;
	static const int WINDOW_HEIGHT = 720;

#if defined( DEBUG )
	static const bool m_enableLayers = true;
#else
	static const bool m_enableLayers = false;
#endif

	static const std::vector< const char * > m_ValidationLayers;

	static const std::vector< const char * > m_DeviceExtensions;
};
