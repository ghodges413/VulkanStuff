//
//  DrawShadows.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawShadows.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include "Math/Math.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

 

#define MAX_SHADOWS 4

FrameBuffer g_shadowFrameBuffers[ MAX_SHADOWS ];

Pipeline	g_shadowUpdatePipelines[ MAX_SHADOWS ];
Descriptors	g_shadowUpdateDescriptors[ MAX_SHADOWS ];

Pipeline	g_shadowDrawPipeline;
Descriptors g_shadowDrawDescriptors;

Buffer g_shadowCameraUniforms;

Model g_shadowModel;	// clip space model (use the inverse projection and inverse view matrices to transform it into world space)

struct shadowLight_t {
	Vec3 pos;
	Vec3 dir;
	float fov;
	float near;
	float far;
	float dim;
};

shadowLight_t g_shadowLights[ MAX_SHADOWS ];

struct shadowCamera_t {
	Mat4 matView;
	Mat4 matProj;
};


/*
====================================================
MakeShadowModel
====================================================
*/
void MakeShadowModel( DeviceContext * device ) {
	FillCube( g_shadowModel );
	for ( int i = 0; i < g_shadowModel.m_vertices.size(); i++ ) {		
		float z = g_shadowModel.m_vertices[ i ].pos.z;
		g_shadowModel.m_vertices[ i ].pos.z = Math::Clampf( z, 0.0f, 1.0f );
	}
	g_shadowModel.MakeVBO( device );
}

