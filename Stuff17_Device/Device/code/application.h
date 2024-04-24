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

#include "Math/Vector.h"
#include "DeviceContext.h"
#include "Buffer.h"

class Texture;


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

struct Entity_t {
	Vec3 pos;
	Vec3 fwd;
	Vec3 up;
};

/*
====================================================
Application
====================================================
*/
class Application {
public:
	Application() {}
	~Application();

	void Initialize();
	void MainLoop();

private:
	GLFWwindow * m_glfwWindow;

	VkInstance m_vkInstance;
	VkDebugReportCallbackEXT m_vkDebugCallback;

	//
	//	Device related
	//
	DeviceContext m_device;

	//
	//	Uniform Buffer
	//
	Buffer m_uniformBuffer;

	//
	//	Texture loaded from file
	//
	static const int m_numTextures = 4;
	Texture * m_texture[ m_numTextures ];

	//
	//	Descriptor Sets
	//
	VkDescriptorPool m_vkDescriptorPool;
	VkDescriptorSetLayout m_vkDescriptorSetLayout;
	static const int m_numDescriptorSets = 256;
	VkDescriptorSet m_vkDescriptorSets[ m_numDescriptorSets ];

	//
	//	PipelineState
	//
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkPipeline;

	std::vector< Entity_t > m_entities;

	bool CheckValidationLayerSupport() const;
	std::vector< const char * > GetGLFWRequiredExtensions() const;

	void InitializeGLFW();
	bool InitializeVulkan();
	bool CreatePipeline( int windowWidth, int windowHeight );
	void Cleanup();
	void DrawFrame();
	void ResizeWindow( int windowWidth, int windowHeight );

	// Window or Instance Related
	static void OnWindowResized( GLFWwindow * window, int width, int height );
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData );

private:



// 	static const int WINDOW_WIDTH = 1200;
// 	static const int WINDOW_HEIGHT = 720;

	static const int WINDOW_WIDTH = 1920;
	static const int WINDOW_HEIGHT = 1080;

#if defined( DEBUG )
	static const bool m_enableLayers = true;
#else
	static const bool m_enableLayers = false;
#endif

	static const std::vector< const char * > m_ValidationLayers;

	static const std::vector< const char * > m_DeviceExtensions;
};

extern Application * g_application;