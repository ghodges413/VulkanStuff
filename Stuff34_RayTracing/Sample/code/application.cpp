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
#include "Math/Frustum.h"
#include "Miscellaneous/Fileio.h"
#include "Miscellaneous/Time.h"
#include "Miscellaneous/Input.h"
#include "BSP/Map.h"
#include "Physics/Body.h"
#include "Entities/Player.h"
#include "Scenes/SceneGame.h"
#include "Renderer/Common.h"
#include "Renderer/DrawPreDepth.h"
#include "Renderer/DrawOffscreen.h"
#include "Renderer/DrawGBuffer.h"
#include "Renderer/BuildLightTiles.h"
#include "Renderer/DrawSkybox.h"
#include "Renderer/DrawLightprobe.h"
#include "Renderer/BuildAmbient.h"
#include "Renderer/DrawAmbient.h"
#include "Renderer/DrawAmbientAO.h"
#include "Renderer/BuildLightProbe.h"
#include "Renderer/DrawShadows.h"
#include "Renderer/DrawSSR.h"
#include "Renderer/DrawSSAO.h"
#include "Renderer/DrawDecals.h"
#include "Renderer/DrawSubsurface.h"
#include "Renderer/DrawTAA.h"
#include "Renderer/DrawTransparent.h"
#include "Renderer/DrawAtmosphere.h"
#include "Renderer/DrawOcean.h"
#include "Renderer/DrawRaytracing.h"
#include <assert.h>

Application * g_application = NULL;

extern Player * g_player;

//#define USE_SSAO_MULTIPLY	// Only available on Nvidia GPUS

#define RENDER_BRUSHES

std::vector< RenderModel > g_renderBrushes;

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
	m_mapBounds.Clear();
	for ( int i = 0; i < g_brushes.size(); i++ ) {
		const brush_t & brush = g_brushes[ i ];

		for ( int w = 0; w < brush.numPlanes; w++ ) {
			const winding_t & winding = brush.windings[ w ];

			for ( int v = 0; v < winding.pts.size(); v++ ) {
				Vec3 pt = winding.pts[ v ];
				m_mapBounds.Expand( pt );
			}
		}
	}

	Vec3 fwd[ 6 ];
	fwd[ 0 ] = Vec3( 1, 0, 0 );
	fwd[ 1 ] = Vec3(-1, 0, 0 );
	fwd[ 2 ] = Vec3( 0, 1, 0 );	
	fwd[ 3 ] = Vec3( 0,-1, 0 );
	fwd[ 4 ] = Vec3( 0, 0, 1 );
	fwd[ 5 ] = Vec3( 0, 0, -1 );

	//
	//	Add "entities" for every position that is inside a brush
	//
	m_entities.clear();
	for ( float z = m_mapBounds.mins.z + 1.0f; z < m_mapBounds.maxs.z + 1.0f; z += 1.0f ) {
		for ( float y = m_mapBounds.mins.y; y < m_mapBounds.maxs.y; y += 1.0f ) {
			for ( float x = m_mapBounds.mins.x; x < m_mapBounds.maxs.x; x += 1.0f ) {
				Vec3 pos = Vec3( x, y, z );

				// Check if this position is inside any of the brushes
				for ( int i = 0; i < g_brushes.size(); i++ ) {
					const brush_t & brush = g_brushes[ i ];
					if ( IsPointInBrush( brush, pos ) ) {
						int idxFwd = rand() % 6;
						int idxUp = ( idxFwd < 4 ) ? ( rand() % 2 ) + 4 : rand() % 4;
						if ( idxFwd < 4 ) {
							idxUp = 4;
						}

						entity.fwd = fwd[ idxFwd ];
						entity.up = fwd[ idxUp ];
						entity.pos = pos;
						entity.isTransparent = false;

// 						entity.fwd = Vec3( 1, 0, 0 );
// 						entity.up = Vec3( 0, 0, 1 );
						m_entities.push_back( entity );
						break;
					}
				}
			}
		}
	}