/*
====================================================
InitShadows
====================================================
*/
bool InitShadows( DeviceContext * device ) {
	MakeShadowModel( device );

	bool result = false;

	Bounds bounds( Vec3( -35, -23, -1 ), Vec3( 20, 23, 11 ) );
	bounds.mins *= 1.5f;
	bounds.maxs *= 1.5f;

	Vec3 center = bounds.Center();
	center.z = bounds.mins.z;

	g_shadowLights[ 0 ].pos = Vec3( bounds.mins.x, bounds.mins.y, bounds.maxs.z );
	g_shadowLights[ 1 ].pos = Vec3( bounds.maxs.x, bounds.mins.y, bounds.maxs.z );
	g_shadowLights[ 2 ].pos = Vec3( bounds.maxs.x, bounds.maxs.y, bounds.maxs.z );
	g_shadowLights[ 3 ].pos = Vec3( bounds.mins.x, bounds.maxs.y, bounds.maxs.z );

	for ( int i = 0; i < MAX_SHADOWS; i++ ) {
		g_shadowLights[ i ].near =10;
		g_shadowLights[ i ].far = 90;
		g_shadowLights[ i ].fov = 45;
		g_shadowLights[ i ].dim = 1024;

		Vec3 b = center;
		Vec3 a = g_shadowLights[ i ].pos;
		Vec3 dir = b - a;
		dir.Normalize();
		g_shadowLights[ i ].dir = dir;
	}

	shadowCamera_t cameras[ MAX_SHADOWS ];
	for ( int i = 0; i < MAX_SHADOWS; i++ ) {
		Vec3 pos = g_shadowLights[ i ].pos;
		Vec3 dir = g_shadowLights[ i ].dir;
		Vec3 lookat = pos + dir;
		Vec3 up = Vec3( 0, 0, 1 );
		Vec3 left = up.Cross( dir );
		up = dir.Cross( left );
		up.Normalize();

		cameras[ i ].matView.LookAt( g_shadowLights[ i ].pos, lookat, up );
		cameras[ i ].matProj.PerspectiveVulkan( g_shadowLights[ i ].fov, 1, g_shadowLights[ i ].near, g_shadowLights[ i ].far );

		cameras[ i ].matView = cameras[ i ].matView.Transpose();
		cameras[ i ].matProj = cameras[ i ].matProj.Transpose();
	}
	g_shadowCameraUniforms.Allocate( device, cameras, sizeof( shadowCamera_t ) * MAX_SHADOWS, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	for ( int i = 0; i < MAX_SHADOWS; i++ ) {
		FrameBuffer::CreateParms_t parms;
		parms.hasColor = false;
		parms.hasDepth = true;
		parms.height = g_shadowLights[ i ].dim;
		parms.width = g_shadowLights[ i ].dim;
		g_shadowFrameBuffers[ i ].Create( device, parms );
	}

	//
	//	shadow update pipeline
	//
	for ( int i = 0; i < MAX_SHADOWS; i++ ) {
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		result = g_shadowUpdateDescriptors[ i ].Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_shadowFrameBuffers[ i ];
		pipelineParms.descriptors = &g_shadowUpdateDescriptors[ i ];
		pipelineParms.shader = g_shaderManager->GetShader( "depthOnly" );
		pipelineParms.width = g_shadowFrameBuffers[ i ].m_parms.width;
		pipelineParms.height = g_shadowFrameBuffers[ i ].m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_FRONT;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		result = g_shadowUpdatePipelines[ i ].Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	shadow draw pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		//descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 4;
		result = g_shadowDrawDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_shadowDrawDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Lighting/Shadowed" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		pipelineParms.cullMode = Pipeline::CULL_MODE_FRONT;
		//pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.blendMode = Pipeline::BLEND_MODE_ADDITIVE;
		pipelineParms.depthCompare = Pipeline::DEPTH_COMPARE_GREATER;
		pipelineParms.depthTest = true;
		//pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		pipelineParms.pushConstantSize = sizeof( float ) * 4;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_shadowDrawPipeline.Create( device, pipelineParms );
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
CleanupShadows
====================================================
*/
void CleanupShadows( DeviceContext * device ) {
	for ( int i = 0; i < MAX_SHADOWS; i++ ) {
		g_shadowFrameBuffers[ i ].Cleanup( device );

		g_shadowUpdatePipelines[ i ].Cleanup( device );
		g_shadowUpdateDescriptors[ i ].Cleanup( device );
	}

	g_shadowDrawPipeline.Cleanup( device );
	g_shadowDrawDescriptors.Cleanup( device );

	g_shadowCameraUniforms.Cleanup( device );

	g_shadowModel.Cleanup();
}

static bool g_shadowUpdated = false;

/*
====================================================
UpdateShadows
====================================================
*/
void UpdateShadows( DrawParms_t & parms ) {
	if ( g_shadowUpdated ) {
		return;
	}

	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.notCulledRenderModels;
	const int numModels = parms.numNotCulledRenderModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	std::vector< Descriptor > descriptorList;
	descriptorList.reserve( numModels );

	//
	//	Draw the models
	//
	for ( int s = 0; s < MAX_SHADOWS; s++ ) {
		descriptorList.clear();

		int camOffset = s * sizeof( shadowCamera_t );
		int camSize = sizeof( shadowCamera_t );

		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_shadowUpdatePipelines[ s ].GetFreeDescriptor();
			descriptor.BindBuffer( &g_shadowCameraUniforms, camOffset, camSize, 0 );					// bind the camera matrices
			descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
			
			descriptor.UpdateDescriptor( device );
			descriptorList.push_back( descriptor );
		}

		g_shadowFrameBuffers[ s ].m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
		g_shadowFrameBuffers[ s ].BeginRenderPass( device, cmdBufferIndex, true, true );

		g_shadowUpdatePipelines[ s ].BindPipeline( cmdBuffer );

		int idx = 0;
		for ( int i = 0; i < numModels; i++ ) {
			const RenderModel & renderModel = renderModels[ i ];
			if ( NULL == renderModel.modelDraw ) {
				continue;
			}

			descriptorList[ idx ].BindDescriptor( cmdBuffer, &g_shadowUpdatePipelines[ s ] );
			renderModel.modelDraw->DrawIndexed( cmdBuffer );
			idx++;
		}

		g_shadowFrameBuffers[ s ].EndRenderPass( device, cmdBufferIndex );
		g_shadowFrameBuffers[ s ].m_imageDepth.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL );
	}

	g_shadowUpdated = true;
}

static std::vector< Descriptor > g_descriptorList;

/*
====================================================
UpdateShadowDescriptors
====================================================
*/
void UpdateShadowDescriptors( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	float push[ 4 ];

	g_descriptorList.reserve( MAX_SHADOWS );
	g_descriptorList.clear();

	for ( int i = 0; i < MAX_SHADOWS; i++ ) {
		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_shadowDrawPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );								// bind the camera matrices

		int offset = i * sizeof( shadowCamera_t );
		descriptor.BindBuffer( &g_shadowCameraUniforms, offset, sizeof( shadowCamera_t ), 1 );	// bind the model matrices (the shadow camera will be used to transform the ndc model to world space)

		descriptor.BindImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerRepeat, 2 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerRepeat, 3 );
		descriptor.BindImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerRepeat, 4 );

		descriptor.BindImage( g_shadowFrameBuffers[ i ].m_imageDepth, Samplers::m_samplerStandard, 5 );
		
		descriptor.UpdateDescriptor( device );
		g_descriptorList.push_back( descriptor );
	}
}

/*
====================================================
DrawShadows
====================================================
*/
void DrawShadows( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	float push[ 4 ];

	//
	//	Draw the shadowed lights
	//
	int idx = 0;
	g_shadowDrawPipeline.BindPipeline( cmdBuffer );
	for ( int i = 0; i < MAX_SHADOWS; i++ ) {
		push[ 0 ] = g_shadowLights[ i ].dim;
		push[ 1 ] = parms.time;
		push[ 2 ] = g_shadowLights[ i ].near;
		push[ 3 ] = g_shadowLights[ i ].far;

		g_shadowDrawPipeline.BindPushConstant( cmdBuffer, 0, sizeof( float ) * 4, push );

		g_descriptorList[ idx ].BindDescriptor( cmdBuffer, &g_shadowDrawPipeline );
		g_shadowModel.DrawIndexed( cmdBuffer );
		idx++;
	}
}

