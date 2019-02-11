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

	//
	//	Device related
	//
	VkPhysicalDevice m_vkPhysicalDevice;
	VkDevice m_vkDevice;

	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;

	//
	//	Swap chain related
	//
	VkSwapchainKHR m_vkSwapChain;
	VkExtent2D m_vkSwapChainExtent;

	VkFormat m_vkSwapChainColorImageFormat;
	std::vector< VkImage > m_vkSwapChainColorImages;
	std::vector< VkImageView > m_vkSwapChainImageViews;

	VkFormat m_vkSwapChainDepthFormat;
	VkImage m_vkSwapChainDepthImage;
	VkImageView m_vkSwapChainDepthImageView;
	VkDeviceMemory m_vkSwapChainDepthImageMemory;
	
	std::vector< VkFramebuffer > m_vkSwapChainFramebuffers;

	VkRenderPass m_vkRenderPass;

	//
	//	Uniform Buffer
	//
	VkBuffer m_vkUniformBuffer;
	VkDeviceMemory m_vkUniformBufferMemory;
	VkDeviceSize m_vkUniformBufferSize;

	//
	//	Command Buffers
	//
	VkCommandPool m_vkCommandPool;
	std::vector< VkCommandBuffer > m_vkCommandBuffers;

	//
	//	Texture loaded from file
	//
	VkFormat m_vkTextureFormat;
	VkImage m_vkTextureImage;
	VkImageView m_vkTextureImageView;
	VkDeviceMemory m_vkTextureDeviceMemory;
	VkSampler m_vkTextureSampler;
	VkExtent2D m_vkTextureExtents;

	//
	//	Model
	//
	VkBuffer m_vkVertexBuffer;
	VkDeviceMemory m_vkVertexBufferMemory;
	VkBuffer m_vkIndexBuffer;
	VkDeviceMemory m_vkIndexBufferMemory;

	//
	//	Shader Modules
	//
	VkShaderModule m_vkShaderModuleVert;
	VkShaderModule m_vkShaderModuleFrag;

	//
	//	Descriptor Sets
	//
	VkDescriptorPool m_vkDescriptorPool;
	VkDescriptorSetLayout m_vkDescriptorSetLayout;
	static const int m_numDescriptorSets = 16;
	VkDescriptorSet m_vkDescriptorSets[ m_numDescriptorSets ];

	//
	//	PipelineState
	//
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkPipeline;

	//
	//	Semaphores
	//
	VkSemaphore m_vkImageAvailableSemaphore;
	VkSemaphore m_vkRenderFinishedSemaphore;


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

	// Swap chain related
	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> & availableFormats );
	static VkPresentModeKHR ChooseSwapPresentMode( const std::vector< VkPresentModeKHR > availablePresentModes );
	static VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR & capabilities, uint32_t width, uint32_t height );
	uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );



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
