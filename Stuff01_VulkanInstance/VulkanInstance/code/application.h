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

	bool CheckValidationLayerSupport() const;
	std::vector< const char * > GetGLFWRequiredExtensions() const;

	void InitializeGLFW();
	bool InitializeVulkan();
	void Cleanup();


	static void OnWindowResized( GLFWwindow * window, int width, int height );
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData );

	static const int WINDOW_WIDTH = 1200;
	static const int WINDOW_HEIGHT = 720;

#if defined( DEBUG )
	static const bool m_enableLayers = true;
#else
	static const bool m_enableLayers = false;
#endif

	static const std::vector< const char * > m_ValidationLayers;
};
