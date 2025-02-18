//
//  BuildLightTiles.cpp
//
#include "Renderer/Common.h"
#include "Renderer/BuildLightTiles.h"
#include "Models/ModelManager.h"
#include "Models/ModelStatic.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "BSP/Brush.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

Pipeline	g_buildLightTilesPipeline;
Descriptors	g_buildLightTilesDescriptors;

// The shader storage buffer for the point lights that will be used in the tiled lighting shader
// struct storageLight_t {
// 	Vec4	m_sphere;
// 	Vec4	m_color;
// };
const int g_numBaseLights = 128;
const int g_maxWorldLights = 1024;	// This should be enough lights for our needs
Buffer	g_lightsStorageBuffer;		// Storage buffer containing the list of point lights

const int g_threadWdith = 16;
int g_numLightTiles = 0;

// The shader storage buffer for the per tile light lists
const int g_maxLightsPerTile = 128 - 3;	// chose to do 31 so that the size of the struct is on a 16/64 byte boundary
//const int g_maxLightsPerTile = 256 - 3;	// chose to do 31 so that the size of the struct is on a 16/64 byte boundary
struct lightList_t {
	int m_numLights;
	float m_depthMin;
	float m_depthMax;
	int m_lightIds[ g_maxLightsPerTile ];
};
Buffer	g_lightTileStorageBuffer;	// Storage buffer containing the tiled lights


// struct buildLightTileParms_t {
// 	Mat4 matView;
// 	Mat4 matProjInv;
// 
// 	int screenWidth;
// 	int screenHeight;
// 	int maxLights;
// 	int pad0;
// };
Buffer	g_buildLightTileUniformBuffer;

//#define DEBUG_PRINT_LIGHTTILES

extern GFrameBuffer g_gbuffer;

/*
====================================================
InitLightTiles
====================================================
*/
#pragma optimize( "", off )
bool InitLightTiles( DeviceContext * device, int width, int height, brush_t * brushes, int numBrushes ) {
	bool result = false;

	int numGroupsX = ( width + g_threadWdith - 1 ) / g_threadWdith;
	int numGroupsY = ( height + g_threadWdith - 1 ) / g_threadWdith;
	g_numLightTiles = numGroupsX * numGroupsY;
	//g_numLightTiles = width * height / ( g_threadWdith * g_threadWdith );
	
	//
	//	Build Tiled Lights List pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numUniformsCompute = 1;
		descriptorParms.numSamplerImagesCompute = 1;
		descriptorParms.numStorageBuffersCompute = 2;		
		result = g_buildLightTilesDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_buildLightTilesDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/tiledBuildLightList" );
		result = g_buildLightTilesPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}
	
