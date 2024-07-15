//
//  application.cpp
//
#include <chrono>
#include <thread>

#include "application.h"
#include "Models/ModelManager.h"
#include "Models/ModelStatic.h"
#include "Graphics/Samplers.h"
#include "Graphics/shader.h"
#include "Graphics/Barrier.h"
#include "Miscellaneous/Fileio.h"
#include "Miscellaneous/Time.h"
#include "Miscellaneous/Input.h"
#include "BSP/Map.h"
#include "Physics/Body.h"
#include "Entities/Player.h"
#include "Scenes/SceneGame.h"
#include "Renderer/DrawOffscreen.h"
#include "Renderer/DrawGBuffer.h"
#include "Renderer/BuildLightTiles.h"
#include "Renderer/DrawSkybox.h"
#include "Renderer/DrawLightprobe.h"
#include <assert.h>

Application * g_application = NULL;

extern Player * g_player;

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

	g_Input = new Input( m_glfwWindow );

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

	m_scene = new SceneGame;
	m_scene->Initialize( &m_device );

	//
	//	Add brushes
	//
	std::vector< Vec3 > pts;
	for ( int i = 0; i < g_brushes.size(); i++ ) {
		const brush_t & brush = g_brushes[ i ];
		pts.clear();

		for ( int w = 0; w < brush.numPlanes; w++ ) {
			const winding_t & winding = brush.windings[ w ];

			for ( int v = 0; v < winding.pts.size(); v++ ) {
				pts.push_back( winding.pts[ v ] );
			}
		}

		if ( 0 == pts.size() ) {
			continue;
		}

		Body body;
		body.m_position = Vec3( 0, 0, 0 );
		body.m_orientation = Quat( 0, 0, 0, 1 );
		body.m_linearVelocity.Zero();
		body.m_angularVelocity.Zero();
		body.m_invMass = 0.0f;
		body.m_elasticity = 1.0f;//0.5f;
		body.m_enableRotation = false;
		body.m_friction = 1.0f;//0.5f;
		body.m_shape = new ShapeConvex( pts.data(), pts.size() );

		Vec3 cm = body.m_shape->GetCenterOfMass();
		assert( cm.IsValid() );
		if ( !cm.IsValid() ) {
			delete body.m_shape;
			continue;
		}

		body.m_owner = NULL;
		body.m_bodyContents = BC_GENERIC;
		body.m_collidesWith = BC_ALL;
		bodyID_t m_bodyid = g_physicsWorld->AllocateBody( body );
	}

	//
	//	Build bounds
	//
	Bounds bounds;
	bounds.Clear();
	for ( int i = 0; i < g_brushes.size(); i++ ) {
		const brush_t & brush = g_brushes[ i ];

		for ( int w = 0; w < brush.numPlanes; w++ ) {
			const winding_t & winding = brush.windings[ w ];

			for ( int v = 0; v < winding.pts.size(); v++ ) {
				Vec3 pt = winding.pts[ v ];
				bounds.Expand( pt );
			}
		}
	}

	Vec3 fwd[ 4 ];
	fwd[ 0 ] = Vec3( 1, 0, 0 );
	fwd[ 1 ] = Vec3( 0, 1, 0 );
	fwd[ 2 ] = Vec3( -1, 0, 0 );
	fwd[ 3 ] = Vec3( 0, -1, 0 );

	//
	//	Add "entities" for every position that is inside a brush
	//
	m_entities.clear();
	for ( float z = bounds.mins.z; z < bounds.maxs.z; z += 1.0f ) {
		for ( float y = bounds.mins.y; y < bounds.maxs.y; y += 1.0f ) {
			for ( float x = bounds.mins.x; x < bounds.maxs.x; x += 1.0f ) {
				Vec3 pos = Vec3( x, y, z );

				// Check if this position is inside any of the brushes
				for ( int i = 0; i < g_brushes.size(); i++ ) {
					const brush_t & brush = g_brushes[ i ];
					if ( IsPointInBrush( brush, pos ) ) {
						//int idx = m_entities.size() % 4;
						//entity.fwd = fwd[ idx ];
						entity.pos = pos;
						m_entities.push_back( entity );
						break;
					}
				}
			}
		}
	}
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
#if 1
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
#else
	Pipeline::CreateParms_t pipelineParms;
	memset( &pipelineParms, 0, sizeof( pipelineParms ) );
	pipelineParms.renderPass = m_device.m_swapChain.m_vkRenderPass;
	pipelineParms.descriptors = &m_descriptors;
	pipelineParms.shader = g_shaderManager->GetShader( "checkerboard2" );
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
#endif
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
	m_uniformBuffer.Allocate( &m_device, NULL, sizeof( float ) * 16 * 4 * 4096, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	//
	//	Texture Images
	//
	{
		Targa targaDiffuse;
		Targa targaNormals;
		Targa targaGloss;
		Targa targaSpecular;

		targaDiffuse.Load( "../../common/data/images/block_1x1/block_a_color.tga" );
		targaNormals.Load( "../../common/data/images/block_1x1/block_a_normal.tga" );
		targaGloss.Load( "../../common/data/images/block_1x1/block_a_rough.tga" );
		targaSpecular.Load( "../../common/data/images/block_1x1/block_a_metal.tga" );

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

	InitOffscreen( &m_device, windowWidth, windowHeight );
	InitDepthPrePass( &m_device );	
	InitGBuffer( &m_device, windowWidth, windowHeight );
	InitLightTiles( &m_device, windowWidth, windowHeight );
	InitDebugDrawLightTiles( &m_device, windowWidth, windowHeight );
	InitDrawTiledGGX( &m_device, windowWidth, windowHeight );
	InitSkybox( &m_device, windowWidth, windowHeight );
	InitLightProbe( &m_device, windowWidth, windowHeight );

	//
	//	Full screen texture rendering
	//
	{
		bool result;
		FillFullScreenQuad( m_modelFullScreen );
		for ( int i = 0; i < m_modelFullScreen.m_vertices.size(); i++ ) {
			m_modelFullScreen.m_vertices[ i ].pos[ 1 ] *= -1.0f;
		}
		m_modelFullScreen.MakeVBO( &m_device );

		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numSamplerImagesFragment = 1;
		result = m_descriptorsCopy.Create( &m_device, descriptorParms );
		if ( !result ) {
			printf( "ERROR: Failed to create copy descriptors\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.renderPass = m_device.m_swapChain.m_vkRenderPass;
		pipelineParms.descriptors = &m_descriptorsCopy;
		pipelineParms.shader = g_shaderManager->GetShader( "copyHDR" );
		pipelineParms.width = m_device.m_swapChain.m_windowWidth;
		pipelineParms.height = m_device.m_swapChain.m_windowHeight;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = m_pipelineCopy.Create( &m_device, pipelineParms );
		if ( !result ) {
			printf( "ERROR: Failed to create copy pipeline\n" );
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
	delete m_scene;
	m_scene = NULL;

	delete g_Input;
	g_Input = NULL;

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

	m_pipelineCopy.Cleanup( &m_device );
	m_descriptorsCopy.Cleanup( &m_device );

	// Delete Uniform Buffer Memory
	m_uniformBuffer.Cleanup( &m_device );

	CleanupDepthPrePass( &m_device );
	CleanupOffscreen( &m_device );
	CleanupGBuffer( &m_device );
	CleanupLightTiles( &m_device );
	CleanupDebugDrawLightTiles( &m_device );
	CleanupDrawTiledGGX( &m_device );
	CleanupSkybox( &m_device );
	CleanupLightProbe( &m_device );

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
Application::MouseMoved
====================================================
*/
void Application::MouseMoved( float x, float y ) {
	Vec2 newPosition = Vec2( x, y );
	Vec2 ds = newPosition - m_mousePosition;
	m_mousePosition = newPosition;

	float sensitivity = 0.01f;
	//if ( m_scene->m_firstPersonMode ) 
	{
		sensitivity = 0.003f;
	}

	g_player->m_cameraPositionTheta += ds.y * sensitivity;
	g_player->m_cameraPositionPhi -= ds.x * sensitivity;

	if ( g_player->m_cameraPositionTheta < 0.14f ) {
		g_player->m_cameraPositionTheta = 0.14f;
	}
	if ( g_player->m_cameraPositionTheta > 3.0f ) {
		g_player->m_cameraPositionTheta = 3.0f;
	}
}

/*
====================================================
Application::MouseScrolled
====================================================
*/
void Application::MouseScrolled( float z ) {
}

/*
====================================================
Application::Keyboard
====================================================
*/
void Application::Keyboard( int key, int scancode, int action, int modifiers ) {
	if ( keyboard_t::key_q == key && ( action == GLFW_RELEASE ) ) {
		m_takeScreenshot = true;
	}
}

/*
====================================================
Application::MainLoop
====================================================
*/
void Application::MainLoop() {
	static int timeLastFrame = 0;
	static int numSamples = 0;
	static float avgTime = 0.0f;
	static float maxTime = 0.0f;

	while ( !glfwWindowShouldClose( m_glfwWindow ) ) {
		int time					= GetTimeMicroseconds();
		float dt_us					= (float)time - (float)timeLastFrame;
		if ( dt_us < 16000.0f ) {
			int x = 16000 - (int)dt_us;
			std::this_thread::sleep_for( std::chrono::microseconds( x ) );
			dt_us = 16000;
			time = GetTimeMicroseconds();
		}
		timeLastFrame				= time;
		printf( "\ndt_ms: %.1f    ", dt_us * 0.001f );

		// Get User Input
		glfwPollEvents();

		// If the time is greater than 33ms (30fps) then force the time difference to smaller
		// to prevent super large simulation steps.
		// This will typically happen when we use the debugger.
		if ( dt_us > 33000.0f ) {
			dt_us = 33000.0f;
		}

		float dt_sec = dt_us * 1e-6f;

		// Run Update
		{
			const int startTime = GetTimeMicroseconds();
			m_scene->Update( dt_sec );
			const int endTime = GetTimeMicroseconds();

			dt_us = (float)endTime - (float)startTime;
			if ( dt_us > maxTime ) {
				maxTime = dt_us;
			}

			avgTime = ( avgTime * float( numSamples ) + dt_us ) / float( numSamples + 1 );
			numSamples++;

			printf( "frame dt_ms: %.2f %.2f %.2f", avgTime * 0.001f, maxTime * 0.001f, dt_us * 0.001f );

			m_timeAccumuledSeconds += dt_sec;
		}

		// Draw the Scene
		DrawFrame();
	}
}


// Mat4 g_worldCamViewPrev;
// Mat4 g_worldCamView;
// Mat4 g_worldCamProj;
// Vec3 g_worldCameraPos;
// Quat g_worldCameraOrient;

void PrintMat4( const Mat4 & mat ) {
	for ( int i = 0; i < 4; i++ ) {
		printf( "%.4f, %.4f, %.4f, %.4f,\n", mat.rows[ i ].x, mat.rows[ i ].y, mat.rows[ i ].z, mat.rows[ i ].w );
	}
}

/*
====================================================
Application::UpdateUniforms
====================================================
*/
void Application::UpdateUniforms() {
	m_renderModels.clear();

	uint32_t uboByteOffset = 0;

	struct camera_t {
		Mat4 matView;
		Mat4 matProj;
	};
	camera_t camera;

	//
	//	Update the uniform buffers
	//
	{
		unsigned char * mappedData = (unsigned char *)m_uniformBuffer.MapBuffer( &m_device );

		//
		// Update the uniform buffer with the camera information
		//
		{
			Vec3 camPos = Vec3( 10, 0, 5 ) * 1.25f;
			Vec3 camLookAt = Vec3( 0, 0, 1 );
			Vec3 camUp = Vec3( 0, 0, 1 );

		
			if ( g_player->m_bodyid.body->m_position.z < 0 ) {
				g_player->m_bodyid.body->m_position.z = 0;
				g_player->m_bodyid.body->m_linearVelocity.z = 0;
			}
			camPos = g_player->m_bodyid.body->m_position + Vec3( 0, 0, 1 );

			Vec3 lookDir;
			lookDir.x = cosf( g_player->m_cameraPositionPhi ) * sinf( g_player->m_cameraPositionTheta );
			lookDir.y = sinf( g_player->m_cameraPositionPhi ) * sinf( g_player->m_cameraPositionTheta );
			lookDir.z = cosf( g_player->m_cameraPositionTheta );
			camLookAt = camPos + lookDir;
			Vec3 left = Vec3( 0, 0, 1 ).Cross( lookDir );
			lookDir.z = 0.0f;
			left.z = 0.0f;
			lookDir.Normalize();
			left.Normalize();

#if defined( GEN_LIGHTPROBE )
			// HACK: This is just for the light probe generation
			extern int g_screenshotCount;
			if ( g_screenshotCount < 6 ) {
				camPos = Vec3( 0, 0, 7 );
				if ( 0 == g_screenshotCount ) {
					// +x
					camLookAt = camPos + Vec3( 1, 0, 0 );
					camUp = Vec3( 0, 0, 1 );
				}
				if ( 1 == g_screenshotCount ) {
					// -x
					camLookAt = camPos + Vec3( -1, 0, 0 );
					camUp = Vec3( 0, 0, 1 );
				}
				if ( 2 == g_screenshotCount ) {
					// +y
					camLookAt = camPos + Vec3( 0, 1, 0 );
					camUp = Vec3( 0, 0, 1 );
				}
				if ( 3 == g_screenshotCount ) {
					// -y
					camLookAt = camPos + Vec3( 0, -1, 0 );
					camUp = Vec3( 0, 0, 1 );
				}

				if ( 4 == g_screenshotCount ) {
					// +z
					camLookAt = camPos + Vec3( 0, 0, 1 );
					camUp = Vec3( -1, 0, 0 );
				}
				if ( 5 == g_screenshotCount ) {
					// -z
					camLookAt = camPos + Vec3( 0, 0, -1 );
					camUp = Vec3( 1, 0, 0 );
				}
			}
#endif
		

			int windowWidth;
			int windowHeight;
			glfwGetWindowSize( m_glfwWindow, &windowWidth, &windowHeight );

			const float zNear   = 0.1f;
			const float zFar    = 1000.0f;
			const float fovy	= 90;
			const float aspect	= (float)windowHeight / (float)windowWidth;
			camera.matProj.PerspectiveVulkan( fovy, aspect, zNear, zFar );
			camera.matProj = camera.matProj.Transpose();	// Vulkan is column major
			camera.matView.LookAt( camPos, camLookAt, camUp );
			camera.matView = camera.matView.Transpose();	// Vulkan is column major

			// Update the uniform buffer for the camera matrices
			memcpy( mappedData, &camera, sizeof( camera ) );

			// update offset into the buffer
			uboByteOffset += m_device.GetAlignedUniformByteOffset( sizeof( camera ) );
		}

		//
		//	Fill the uniform buffer with miscellaneous data
		//
		{
			Vec4 time = Vec4( m_timeAccumuledSeconds );
			memcpy( mappedData + uboByteOffset, &time, sizeof( time ) );
			uboByteOffset += m_device.GetAlignedUniformByteOffset( sizeof( time ) );
		}

		//
		//	Update the uniform buffer with the body positions/orientations
		//
		Model * model = g_modelManager->GetModel( "../../common/data/meshes/static/block_a_lp.obj" );

		for ( int i = 0; i < m_entities.size(); i++ ) {
			Entity_t & entity = m_entities[ i ];

			Mat4 matOrient;
			matOrient.Orient( entity.pos, entity.fwd, entity.up );
			matOrient = matOrient.Transpose();	// Vulkan is column major

			RenderModel renderModel;
			renderModel.modelDraw = model;
			renderModel.uboByteOffset = uboByteOffset;
			renderModel.uboByteSize = sizeof( matOrient );
			m_renderModels.push_back( renderModel );

			// Update the uniform buffer with the orientation of this entity
			memcpy( mappedData + uboByteOffset, matOrient.ToPtr(), sizeof( matOrient ) );
			uboByteOffset += m_device.GetAlignedUniformByteOffset( sizeof( matOrient ) );
			if ( uboByteOffset >= m_uniformBuffer.m_vkBufferSize ) {
				break;
			}
		}

		m_uniformBuffer.UnmapBuffer( &m_device );
	}
	
	//
	//	Update Lights
	//
	{
		std::vector< storageLight_t > lights;

		buildLightTileParms_t parms;
		parms.matView = camera.matView;
		parms.matProjInv = camera.matProj.Inverse();
		Vec4 a = Vec4( 1, 1, 0, 1 );
		Vec4 b = Vec4( 1, 1, 1, 1 );
		a = parms.matProjInv.Transpose() * a;
		b = parms.matProjInv.Transpose() * b;
		a /= a.w;
		b /= b.w;
		//parms.matProjInv.Inverse();
		parms.screenWidth = g_offscreenFrameBuffer.m_imageDepth.m_parms.width;
		parms.screenHeight = g_offscreenFrameBuffer.m_imageDepth.m_parms.height;
		UpdateLights( &m_device, lights.data(), lights.size(), parms );
	}
}

/*
====================================================
Application::DrawFrame
====================================================
*/
void Application::DrawFrame() {
	UpdateUniforms();

	//
	//	Begin the render frame
	//
	const uint32_t imageIndex = m_device.BeginFrame();
	VkCommandBuffer cmdBuffer = m_device.m_vkCommandBuffers[ imageIndex ];

	DrawParms_t parms;
	parms.device = &m_device;
	parms.cmdBufferIndex = imageIndex;
	parms.uniforms = &m_uniformBuffer;
	parms.renderModels = m_renderModels.data();
	parms.numModels = (int)m_renderModels.size();
	parms.time = m_timeAccumuledSeconds;
	parms.scissorWidth = 256;
	parms.scissorHeight = 256;

	// Fill the g-buffer
	DrawGBuffer( parms );

	// Use the g-buffer in a compute shader to build the list of tiled lights
	BuildLightTiles( parms );

	// For debug purposes, draw the number of lights in each tile
	DebugDrawLightTiles( parms );

	//
	//	Draw to Offscreen Buffer
	//	
	g_offscreenFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_offscreenFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	g_offscreenFrameBuffer.BeginRenderPass( &m_device, imageIndex, true, true );

	DrawDepthPrePass( parms );
	
	//DrawOffscreen( parms );	// this is an unlit forward rendering to the off screen buffer, only used in older samples

	DrawSkybox( parms );

	// Use the g-buffer and list of tiled lights to draw to the off-screen buffer
	DrawTiledGGX( parms );

#if !defined( GEN_LIGHTPROBE )
	DrawLightProbe( parms );
#endif

	g_offscreenFrameBuffer.EndRenderPass( &m_device, imageIndex );
	g_offscreenFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_offscreenFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL );

	//
	//	Draw the offscreen framebuffer to the swap chain frame buffer
	//
 	m_device.BeginRenderPass();
	{
		// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
		m_pipelineCopy.BindPipeline( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = m_pipelineCopy.GetFreeDescriptor();
		descriptor.BindImage( g_offscreenFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
		//descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 0 );
		//descriptor.BindImage( g_debugDrawLightFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
		descriptor.BindDescriptor( &m_device, cmdBuffer, &m_pipelineCopy );
		m_modelFullScreen.DrawIndexed( cmdBuffer );
	}
 	m_device.EndRenderPass();

	//
	//	End the render frame
	//
	m_device.EndFrame();

#if defined( GEN_LIGHTPROBE )
	BuildLightProbe( &m_device, g_offscreenFrameBuffer );
#endif

	if ( m_takeScreenshot ) {
		m_device.m_swapChain.Screenshot( &m_device );
		m_takeScreenshot = false;
	}
}