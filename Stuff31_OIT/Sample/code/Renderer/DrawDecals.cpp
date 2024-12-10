//
//  DrawDecals.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawDecals.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include "Math/Math.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

#pragma optimize( "", off )


Pipeline	g_decalPipeline;
Descriptors g_decalDescriptors;

Buffer g_decalUniforms;

Model g_decalModel;	// clip space model (use the inverse projection and inverse view matrices to transform it into world space)

struct decalMaterial_t {
	Image imageDiffuse;
	Image imageNormals;
	Image imageRoughness;
	Image imageSpecular;
	Image imageMask;
};

static decalMaterial_t g_materials[ 2 ];

struct decalCamera_t {
	Mat4 matView;
	Mat4 matProj;
};

struct decal_t {
	Vec3 pos;
	Vec3 look;
	Vec3 up;
	float halfWidth;
	float halfHeight;
	float halfDepth;
	int imageId;
};

#define NUM_DECALS 4

static decal_t g_decals[ NUM_DECALS ];

/*
====================================================
MakeDecalModel
====================================================
*/
void MakeDecalModel( DeviceContext * device ) {
	FillCube( g_decalModel );
	for ( int i = 0; i < g_decalModel.m_vertices.size(); i++ ) {		
		float z = g_decalModel.m_vertices[ i ].pos.z;
		g_decalModel.m_vertices[ i ].pos.z = Math::Clampf( z, 0.0f, 1.0f );
	}
	g_decalModel.MakeVBO( device );
}