#if defined( RENDER_BRUSHES )
	m_entities.clear();
	for ( int i = 0; i < g_brushes.size(); i++ ) {
		entity.fwd = Vec3( 1, 0, 0 );
		entity.up = Vec3( 0, 0, 1 );
		entity.pos = Vec3( 0, 0, 0 );
		entity.isTransparent = false;
		m_entities.push_back( entity );

		

		const brush_t & brush = g_brushes[ i ];

		Model * model = new Model;
		model->BuildFromBrush( &brush );
		model->MakeVBO( &m_device );

		RenderModel renderModel;
		renderModel.modelDraw = model;
		renderModel.orient = Quat( 0, 0, 0, 1 );
		renderModel.pos = Vec3( 0, 0, 0 );
		g_renderBrushes.push_back( renderModel );
	}
#endif

	// Add transparent entities
// 	entity.fwd = Vec3( 0, 1, 0 );
// 	entity.up = Vec3( 0, 0, 1 );
// 	entity.pos = Vec3( -12, -5.5f, 2.0f );
// 	entity.isTransparent = true;
// 	m_entities.push_back( entity );
// 
// 	entity.pos = Vec3( -12, -5, 2 );
// 	m_entities.push_back( entity );
// 
// 	entity.pos = Vec3( -12, -6, 2 );
// 	m_entities.push_back( entity );

	// Add moving entity
	entity.fwd = Vec3( 0, 1, 0 );
	entity.up = Vec3( 0, 0, 1 );
	entity.pos = Vec3( -16, 0, 2.0f );
	entity.isTransparent = false;
	m_entities.push_back( entity );

	int windowWidth;
	int windowHeight;
	glfwGetWindowSize( m_glfwWindow, &windowWidth, &windowHeight );
	InitLightTiles( &m_device, windowWidth, windowHeight, g_brushes.data(), g_brushes.size() );
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
	m_uniformBuffer[ 0 ].Allocate( &m_device, NULL, sizeof( float ) * 16 * 4 * 4096, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	m_uniformBuffer[ 1 ].Allocate( &m_device, NULL, sizeof( float ) * 16 * 4 * 4096, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	m_unformModelIDs.Allocate( &m_device, NULL, sizeof( int ) * 16 * 4 * 4096, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	m_storageBufferOrientsRTX.Allocate( &m_device, NULL, sizeof( float ) * 16 * 4 * 4096, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	//
	//	Render passes
	//
	InitOffscreen( &m_device, windowWidth, windowHeight );
	InitDepthPrePass( &m_device );	
	InitGBuffer( &m_device, windowWidth, windowHeight );
//	InitLightTiles( &m_device, windowWidth, windowHeight, g_brushes.data(), g_brushes.size() );
	InitDebugDrawLightTiles( &m_device, windowWidth, windowHeight );
	InitDrawTiledGGX( &m_device, windowWidth, windowHeight );
	InitSkybox( &m_device, windowWidth, windowHeight );
#if !defined( GEN_LIGHTPROBE )
	InitLightProbe( &m_device, windowWidth, windowHeight );
#endif
#if defined( USE_SSAO_MULTIPLY )
	InitAmbient( &m_device, windowWidth, windowHeight );
	InitSSAO( &m_device, windowWidth, windowHeight );
#else
	InitAmbientAO( &m_device, windowWidth, windowHeight );
#endif
	InitShadows( &m_device );
	InitScreenSpaceReflections( &m_device, windowWidth, windowHeight );
	InitDecals( &m_device );
	InitPreDepth( &m_device, windowWidth, windowHeight );
	InitSubsurface( &m_device, windowWidth, windowHeight );
	InitTemporalAntiAliasing( &m_device, windowWidth, windowHeight );
	InitTransparent( &m_device, windowWidth, windowHeight );
	InitAtmosphere( &m_device, windowWidth, windowHeight );
	InitOceanSimulation( &m_device );
	InitRaytracing( &m_device, windowWidth, windowHeight );

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

	for ( int i = 0; i < g_renderBrushes.size(); i++ ) {
		g_renderBrushes[ i ].modelDraw->Cleanup();
		delete g_renderBrushes[ i ].modelDraw;
	}

	m_pipelineCopy.Cleanup( &m_device );
	m_descriptorsCopy.Cleanup( &m_device );

	// Delete Uniform Buffer Memory
	m_uniformBuffer[ 0 ].Cleanup( &m_device );
	m_uniformBuffer[ 1 ].Cleanup( &m_device );
	m_unformModelIDs.Cleanup( &m_device );

	m_storageBufferOrientsRTX.Cleanup( &m_device );

	CleanupDepthPrePass( &m_device );
	CleanupOffscreen( &m_device );
	CleanupGBuffer( &m_device );
	CleanupLightTiles( &m_device );
	CleanupDebugDrawLightTiles( &m_device );
	CleanupDrawTiledGGX( &m_device );
	CleanupSkybox( &m_device );
#if !defined( GEN_LIGHTPROBE )
	CleanupLightProbe( &m_device );
#endif
#if defined( USE_SSAO_MULTIPLY )
	CleanupAmbient( &m_device );
	CleanupSSAO( &m_device );
#else
	CleanupAmbientAO( &m_device );
#endif
	CleanupShadows( &m_device );
	CleanupScreenSpaceReflections( &m_device );
	CleanupDecals( &m_device );
	CleanupPreDepth( &m_device );
	CleanupSubsurface( &m_device );
	CleanupTemporalAntiAliasing( &m_device );
	CleanupTransparent( &m_device );
	CleanupAtmosphere( &m_device );
	CleanupOceanSimulation( &m_device );
	CleanupRaytracing( &m_device );

	m_modelFullScreen.Cleanup();

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

	// TODO: Destroy and rebuild any pipelines that also need resizing
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
float g_dt_sec = 0;
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
		g_dt_sec = dt_sec;

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


void PrintMat4( const Mat4 & mat ) {
	for ( int i = 0; i < 4; i++ ) {
		printf( "%.4f, %.4f, %.4f, %.4f,\n", mat.rows[ i ].x, mat.rows[ i ].y, mat.rows[ i ].z, mat.rows[ i ].w );
	}
}

const Vec2 g_halton[ 16 ] = {
	Vec2( 0.500000f, 0.333333f ),
	Vec2( 0.250000f, 0.666667f ),
	Vec2( 0.750000f, 0.111111f ),
	Vec2( 0.125000f, 0.444444f ),
	Vec2( 0.625000f, 0.777778f ),
	Vec2( 0.375000f, 0.222222f ),
	Vec2( 0.875000f, 0.555556f ),
	Vec2( 0.062500f, 0.888889f ),
	Vec2( 0.562500f, 0.037037f ),
	Vec2( 0.312500f, 0.370370f ),
	Vec2( 0.812500f, 0.703704f ),
	Vec2( 0.187500f, 0.148148f ),
	Vec2( 0.687500f, 0.481481f ),
	Vec2( 0.437500f, 0.814815f ),
	Vec2( 0.937500f, 0.259259f ),
	Vec2( 0.031250f, 0.592593f ),
};
int g_frame = 0;

Camera_t g_camera;
Frustum g_frustum;

/*
====================================================
Application::UpdateUniforms
====================================================
*/
void Application::UpdateUniforms() {
	m_renderModels.clear();
	g_frame++;

	uint32_t uboByteOffset = 0;

	Camera_t camera;

	Vec3 camPos = Vec3( 10, 0, 5 ) * 1.25f;
	Vec3 camLookAt = Vec3( 0, 0, 1 );
	Vec3 camUp = Vec3( 0, 0, 1 );

	//
	//	Update the uniform buffers
	//
	{
		unsigned char * mappedData = (unsigned char *)m_uniformBuffer[ g_frame % 2 ].MapBuffer( &m_device );
		unsigned char * mappedModelIDs = (unsigned char *)m_unformModelIDs.MapBuffer( &m_device );
		Mat4 * mappedOrientsRTX = (Mat4*)m_storageBufferOrientsRTX.MapBuffer( &m_device );

		//
		// Update the uniform buffer with the camera information
		//
		{
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

// 			camPos = Vec3( -5.51734829, -10.3850698, 2.00399995 );
// 			camLookAt = Vec3( -6.38830996, -9.89649677, 1.95182037 );

// 			camPos = Vec3( -4.52500010, -4.73808956, 1.99951208 );
// 			camLookAt = Vec3( -3.87189722, -3.98253512, 1.94858062 );

			// top of stairs
// 			camPos = Vec3( 14.6427755, -1.23668635, 4.00399971 );
// 			camLookAt = Vec3( 13.6438780, -1.22828746, 3.95781255 );

			// skull position
// 			camPos = Vec3( -14.7821207, 0.0609404333, 2.00399995 );
// 			camLookAt = Vec3( -15.7800989, -0.00261096284, 2.00404620 );

			Vec3 left = Vec3( 0, 0, 1 ).Cross( lookDir );
			lookDir.z = 0.0f;
			left.z = 0.0f;
			lookDir.Normalize();
			left.Normalize();
					

			int windowWidth;
			int windowHeight;
			glfwGetWindowSize( m_glfwWindow, &windowWidth, &windowHeight );

			const float zNear   = 0.25f;
			const float zFar    = 500.0f;
			float fovy	= 60;
			const float aspect	= (float)windowHeight / (float)windowWidth;

#if defined( GEN_LIGHTPROBE )
			camPos = GetLightProbePos();
			camLookAt = GetLightProbeLookAt();
			camUp = GetLightProbeUp();
			fovy = 90;
#endif
#if defined( GEN_AMBIENT )
			camPos = GetAmbientPos();
			camLookAt = GetAmbientLookAt();
			camUp = Vec3( 0, 0, 1 );
			fovy = 90;
#endif
			// Build the camera frustum
			Vec3 camLook = camLookAt - camPos;
			camLook.Normalize();
			g_frustum.Build( zNear, zFar, fovy, windowWidth, windowHeight, camPos, camUp, camLook );

			// For TAA, we add a subpixel offset to the camera position
			Mat4 matJitterTAA;
			matJitterTAA.Identity();

#if defined( USE_TAA )
			int idx = g_frame % 16;
			//idx = 2;
			Vec2 offset = g_halton[ idx ];
			offset -= Vec2( 0.5f, 0.5f );
			offset *= 2.0f;
			offset.x *= 1.0f / float( windowWidth );
			offset.y *= 1.0f / float( windowHeight );
			matJitterTAA.rows[ 0 ] = Vec4( 1, 0, 0, offset.x );
			matJitterTAA.rows[ 1 ] = Vec4( 0, 1, 0, offset.y );
			matJitterTAA.rows[ 2 ] = Vec4( 0, 0, 1, 0 );
			matJitterTAA.rows[ 3 ] = Vec4( 0, 0, 0, 1 );
#endif
			g_camera.matProj.PerspectiveVulkan( fovy, aspect, zNear, zFar );
			g_camera.matView.LookAt( camPos, camLookAt, camUp );

			camera.matProj = matJitterTAA * g_camera.matProj;	// Add the sub-pixel jitter for TAA
			camera.matProj = camera.matProj.Transpose();	// Vulkan is column major
			camera.matView = g_camera.matView.Transpose();	// Vulkan is column major

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
		Model * modelBlock = g_modelManager->GetModel( "../../common/data/meshes/static/block_a_lp.obj" );
		Model * modelSkull = g_modelManager->GetModel( "../../common/data/meshes/static/skull_a_lp.obj" );

		int numTransparent = 0;
		int uboByteOffsetModelIDs = 0;
		for ( int i = 0; i < m_entities.size(); i++ ) {
			Entity_t & entity = m_entities[ i ];

			Mat4 matOrient;
			matOrient.Orient( entity.pos, entity.fwd, entity.up );
			matOrient = matOrient.Transpose();	// Vulkan is column major

			Mat3 matRot;
			matRot.Orient( entity.fwd, entity.up );

			RenderModel renderModel;
			renderModel.modelDraw = modelBlock;
			renderModel.orient = Quat( matRot );
#if defined( RENDER_BRUSHES )
			if ( i < g_renderBrushes.size() ) {
				renderModel = g_renderBrushes[ i ];
			}
#endif
			if ( entity.isTransparent ) {
				renderModel.isTransparent = true;

				int color = numTransparent % 3;
				if ( 0 == color ) {
					renderModel.transparentColor = Vec4( 1, 0, 0, 0.75f );
				} else if ( 1 == color ) {
					renderModel.transparentColor = Vec4( 0, 1, 0, 0.75f );
				} else {
					renderModel.transparentColor = Vec4( 0, 0, 1, 0.75f );
				}

				numTransparent++;
				renderModel.modelDraw = modelSkull;
			}
			if ( ( m_entities.size() - 1 ) == i ) {
				renderModel.subsurfaceId = 1;
				renderModel.modelDraw = modelSkull;

// 				Vec3 pos = entity.pos;
// 				pos.y += cosf( m_timeAccumuledSeconds * 0.1f );
// 				matOrient.Orient( pos, entity.fwd, entity.up );
// 				matOrient = matOrient.Transpose();	// Vulkan is column major
			}
			renderModel.uboByteOffset = uboByteOffset;
			renderModel.uboByteSize = sizeof( matOrient );
			renderModel.uboByteOffsetModelID = uboByteOffsetModelIDs;
			renderModel.uboByteSizeModelID = sizeof( int ) * 4;
			renderModel.pos = entity.pos;
			m_renderModels.push_back( renderModel );


			// Copy the model ID's into the ubo
			int ivec4[ 4 ];
			ivec4[ 0 ] = i + 1;
			ivec4[ 1 ] = i + 1;
			ivec4[ 2 ] = i + 1;
			ivec4[ 3 ] = i + 1;
			memcpy( mappedModelIDs + uboByteOffsetModelIDs, ivec4, sizeof( int ) * 4 );
			uboByteOffsetModelIDs += m_device.GetAlignedUniformByteOffset( sizeof( int ) * 4 );

			// Update the uniform buffer with the orientation of this entity
			memcpy( mappedData + uboByteOffset, matOrient.ToPtr(), sizeof( matOrient ) );
			uboByteOffset += m_device.GetAlignedUniformByteOffset( sizeof( matOrient ) );
			if ( uboByteOffset >= m_uniformBuffer[ g_frame % 2 ].m_vkBufferSize ) {
				break;
			}

			// Update the storage buffer for the rtx
			mappedOrientsRTX[ i ] = matOrient;
		}

		m_uniformBuffer[ g_frame % 2 ].UnmapBuffer( &m_device );
		m_unformModelIDs.UnmapBuffer( &m_device );
		m_storageBufferOrientsRTX.UnmapBuffer( &m_device );
	}
	
	//
	//	Update Lights
	//
	{
		std::vector< storageLight_t > lights;

		Vec3 pos = Vec3( -16, 0, 2.5f );
		pos.y += sinf( m_timeAccumuledSeconds * 1.1f ) * 4.0f;
		storageLight_t light;
		light.m_color = Vec4( 1.0f, 0.0f, 1.0f, 10.0f );
		light.m_sphere = Vec4( pos.x, pos.y, pos.z, 5.0f );
		lights.push_back( light );

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

	UpdateAtmosphere( &m_device, g_camera, g_dt_sec, m_mapBounds );
	UpdateOceanParms( &m_device, g_dt_sec, g_camera.matView, g_camera.matProj, camPos, camLookAt );
	UpdateRaytracing( &m_device, m_renderModels.data(), m_renderModels.size() );
}

/*
====================================================
Application::DrawFrame
====================================================
*/
void Application::DrawFrame() {
	UpdateUniforms();

	std::vector< RenderModel > culledRenderModels;
	culledRenderModels.reserve( m_renderModels.size() );

	for ( int i = 0; i < m_renderModels.size(); i++ ) {
		const RenderModel & renderModel = m_renderModels[ i ];
		const Vec3 & pos = renderModel.pos;
		Bounds bounds = renderModel.modelDraw->m_bounds;

		bounds.mins += pos;
		bounds.maxs += pos;

		//if ( g_frustum.DoBoundsIntersectFrustum( bounds ) ) 
		{
			culledRenderModels.push_back( m_renderModels[ i ] );
		}
	}
	printf( "   Num models culled: %i", m_renderModels.size() - culledRenderModels.size() );
	printf( "   Num models drawn: %i", culledRenderModels.size() );

	//
	//	Begin the render frame
	//
	const uint32_t imageIndex = m_device.BeginFrame();
	VkCommandBuffer cmdBuffer = m_device.m_vkCommandBuffers[ imageIndex ];

	DrawParms_t parms;
	parms.device = &m_device;
	parms.cmdBufferIndex = imageIndex;
	parms.uniforms = &m_uniformBuffer[ g_frame % 2 ];
	parms.uniformsOld = &m_uniformBuffer[ ( g_frame + 1 ) % 2 ];
	parms.uniformsModelIDs = &m_unformModelIDs;
	parms.storageOrientsRTX = &m_storageBufferOrientsRTX;
	parms.renderModels = culledRenderModels.data();
	parms.numModels = (int)culledRenderModels.size();
	parms.notCulledRenderModels = m_renderModels.data();
	parms.numNotCulledRenderModels = (int)m_renderModels.size();
	parms.time = m_timeAccumuledSeconds;
	parms.camera = g_camera;

//	UpdateOceanSimulation( parms );
	UpdateSunShadowDescriptors( parms );	

	// Update the shadows
	UpdateShadows( parms );
	UpdateSunShadow( parms );
	
	DrawPreDepth( parms );

	//
	//	Fill the g-buffer
	//
#if defined( RENDER_BRUSHES )
	UpdateGBufferCheckerBoardDescriptors( parms );
#else
	UpdateGBufferDescriptors( parms );
#endif
	UpdateDecalDescriptors( parms );
	g_gbuffer.BeginRenderPass( parms.device, parms.cmdBufferIndex );

#if defined( RENDER_BRUSHES )
	DrawGBufferCheckerBoard( parms );
#else
	DrawGBuffer( parms );
#endif
//	DrawOceanDeferred( parms );

//	DrawDecals( parms );

	g_gbuffer.EndRenderPass( parms.device, parms.cmdBufferIndex );

	// Use the g-buffer in a compute shader to build the list of tiled lights
	BuildLightTiles( parms );

	// For debug purposes, draw the number of lights in each tile
	DebugDrawLightTiles( parms );

#if defined( USE_TAA ) || defined( ENABLE_RAYTRACING )
	CalculateVelocityBuffer( parms );	// The velocity buffer is required for both TAA and raytracing
#endif

	DrawRaytracing( parms );

	//
	//	Draw to Offscreen Buffer
	//
	UpdateShadowDescriptors( parms );
	g_offscreenFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_offscreenFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	g_offscreenFrameBuffer.BeginRenderPass( &m_device, imageIndex, true, true );

	DrawDepthPrePass( parms );
	
//	DrawOffscreen( parms );	// this is an unlit forward rendering to the off screen buffer, only used in older samples

//	DrawSkybox( parms );
	DrawAtmosphere( parms );
//	DrawOcean( parms );

#if !defined( GEN_AMBIENT )
#if defined( USE_SSAO_MULTIPLY )
	DrawAmbient( parms );
	DrawSSAO( parms );
#else
	DrawAmbientAO( parms );
#endif	
#endif

	// Use the g-buffer and list of tiled lights to draw to the off-screen buffer
	DrawTiledGGX( parms );

#if !defined( GEN_LIGHTPROBE ) && !defined( USE_SSR ) && !defined( GEN_AMBIENT )
	DrawLightProbe( parms );
#endif

	DrawShadows( parms );

	DrawSubsurface( parms );

	DrawTransparent( parms );

	g_offscreenFrameBuffer.EndRenderPass( &m_device, imageIndex );
	g_offscreenFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_offscreenFrameBuffer.m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL );

#if defined( USE_SSR )
	// Fill a buffer with the screen space reflections
	DrawScreenSpaceReflections( parms );
#endif

#if defined( USE_TAA )
	DrawTemporalAntiAliasing( parms );
#endif

	//
	//	Draw the offscreen framebuffer to the swap chain frame buffer
	//
 	m_device.BeginRenderPass();
	{
		// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
		m_pipelineCopy.BindPipeline( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = m_pipelineCopy.GetFreeDescriptor();
#if defined( ENABLE_RAYTRACING ) && !defined( USE_TAA )
		extern Image g_rtxImage;	// Lighting buffer
		extern Image g_rtxImageHistory;
		extern Image g_rtxImageAccumulated;
		extern Image g_rtxImageDenoised;
		extern Image * g_rtxImageOut;
		//descriptor.BindImage( g_rtxImage, Samplers::m_samplerStandard, 0 );
		//descriptor.BindImage( g_rtxImageHistory, Samplers::m_samplerStandard, 0 );
		//descriptor.BindImage( g_rtxImageAccumulated, Samplers::m_samplerStandard, 0 );
		descriptor.BindImage( g_rtxImageDenoised, Samplers::m_samplerStandard, 0 );
		if ( g_rtxImageOut ) {
			descriptor.BindImage( *g_rtxImageOut, Samplers::m_samplerStandard, 0 );
		}
#elif defined( USE_SSR ) && !defined( USE_TAA )
		descriptor.BindImage( g_ssrFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
#elif defined( USE_TAA )
		extern FrameBuffer * g_taaFrameBufferPtr;
		extern FrameBuffer * g_taaVelocityBufferPtr;
		descriptor.BindImage( g_taaFrameBufferPtr->m_imageColor, Samplers::m_samplerStandard, 0 );

		extern Image g_rtxImage;	// Lighting buffer
		extern Image g_rtxImageDenoised;
		//descriptor.BindImage( g_rtxImage, Samplers::m_samplerStandard, 0 );
		//descriptor.BindImage( g_rtxImageDenoised, Samplers::m_samplerStandard, 0 );

//		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 0 );

// 		extern FrameBuffer		g_sunShadowFrameBuffer;
// 		descriptor.BindImage( g_sunShadowFrameBuffer.m_imageDepth, Samplers::m_samplerStandard, 0 );
#else
		descriptor.BindImage( g_offscreenFrameBuffer.m_imageColor, Samplers::m_samplerStandard, 0 );
#endif	
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
#if defined( GEN_AMBIENT )
	BuildAmbient( &m_device, g_offscreenFrameBuffer );
#endif

	if ( m_takeScreenshot ) {
		m_device.m_swapChain.Screenshot( &m_device );
		m_takeScreenshot = false;
	}
}