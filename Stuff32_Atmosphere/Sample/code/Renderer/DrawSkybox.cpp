//
//  DrawSkybox.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawSkybox.h"
#include "Models/ModelShape.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Graphics/Targa.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

Pipeline		g_skyboxPipeline;
Descriptors		g_skyboxDescriptors;
static Model			g_skyboxModel;			// origin centered cube for drawing

/*
====================================================
FillCube2
====================================================
*/
void FillCube2( Model * model, Vec3 offset, float size, Vec4 emissiveColor ) {
	extern void FillCubeTessellated( Model & model, int numDivisions );

	Model modelTmp;
	FillCubeTessellated( modelTmp, 1 );

	for ( int i = 0; i < modelTmp.m_vertices.size(); i++ ) {
		modelTmp.m_vertices[ i ].pos[ 0 ] *= size;
		modelTmp.m_vertices[ i ].pos[ 1 ] *= size;
		modelTmp.m_vertices[ i ].pos[ 2 ] *= size;

		modelTmp.m_vertices[ i ].pos[ 0 ] += offset.x;
		modelTmp.m_vertices[ i ].pos[ 1 ] += offset.y;
		modelTmp.m_vertices[ i ].pos[ 2 ] += offset.z;

		modelTmp.m_vertices[ i ].buff[ 0 ] = vert_t::FloatToByte_01( emissiveColor.x );
		modelTmp.m_vertices[ i ].buff[ 1 ] = vert_t::FloatToByte_01( emissiveColor.y );
		modelTmp.m_vertices[ i ].buff[ 2 ] = vert_t::FloatToByte_01( emissiveColor.z );
		modelTmp.m_vertices[ i ].buff[ 3 ] = vert_t::FloatToByte_01( emissiveColor.w );
	}

	int offsetIdx = model->m_vertices.size();
	for ( int i = 0; i < modelTmp.m_vertices.size(); i++ ) {
		model->m_vertices.push_back( modelTmp.m_vertices[ i ] );
	}

	for ( int i = 0; i < modelTmp.m_indices.size(); i++ ) {
		model->m_indices.push_back( modelTmp.m_indices[ i ] + offsetIdx );
	}
}

/*
====================================================
InitSkybox
====================================================
*/
bool InitSkybox( DeviceContext * device, int width, int height ) {
	bool result;

	FillCube2( &g_skyboxModel, Vec3( 0, 0, 0 ), 500, Vec4( 0, 0, 0, 0 ) );
	g_skyboxModel.MakeVBO( device );

	//
	//	Sky
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		result = g_skyboxDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_skyboxDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Skybox/SkySDF" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = false;
		result = g_skyboxPipeline.Create( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	return true;
}

/*
====================================================
CleanupSkybox
====================================================
*/
bool CleanupSkybox( DeviceContext * device ) {
	g_skyboxModel.Cleanup();

	g_skyboxPipeline.Cleanup( device );
	g_skyboxDescriptors.Cleanup( device );

	return true;
}

/*
====================================================
DrawSkybox
====================================================
*/
void DrawSkybox( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	const int timeOffset = camOffset + camSize;
	const int timeSize = sizeof( Vec4 );
	
	//
	//	Draw the sky
	//
	{
		g_skyboxPipeline.BindPipeline( cmdBuffer );
	
		Descriptor descriptor = g_skyboxPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );
		descriptor.BindBuffer( uniforms, timeOffset, timeSize, 1 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_skyboxPipeline );

		g_skyboxModel.DrawIndexed( cmdBuffer );
	}
}

