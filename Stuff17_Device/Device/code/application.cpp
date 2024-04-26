//
//  application.cpp
//
#include "application.h"
#include "Models/ModelManager.h"
#include "Models/ModelStatic.h"
#include "Graphics/Samplers.h"
#include "Graphics/shader.h"
#include "Miscellaneous/Fileio.h"
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
	entity.pos = Vec3( 0, 0, 0 );
	entity.fwd = Vec3( 1, 0, 0 );
	entity.up = Vec3( 0, 0, 1 );
	m_entities.push_back( entity );

	entity.pos = Vec3( 0, 4, 0 );
	entity.fwd = Vec3( 1, 0, 0 );
	entity.up = Vec3( 0, 0, 1 );
	m_entities.push_back( entity );

	entity.pos = Vec3( 0,-4, 0 );
	entity.fwd = Vec3( 1, 0, 0 );
	entity.up = Vec3( 0, 0, 1 );
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
Application::CreatePipeline
====================================================
*/
bool Application::CreatePipeline( int windowWidth, int windowHeight ) {
	Pipeline::CreateParms_t pipelineParms;
	memset( &pipelineParms, 0, sizeof( pipelineParms ) );
	pipelineParms.renderPass = m_device.m_swapChain.m_vkRenderPass;
	pipelineParms.descriptors = &m_descriptors;
	pipelineParms.shader = g_shaderManager->GetShader( "GGX" );
	pipelineParms.width = m_device.m_swapChain.m_windowWidth;
	pipelineParms.height = m_device.m_swapChain.m_windowHeight;
	pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
	pipelineParms.blendMode = Pipeline::BLEND_MODE_NONE;
	pipelineParms.fillMode = Pipeline::FILL_MODE_FILL;
	pipelineParms.depthTest = true;
	pipelineParms.depthWrite = true;
	bool result = m_pipeline.Create( &m_device, pipelineParms );
	if ( !result ) {
		printf( "ERROR: Failed to create copy pipeline\n" );
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
	//
	//	Vulkan Instance
	//
	{
		std::vector< const char * > extensions = GetGLFWRequiredExtensions();
		if ( !m_device.CreateInstance( m_enableLayers, extensions ) ) {
			printf( "ERROR: Failed to create vulkan instance\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Vulkan Surface for GLFW Window
	//
	if ( VK_SUCCESS != glfwCreateWindowSurface( m_device.m_vkInstance, m_glfwWindow, nullptr, &m_device.m_vkSurface ) ) {
		printf( "ERROR: Failed to create window surface\n" );
		assert( 0 );
		return false;
	}

	int windowWidth;
	int windowHeight;
	glfwGetWindowSize( m_glfwWindow, &windowWidth, &windowHeight );

	//
	//	Vulkan Device
	//
	if ( !m_device.CreateDevice() ) {
		printf( "ERROR: Failed to create device\n" );
		assert( 0 );
		return false;
	}	

	//
	//	Create SwapChain
	//
	if ( !m_device.CreateSwapChain( windowWidth, windowHeight ) ) {
		printf( "ERROR: Failed to create swapchain\n" );
		assert( 0 );
		return false;
	}

	//
	//	Initialize texture samplers
	//
	Samplers::InitializeSamplers( &m_device );

	//
	//	Initialize Globals that depend on the device
	//
	g_shaderManager = new ShaderManager( &m_device );
	g_modelManager = new ModelManager( &m_device );

	//
	//	Command Buffers
	//
	if ( !m_device.CreateCommandBuffers() ) {
		printf( "ERROR: Failed to create command buffers\n" );
		assert( 0 );
		return false;
	}

	//
	//	Uniform Buffer
	//
	m_uniformBuffer.Allocate( &m_device, NULL, sizeof( float ) * 16 * 4 * 1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	//
	//	Texture Images
	//
	{
		Targa targaDiffuse;
		targaDiffuse.Load( "../../common/data/images/shaderBall/paintedMetal_Diffuse.tga" );
		
		Targa targaNormals;
		targaNormals.Load( "../../common/data/images/shaderBall/paintedMetal_Normal.tga" );

		Targa targaGloss;
		targaGloss.Load( "../../common/data/images/shaderBall/paintedMetal_Glossiness.tga" );
		
		Targa targaSpecular;
		targaSpecular.Load( "../../common/data/images/shaderBall/paintedMetal_Specular.tga" );

		Image::CreateParms_t imageParms;
		imageParms.depth = 1;
		imageParms.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageParms.mipLevels = 1;
		imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		imageParms.width = targaDiffuse.GetWidth();
		imageParms.height = targaDiffuse.GetHeight();
		m_imageDiffuse.Create( &m_device, imageParms );
		m_imageDiffuse.UploadData( &m_device, targaDiffuse.DataPtr() );

		imageParms.width = targaNormals.GetWidth();
		imageParms.height = targaNormals.GetHeight();
		m_imageNormals.Create( &m_device, imageParms );
		m_imageNormals.UploadData( &m_device, targaNormals.DataPtr() );

		imageParms.width = targaGloss.GetWidth();
		imageParms.height = targaGloss.GetHeight();
		m_imageGloss.Create( &m_device, imageParms );
		m_imageGloss.UploadData( &m_device, targaGloss.DataPtr() );

		imageParms.width = targaSpecular.GetWidth();
		imageParms.height = targaSpecular.GetHeight();
		m_imageSpecular.Create( &m_device, imageParms );
		m_imageSpecular.UploadData( &m_device, targaSpecular.DataPtr() );
	}

	//
	//	Model loaded from file
	//
	Model * model = g_modelManager->GetModel( "../../common/data/meshes/static/shaderBall.obj" );

	//
	//	Descriptor Sets
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		descriptorParms.numSamplerImagesFragment = 4;
		bool result = m_descriptors.Create( &m_device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Pipeline State
	//
	if ( !CreatePipeline( windowWidth, windowHeight ) ) {
		printf( "Failed to create pipeline!\n" );
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
	delete g_shaderManager;
	g_shaderManager = NULL;

	// Delete models
	delete g_modelManager;

	// Destroy Images
	m_imageDiffuse.Cleanup( &m_device );
	m_imageNormals.Cleanup( &m_device );
	m_imageGloss.Cleanup( &m_device );
	m_imageSpecular.Cleanup( &m_device );

	m_pipeline.Cleanup( &m_device );
	m_descriptors.Cleanup( &m_device );

	// Delete Uniform Buffer Memory
	m_uniformBuffer.Cleanup( &m_device );

	// Delete Samplers
	Samplers::Cleanup( &m_device );

	// Delete Device Context
	m_device.Cleanup();

	// Delete GLFW
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
	m_device.ResizeWindow( windowWidth, windowHeight );

	// Destroy the pipeline
	m_pipeline.Cleanup( &m_device );

	// Rebuild the pipeline
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
====================================================
Application::DrawFrame
====================================================
*/
float g_time = 0;
void Application::DrawFrame() {
	int numDescriptorSetsUsed = 0;
	uint32_t uboByteOffset = 0;

	struct camera_t {
		Mat4 matView;
		Mat4 matProj;
	};
	camera_t camera;

	unsigned char * mappedData = (unsigned char *)m_uniformBuffer.MapBuffer( &m_device );

	// Update the uniform buffer with the camera information
	{
		Vec3 camPos = Vec3( 10, 0, 5 ) * 1.25f;
		Vec3 camLookAt = Vec3( 0, 0, 1 );
		Vec3 camUp = Vec3( 0, 0, 1 );

		int windowWidth;
		int windowHeight;
		glfwGetWindowSize( m_glfwWindow, &windowWidth, &windowHeight );

		const float zNear   = 0.1f;
		const float zFar    = 1000.0f;
		const float fovy	= 45;
		const float aspect	= (float)windowHeight / (float)windowWidth;
		camera.matProj.PerspectiveVulkan( fovy, aspect, zNear, zFar );
		camera.matProj = camera.matProj.Transpose();	// Vulkan is column major
		camera.matView.LookAt( camPos, camLookAt, camUp );
		camera.matView = camera.matView.Transpose();	// Vulkan is column major

		// Update the uniform buffer for the camera matrices
		memcpy( mappedData, &camera, sizeof( camera ) );

		// update offset into the buffer
//		uboByteOffset += sizeof( camera );
		uboByteOffset += m_device.GetAlignedUniformByteOffset( sizeof( camera ) );
	}

	const uint32_t imageIndex = m_device.BeginFrame();

	Image * images[ 4 ];
	images[ 0 ] = &m_imageDiffuse;
	images[ 1 ] = &m_imageNormals;
	images[ 2 ] = &m_imageGloss;
	images[ 3 ] = &m_imageSpecular;

	// Record Draw Commands
	{
		m_device.BeginRenderPass();

		Model * model = g_modelManager->GetModel( "../../common/data/meshes/static/shaderBall.obj" );
		
		g_time += 0.001f;
		for ( int i = 0; i < m_entities.size(); i++ ) {
			const float x = cosf( g_time * float( i ) );
			const float y = sinf( g_time * float( i ) );

			Entity_t & entity = m_entities[ i ];
			entity.fwd = Vec3( x, y, 0 );

			Mat4 matOrient;
			matOrient.Orient( entity.pos, entity.fwd, entity.up );
			matOrient = matOrient.Transpose();	// Vulkan is column major

			// Update the uniform buffer with the orientation of this entity
			memcpy( mappedData + uboByteOffset, matOrient.ToPtr(), sizeof( matOrient ) );

			VkCommandBuffer cmdBuffer = m_device.m_vkCommandBuffers[ imageIndex ];

			m_pipeline.BindPipeline( cmdBuffer );

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = m_pipeline.GetFreeDescriptor();
			descriptor.BindBuffer( &m_uniformBuffer, 0, sizeof( camera ), 0 );					// bind the camera matrices
			descriptor.BindBuffer( &m_uniformBuffer, uboByteOffset, sizeof( matOrient ), 1 );	// bind the model matrices
			descriptor.BindImage( m_imageDiffuse, Samplers::m_samplerStandard, 2 );
			descriptor.BindImage( m_imageNormals, Samplers::m_samplerStandard, 3 );
			descriptor.BindImage( m_imageGloss, Samplers::m_samplerStandard, 4 );
			descriptor.BindImage( m_imageSpecular, Samplers::m_samplerStandard, 5 );
			
			descriptor.BindDescriptor( &m_device, cmdBuffer, &m_pipeline );
			model->DrawIndexed( cmdBuffer );

			//uboByteOffset += sizeof( matOrient );
			uboByteOffset += m_device.GetAlignedUniformByteOffset( sizeof( matOrient ) );
		}

		m_device.EndRenderPass();
	}

	m_device.EndFrame();
}