#if defined( DEBUG_PRINT_LIGHTTILES )
	g_lightTileStorageBuffer.Allocate( device, NULL, sizeof( lightList_t ) * g_numLightTiles, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
#else	
	g_lightTileStorageBuffer.Allocate( device, NULL, sizeof( lightList_t ) * g_numLightTiles, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
#endif
	g_lightsStorageBuffer.Allocate( device, NULL, sizeof( storageLight_t ) * g_maxWorldLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	g_buildLightTileUniformBuffer.Allocate( device, NULL, sizeof( buildLightTileParms_t ) * 4, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );


	//
	//	Initialize some random lights
	//
	Bounds bounds;
	bounds.Clear();
	for ( int i = 0; i < numBrushes; i++ ) {
		const brush_t & brush = brushes[ i ];

		for ( int j = 0; j < brush.numPlanes; j++ ) {
			const winding_t & winding = brush.windings[ j ];

			for ( int k = 0; k < winding.pts.size(); k++ ) {
				bounds.Expand( winding.pts[ k ] );
			}
		}
	}

	const int maxLights = g_numBaseLights;//( numLights < g_maxWorldLights ) ? numLights : g_maxWorldLights;

	storageLight_t * storageLights = (storageLight_t *)g_lightsStorageBuffer.MapBuffer( device );
	for ( int i = 0; i < maxLights; i++ ) {
		storageLight_t & light = storageLights[ i ];
		light.m_color = Vec4( 1, 1, 1, 1 );

		int colorID = i & 3;
		if ( 0 == colorID ) {
			light.m_color = Vec4( 1, 0, 0, 1 );
		} else if ( 1 == colorID ) {
			light.m_color = Vec4( 0, 1, 0, 1 );
		} else if ( 2 == colorID ) {
			light.m_color = Vec4( 0, 0, 1, 1 );
		}

		Vec3 pos;
		bool isInBrush = false;
		do {
			pos.x = bounds.mins.x + Random::Get() * bounds.WidthX();
			pos.y = bounds.mins.y + Random::Get() * bounds.WidthY();
			pos.z = bounds.mins.z + Random::Get() * bounds.WidthZ();

			isInBrush = false;
			for ( int j = 0; j < numBrushes; j++ ) {
				const brush_t & brush = brushes[ j ];
				isInBrush = IsPointInBrush( brush, pos );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 1, 0, 0 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3(-1, 0, 0 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 0, 1, 0 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 0,-1, 0 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 0, 0, 1 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 0, 0,-1 ) );

				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 1, 1, 1 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 1, 1,-1 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 1,-1, 1 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3( 1,-1,-1 ) );

				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3(-1, 1, 1 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3(-1, 1,-1 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3(-1,-1, 1 ) );
				isInBrush = isInBrush || IsPointInBrush( brush, pos + Vec3(-1,-1,-1 ) );
				if ( isInBrush ) {
					break;
				}
			}
		} while ( isInBrush );

		light.m_sphere.x = pos.x;
		light.m_sphere.y = pos.y;
		light.m_sphere.z = pos.z;
		light.m_sphere.w = 10.0f;
	}
	g_lightsStorageBuffer.UnmapBuffer( device );

	return result;
}
#pragma optimize( "", on )

/*
====================================================
CleanupLightTiles
====================================================
*/
void CleanupLightTiles( DeviceContext * device ) {
	g_buildLightTilesPipeline.Cleanup( device );
	g_buildLightTilesDescriptors.Cleanup( device );

	g_lightTileStorageBuffer.Cleanup( device );
	g_lightsStorageBuffer.Cleanup( device );
	g_buildLightTileUniformBuffer.Cleanup( device );
}

/*
====================================================
UpdateLights
====================================================
*/
void UpdateLights( DeviceContext * device, const storageLight_t * lights, int numLights, const buildLightTileParms_t & buildParms ) {
	//
	//	Update the light list
	//
	const int maxLights = ( ( numLights + g_numBaseLights ) < g_maxWorldLights ) ? ( numLights + g_numBaseLights ) : g_maxWorldLights;

	storageLight_t * storageLights = (storageLight_t *)g_lightsStorageBuffer.MapBuffer( device );
	for ( int i = g_numBaseLights; i < maxLights; i++ ) {
		storageLights[ i ] = lights[ i - g_numBaseLights ];
	}
	g_lightsStorageBuffer.UnmapBuffer( device );

	//
	//	Update the parms
	//
	buildLightTileParms_t * parms = (buildLightTileParms_t *)g_buildLightTileUniformBuffer.MapBuffer( device );
	{
		parms->matView = buildParms.matView;
		parms->matProjInv = buildParms.matProjInv;
		parms->screenWidth = buildParms.screenWidth;
		parms->screenHeight = buildParms.screenHeight;
		parms->maxLights = maxLights;
		parms->pad0 = 0;
	}
	g_buildLightTileUniformBuffer.UnmapBuffer( device );

	// Reset Light Tiles to zero
// 	lightList_t * lightTiles = (lightList_t *)g_lightTileStorageBuffer.MapBuffer( device );
// 	memset( lightTiles, 0, sizeof( lightList_t ) * g_numLightTiles );
// 	g_lightTileStorageBuffer.UnmapBuffer( device );
}

/*
====================================================
BuildLightTiles
====================================================
*/
void BuildLightTiles( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 4;

	const int width = g_offscreenFrameBuffer.m_parms.width;
	const int height = g_offscreenFrameBuffer.m_parms.height;
	
	//
	//	Fill our per tile light lists
	//
	{
		g_buildLightTilesPipeline.BindPipelineCompute( cmdBuffer );

		Descriptor descriptor = g_buildLightTilesPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( &g_buildLightTileUniformBuffer, 0, sizeof( buildLightTileParms_t ), 0 );
		descriptor.BindImage( g_gbuffer.m_imageDepth, Samplers::m_samplerDepth, 1 );
		descriptor.BindStorageBuffer( &g_lightsStorageBuffer, 0, g_lightsStorageBuffer.m_vkBufferSize, 2 );
		descriptor.BindStorageBuffer( &g_lightTileStorageBuffer, 0, g_lightTileStorageBuffer.m_vkBufferSize, 3 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_buildLightTilesPipeline );

		int numGroupsX = ( width + g_threadWdith - 1 ) / g_threadWdith;
		int numGroupsY = ( height + g_threadWdith - 1 ) / g_threadWdith;
		g_buildLightTilesPipeline.DispatchCompute( cmdBuffer, numGroupsX, numGroupsY, 1 );
		//g_buildLightTilesPipeline.DispatchCompute( cmdBuffer, width / g_threadWdith, height / g_threadWdith, 1 );
	}
	
	//
	//	Debug map and check light list
	//
#if defined( DEBUG_PRINT_LIGHTTILES )
	{
		MemoryBarriers::CreateMemoryBarrier( cmdBuffer );
		printf( "\n--------------------Begin Light list---------------------------\n" );

		lightList_t * lightList = ( lightList_t * )g_lightTileStorageBuffer.MapBuffer( device );
		if ( lightList ) {
			for ( int i = 0; i < g_numLightTiles; ++i ) {
				const lightList_t & list = lightList[ i ];
				if ( list.m_numLights > 0 ) {
					printf( "Tile: %i  DepthMin: %.5f  DepthMax: %.5f   numLights %i\n", i, list.m_depthMin, list.m_depthMax, list.m_numLights );
				}
			}
			g_lightTileStorageBuffer.UnmapBuffer( device );
		}
		printf( "--------------------End Light list---------------------------\n" );
	}
#endif
}


/*
========================================================================================================

Debug Draw Light Tiles

========================================================================================================
*/


FrameBuffer	g_debugDrawLightFrameBuffer;

Pipeline	g_debugDrawLightTilesPipeline;
Descriptors	g_debugDrawLightTilesDescriptors;

Model * g_debugDrawLightTileFullscreen = NULL;


/*
====================================================
InitDebugDrawLightTiles
====================================================
*/
bool InitDebugDrawLightTiles( DeviceContext * device, int width, int height ) {
	bool result = false;

	g_debugDrawLightTileFullscreen = g_modelManager->AllocateModel();
	FillFullScreenQuad( *g_debugDrawLightTileFullscreen );
	g_debugDrawLightTileFullscreen->MakeVBO( device );

	//
	//	Build framebuffer
	//
	{
		FrameBuffer::CreateParms_t frameBufferParms;
		frameBufferParms.width = width;
		frameBufferParms.height = height;
		frameBufferParms.hasColor = true;
		frameBufferParms.hasDepth = false;
		result = g_debugDrawLightFrameBuffer.Create( device, frameBufferParms );
		if ( !result ) {
			printf( "Unable to create off screen buffer!\n" );
			assert( 0 );
			return false;
		}

// 		VkCommandBuffer cmdBuffer = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );
// 		g_debugDrawLightFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
// 		device->FlushCommandBuffer( cmdBuffer, device->m_vkGraphicsQueue );

	}
	
	//
	//	Build pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numStorageBuffersFragment = 1;
		result = g_debugDrawLightTilesDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_debugDrawLightFrameBuffer;
		pipelineParms.descriptors = &g_debugDrawLightTilesDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/DebugDrawLightTiles" );
		pipelineParms.width = g_debugDrawLightFrameBuffer.m_parms.width;
		pipelineParms.height = g_debugDrawLightFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = g_debugDrawLightTilesPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}
	
	return result;
}