/*
====================================================
InitDecals
====================================================
*/
bool InitDecals( DeviceContext * device ) {
	MakeDecalModel( device );

	bool result = false;

	g_decals[ 0 ].pos = Vec3( -12, -5.5f, 0.5f );
	g_decals[ 0 ].look = Vec3( 0, 0, -1 );
	g_decals[ 0 ].up = Vec3( 0, 1, 0 );
	g_decals[ 0 ].halfWidth = 4;
	g_decals[ 0 ].halfHeight = 2;
	g_decals[ 0 ].halfDepth = 0.2f;
	g_decals[ 0 ].imageId = 0;

	g_decals[ 1 ].pos = Vec3( -13, 1, 0.5f );
	g_decals[ 1 ].look = Vec3( 0, 0, -1 );
	g_decals[ 1 ].up = Vec3( 0, 1, 0 );
	g_decals[ 1 ].halfWidth = 4;
	g_decals[ 1 ].halfHeight = 4;
	g_decals[ 1 ].halfDepth = 0.2f;
	g_decals[ 1 ].imageId = 1;

	g_decals[ 2 ].pos = Vec3( -3, -6, 0.5f );
	g_decals[ 2 ].look = Vec3( 0, 0, -1 );
	g_decals[ 2 ].up = Vec3( 0, 1, 0 );
	g_decals[ 2 ].halfWidth = 2;
	g_decals[ 2 ].halfHeight = 2;
	g_decals[ 2 ].halfDepth = 0.2f;
	g_decals[ 2 ].imageId = 1;

	g_decals[ 3 ].pos = Vec3( 0, 0, 0.5f );
	g_decals[ 3 ].look = Vec3( 0, 0, -1 );
	g_decals[ 3 ].up = Vec3( 0, 1, 0 );
	g_decals[ 3 ].halfWidth = 2;
	g_decals[ 3 ].halfHeight = 2;
	g_decals[ 3 ].halfDepth = 0.2f;
	g_decals[ 3 ].imageId = 0;

// 	for ( int i = 0; i < 4; i++ ) {
// 		g_decals[ i ].halfWidth = 1;
// 		g_decals[ i ].halfHeight = 1;
// 	}

	Targa targaDiffuse[ 2 ];
	Targa targaNormals[ 2 ];
	Targa targaRoughness[ 2 ];
	Targa targaSpecular[ 2 ];
	Targa targaMask[ 2 ];

	targaDiffuse[ 0 ].Load( "../../common/data/images/decals/puddle_color.tga" );
	targaNormals[ 0 ].Load( "../../common/data/images/decals/puddle_normal.tga" );
	targaRoughness[ 0 ].Load( "../../common/data/images/decals/puddle_rough.tga" );
	targaSpecular[ 0 ].Load( "../../common/data/images/decals/puddle_metal.tga" );
	targaMask[ 0 ].Load( "../../common/data/images/decals/puddle_trans.tga" );
	
	targaDiffuse[ 1 ].Load( "../../common/data/images/decals/blood_color.tga" );
	targaNormals[ 1 ].Load( "../../common/data/images/decals/blood_normal.tga" );
	targaRoughness[ 1 ].Load( "../../common/data/images/decals/blood_rough.tga" );
	targaSpecular[ 1 ].Load( "../../common/data/images/decals/blood_metal.tga" );
	targaMask[ 1 ].Load( "../../common/data/images/decals/blood_trans.tga" );

	Image::CreateParms_t imageParms;
	imageParms.depth = 1;
	imageParms.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageParms.mipLevels = 1;
	imageParms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	for ( int i = 0; i < 2; i++ ) {
		imageParms.width = targaDiffuse[ i ].GetWidth();
		imageParms.height = targaDiffuse[ i ].GetHeight();
		g_materials[ i ].imageDiffuse.Create( device, imageParms );
		g_materials[ i ].imageDiffuse.UploadData( device, targaDiffuse[ i ].DataPtr() );

		imageParms.width = targaNormals[ i ].GetWidth();
		imageParms.height = targaNormals[ i ].GetHeight();
		g_materials[ i ].imageNormals.Create( device, imageParms );
		g_materials[ i ].imageNormals.UploadData( device, targaNormals[ i ].DataPtr() );

		imageParms.width = targaRoughness[ i ].GetWidth();
		imageParms.height = targaRoughness[ i ].GetHeight();
		g_materials[ i ].imageRoughness.Create( device, imageParms );
		g_materials[ i ].imageRoughness.UploadData( device, targaRoughness[ i ].DataPtr() );

		imageParms.width = targaSpecular[ i ].GetWidth();
		imageParms.height = targaSpecular[ i ].GetHeight();
		g_materials[ i ].imageSpecular.Create( device, imageParms );
		g_materials[ i ].imageSpecular.UploadData( device, targaSpecular[ i ].DataPtr() );

		imageParms.width = targaMask[ i ].GetWidth();
		imageParms.height = targaMask[ i ].GetHeight();
		g_materials[ i ].imageMask.Create( device, imageParms );
		g_materials[ i ].imageMask.UploadData( device, targaMask[ i ].DataPtr() );
	}

	decalCamera_t cameras[ NUM_DECALS ];
	for ( int i = 0; i < NUM_DECALS; i++ ) {
		Vec3 pos = g_decals[ i ].pos;
		Vec3 dir = g_decals[ i ].look;
		Vec3 lookat = pos + dir;
		Vec3 up = g_decals[ i ].up;
		Vec3 left = up.Cross( dir );
		up = dir.Cross( left );
		up.Normalize();

		float width = g_decals[ i ].halfWidth;
		float height = g_decals[ i ].halfHeight;
		float depth = g_decals[ i ].halfDepth;

		cameras[ i ].matView.LookAt( pos, lookat, up );
		cameras[ i ].matProj.OrthoVulkan( -width, width, -height, height, -depth, depth );

		cameras[ i ].matView = cameras[ i ].matView.Transpose();
		cameras[ i ].matProj = cameras[ i ].matProj.Transpose();
	}
	g_decalUniforms.Allocate( device, cameras, sizeof( decalCamera_t ) * NUM_DECALS, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	//
	//	decal draw pipeline
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		//descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 6;
		result = g_decalDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		pipelineParms.framebuffer = NULL;// &g_offscreenFrameBuffer;
		pipelineParms.renderPass = g_gbuffer.m_vkRenderPass;
		pipelineParms.numColorAttachments = GFrameBuffer::s_numColorImages;
		pipelineParms.descriptors = &g_decalDescriptors;		
		pipelineParms.shader = g_shaderManager->GetShader( "decals" );
		pipelineParms.width = g_gbuffer.m_parms.width;
		pipelineParms.height = g_gbuffer.m_parms.height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
		//pipelineParms.cullMode = Pipeline::CULL_MODE_FRONT;
		//pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		//pipelineParms.blendMode = Pipeline::BLEND_MODE_DECAL_MRT;
		//pipelineParms.depthCompare = Pipeline::DEPTH_COMPARE_GREATER;
		pipelineParms.depthCompare = Pipeline::DEPTH_COMPARE_LEQUAL;
		pipelineParms.depthTest = true;
		//pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
// 		pipelineParms.pushConstantSize = sizeof( float ) * 4;
// 		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		result = g_decalPipeline.Create( device, pipelineParms );
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
CleanupDecals
====================================================
*/
void CleanupDecals( DeviceContext * device ) {
	for ( int i = 0; i < 2; i++ ) {
		g_materials[ i ].imageDiffuse.Cleanup( device );
		g_materials[ i ].imageNormals.Cleanup( device );
		g_materials[ i ].imageRoughness.Cleanup( device );
		g_materials[ i ].imageSpecular.Cleanup( device );
		g_materials[ i ].imageMask.Cleanup( device );
	}

	g_decalPipeline.Cleanup( device );
	g_decalDescriptors.Cleanup( device );

	g_decalUniforms.Cleanup( device );

	g_decalModel.Cleanup();
}

static std::vector< Descriptor > g_descriptorList;

/*
====================================================
UpdateDecalDescriptors
====================================================
*/
void UpdateDecalDescriptors( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	g_descriptorList.reserve( NUM_DECALS );
	g_descriptorList.clear();

	for ( int i = 0; i < NUM_DECALS; i++ ) {
		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_decalPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );								// bind the camera matrices

		int offset = i * sizeof( decalCamera_t );
		descriptor.BindBuffer( &g_decalUniforms, offset, sizeof( decalCamera_t ), 1 );	// bind the model matrices (the shadow camera will be used to transform the ndc model to world space)

		int matId = g_decals[ i ].imageId;
		descriptor.BindImage( g_materials[ matId ].imageDiffuse, Samplers::m_samplerStandard, 2 );
		descriptor.BindImage( g_materials[ matId ].imageNormals, Samplers::m_samplerStandard, 3 );
		descriptor.BindImage( g_materials[ matId ].imageRoughness, Samplers::m_samplerStandard, 4 );
		descriptor.BindImage( g_materials[ matId ].imageSpecular, Samplers::m_samplerStandard, 5 );
		descriptor.BindImage( g_materials[ matId ].imageMask, Samplers::m_samplerStandard, 6 );

		descriptor.BindImage( g_preDepthFrameBuffer.m_imageDepth, Samplers::m_samplerStandard, 7 );
		
		descriptor.UpdateDescriptor( device );
		g_descriptorList.push_back( descriptor );
	}
}

/*
====================================================
DrawDecals
====================================================
*/
void DrawDecals( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 2;

	//
	//	Draw the shadowed lights
	//
	g_decalPipeline.BindPipeline( cmdBuffer );
	for ( int i = 0; i < NUM_DECALS; i++ ) {
		g_descriptorList[ i ].BindDescriptor( cmdBuffer, &g_decalPipeline );
		g_decalModel.DrawIndexed( cmdBuffer );
	}
}

