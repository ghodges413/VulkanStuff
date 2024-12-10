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
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/Buffer.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Image.h"
#include "Graphics/Targa.h"
#include "Scenes/Scene.h"

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

	bool isTransparent;
};

/*
====================================================
Application
====================================================
*/
class Application {
public:
	Application() { m_scene = NULL; m_timeAccumuledSeconds = 0; m_takeScreenshot = false; }
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
	Buffer m_uniformBuffer[ 2 ];

	//
	//	Descriptor Sets
	//
	Descriptors m_descriptorsCopy;

	//
	//	PipelineState
	//
 	Pipeline m_pipelineCopy;

	Model m_modelFullScreen;
	

	std::vector< Entity_t > m_entities;
	std::vector< RenderModel > m_renderModels;

	std::vector< const char * > GetGLFWRequiredExtensions() const;

	void InitializeGLFW();
	bool InitializeVulkan();
	void Cleanup();
	void UpdateUniforms();
	void DrawFrame();
	void ResizeWindow( int windowWidth, int windowHeight );

public:
	void MouseMoved( float x, float y );
	void MouseScrolled( float z );
	void Keyboard( int key, int scancode, int action, int modifiers );

	// Window or Instance Related
	static void OnWindowResized( GLFWwindow * window, int width, int height );

private:
#if defined( GEN_LIGHTPROBE ) || defined( GEN_AMBIENT )
	static const int WINDOW_WIDTH = 256;
	static const int WINDOW_HEIGHT = 256;
#else
 	static const int WINDOW_WIDTH = 1920;
 	static const int WINDOW_HEIGHT = 1080;
#endif

	

#if defined( _DEBUG )
	static const bool m_enableLayers = true;
#else
	static const bool m_enableLayers = false;
#endif

	Vec2 m_mousePosition;

	Scene * m_scene;

	bool m_takeScreenshot;
	float m_timeAccumuledSeconds;
};

extern Application * g_application;