/*
====================================================
CleanupDebugDrawLightTiles
====================================================
*/
void CleanupDebugDrawLightTiles( DeviceContext * device ) {
	g_debugDrawLightFrameBuffer.Cleanup( device );

	g_debugDrawLightTilesPipeline.Cleanup( device );
	g_debugDrawLightTilesDescriptors.Cleanup( device );
}


/*
====================================================
DebugDrawLightTiles
====================================================
*/
void DebugDrawLightTiles( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	//
	//	Draw colored tiles based upon the number of lights in each tile
	//
	g_debugDrawLightFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	g_debugDrawLightFrameBuffer.BeginRenderPass( device, cmdBufferIndex, true, true );
	{
		g_debugDrawLightTilesPipeline.BindPipeline( cmdBuffer );
		Descriptor descriptor = g_debugDrawLightTilesPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( &g_buildLightTileUniformBuffer, 0, sizeof( buildLightTileParms_t ), 0 );
		descriptor.BindStorageBuffer( &g_lightTileStorageBuffer, 0, g_lightTileStorageBuffer.m_vkBufferSize, 1 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_debugDrawLightTilesPipeline );

		g_debugDrawLightTileFullscreen->DrawIndexed( cmdBuffer );
	}
	g_debugDrawLightFrameBuffer.EndRenderPass( device, cmdBufferIndex );
	g_debugDrawLightFrameBuffer.m_imageColor.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
}


/*
========================================================================================================

Draw Tiled GGX

========================================================================================================
*/


Pipeline	g_drawTiledGGXPipeline;
Descriptors	g_drawTiledGGXDescriptors;

extern Image g_imageDiffuse;
extern Image g_imageNormals;
extern Image g_imageGloss;
extern Image g_imageSpecular;

/*
====================================================
InitDrawTiledGGX
====================================================
*/
bool InitDrawTiledGGX( DeviceContext * device, int width, int height ) {
	bool result = false;

	//
	//	ggx pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 1;
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 3;
		descriptorParms.numStorageBuffersFragment = 2;
		result = g_drawTiledGGXDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_drawTiledGGXDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/tiledDeferredGGX" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = g_drawTiledGGXPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	return result;
}

/*
====================================================
CleanupDrawTiledGGX
====================================================
*/
void CleanupDrawTiledGGX( DeviceContext * device ) {
	g_drawTiledGGXPipeline.Cleanup( device );
	g_drawTiledGGXDescriptors.Cleanup( device );
}

/*
====================================================
DrawTiledGGX
====================================================
*/
void DrawTiledGGX( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 4;

	//
	//	Draw colored tiles based upon the number of lights in each tile
	//
	{
		g_drawTiledGGXPipeline.BindPipeline( cmdBuffer );
		Descriptor descriptor = g_drawTiledGGXPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );
		descriptor.BindBuffer( &g_buildLightTileUniformBuffer, 0, sizeof( buildLightTileParms_t ), 1 );

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 2 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 3 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 4 );

		descriptor.BindStorageBuffer( &g_lightsStorageBuffer, 0, g_lightsStorageBuffer.m_vkBufferSize, 5 );
		descriptor.BindStorageBuffer( &g_lightTileStorageBuffer, 0, g_lightTileStorageBuffer.m_vkBufferSize, 6 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_drawTiledGGXPipeline );

		g_debugDrawLightTileFullscreen->DrawIndexed( cmdBuffer );
	}
}
