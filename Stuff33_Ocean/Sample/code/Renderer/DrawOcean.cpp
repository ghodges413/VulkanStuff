//
//  DrawOcean.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawOcean.h"
#include "Models/ModelShape.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Graphics/Targa.h"
#include "Miscellaneous/Fileio.h"
#include "Miscellaneous/Input.h"

#include <assert.h>
#include <stdio.h>
#include <vector>


#pragma optimize( "", off )


/*
========================================================================================================

Tessendorf Ocean Simulation

========================================================================================================
*/

Pipeline		g_oceanPipelineComputeUpdateSpectrum;
Descriptors		g_oceanDescriptorsUpdateSpectrum;

Pipeline		g_oceanPipelineComputeRows512;
Descriptors		g_oceanDescriptorsFFTRows512;

Pipeline		g_oceanPipelineComputeCols512;
Descriptors		g_oceanDescriptorsFFTCols512;

Pipeline		g_oceanPipelineComputeRows256;
Descriptors		g_oceanDescriptorsFFTRows256;

Pipeline		g_oceanPipelineComputeCols256;
Descriptors		g_oceanDescriptorsFFTCols256;

Pipeline		g_oceanPipelineComputeHeights;
Descriptors		g_oceanDescriptorsHeights;

Pipeline		g_oceanPipelineComputeNormals;
Descriptors		g_oceanDescriptorsNormals;

Pipeline		g_oceanPipelineDraw;
Descriptors		g_oceanDescriptorsDraw;

Image			g_oceanImageH0[ 4 ];
Image			g_oceanImageHT[ 4 ];
Image			g_oceanFFTA[ 4 ];
Image			g_oceanFFTB[ 4 ];
Image			g_oceanHeights[ 4 ];
Image			g_oceanNormals[ 4 ];


/*
========================================================================================================

Tessendorf Ocean Simulation

========================================================================================================
*/

/*
====================================================
OceanParms_t
====================================================
*/
struct OceanParms_t {
	int		m_dimSamples;
	float	m_dimPhysical;
	float	m_timeScale;
	float	m_amplitude;
	Vec2	m_windDir;
	float	m_windSpeed;
	float	m_windDependency;
	float	m_choppiness;
	float	m_suppressionFactor;
	float	m_gravity;

	OceanParms_t() {
		m_dimSamples = 512;
		m_dimPhysical = 1000;
		m_timeScale = 1.0f;
		m_amplitude = 1.0f;
		m_windDir = Vec2( 0.4f, 0.6f );
		m_windDir.Normalize();
		m_windSpeed = 600.0f;
		m_windDependency = 1.0f;
		m_choppiness = 1.0f;
		m_suppressionFactor = 0.001f;
		m_gravity = 981.0f;
	}

	OceanParms_t( int _dimSamples,
		float _dimPhysical,
		float _timeScale,
		float _amplitude,
		Vec2 _windDir,
		float _windSpeed,
		float _windDependency,
		float _choppiness,
		float _suppressionFactor,
		float _gravity ) {
		m_dimSamples = _dimSamples;
		m_dimPhysical = _dimPhysical;
		m_timeScale = _timeScale;
		m_amplitude = _amplitude;
		m_windDir = _windDir;
		m_windDir.Normalize();
		m_windSpeed = _windSpeed;
		m_windDependency = _windDependency;
		m_choppiness = _choppiness;
		m_suppressionFactor = _suppressionFactor;
		m_gravity = _gravity;
	}
};

//Vec2 g_windDir = Vec2( 1.0f, 0.0f );
Vec2 g_windDir = Vec2( 0.4f, 0.6f );

static const float g_choppinness = 0.25f;
static const int g_NumOceanParms = 4;
static const float g_oceanGravity = 981.0f;
const OceanParms_t g_OceanParms[ g_NumOceanParms ] = {
	OceanParms_t( 512, 1000.0f, 0.5f, 1.0f, g_windDir, 600.0f, 1.0f, g_choppinness, 0.001f, g_oceanGravity ),
	OceanParms_t( 256, 2000.0f, 1.0f, 1.0f, g_windDir, 600.0f, 1.0f, g_choppinness, 0.001f, g_oceanGravity ),
	OceanParms_t( 256, 3000.0f, 1.0f, 1.0f, g_windDir, 600.0f, 1.0f, 0.0f, 0.001f, g_oceanGravity ),
	OceanParms_t( 256, 7000.0f, 1.0f, 1.0f, g_windDir, 600.0f, 1.0f, 0.0f, 0.001f, g_oceanGravity ),
// 	OceanParms_t(  64, 4000.0f, 1.0f, 1.0f, Vec2( 0.4f, 0.6f ), 600.0f, 1.0f, 1.0f, 0.001f, 981.0f ),
// 	OceanParms_t(  64, 8000.0f, 1.0f, 1.0f, Vec2( 0.4f, 0.6f ), 600.0f, 1.0f, 1.0f, 0.001f, 981.0f ),
// 	OceanParms_t(  64, 20000.0f, 1.0f, 1.0f, Vec2( 0.4f, 0.6f ), 600.0f, 1.0f, 1.0f, 0.001f, 981.0f )
};

/*
====================================================
GetK
====================================================
*/
static Vec2 GetK( const int idx, const int width, const float length ) {
	const float pi = acosf( -1.0f );
	const float pi2 = pi * 2.0f;
	const float invLength = 1.0f / length;

	const int i = idx % width;
	const int j = idx / width;
	const float m = float( i ) - float( width ) * 0.5f;
	const float n = float( j ) - float( width ) * 0.5f;

	Vec2 K = Vec2( m, n ) * pi2 * invLength;
	return K;
}

/*
====================================================
GetX
====================================================
*/
static Vec2 GetX( const int idx, const int width, const float length ) {
	Vec2 X;

	const int i = idx % width;
	const int j = idx / width;
	const float m = float( i ) - float( width ) * 0.5f;
	const float n = float( j ) - float( width ) * 0.5f;

	X.x = m / float( ( width ) ) * length;
	X.y = n / float( ( width ) ) * length;
	return X;
}

/*
====================================================
PhillipsSpectrum
====================================================
*/
static float PhillipsSpectrum( const Vec2 & K, const OceanParms_t & parms ) {
	const float Ksqr = K.Dot( K );
	if ( 0.0f == Ksqr ) {
		return 0.0f;
	}

	const float L = parms.m_windSpeed * parms.m_windSpeed / parms.m_gravity;
	const float l = L * parms.m_suppressionFactor;
	float amplitude = parms.m_amplitude * expf( -Ksqr * l * l );

	Vec2 k = K;
	k.Normalize();
	const float kwind = k.Dot( parms.m_windDir );
	if ( kwind < 0.0f ) {
		amplitude *= parms.m_windDependency;
	}

	const float phillips = amplitude * expf( -1.0f / ( Ksqr * L * L ) ) * ( kwind * kwind ) / ( Ksqr * Ksqr );
	return phillips;
}

/*
====================================================
H0
====================================================
*/
static Complex H0( const Vec2 & K, const OceanParms_t & parms ) {
	const float xir = Random::Gaussian();
	const float xii = Random::Gaussian();

	const float invRoot2 = 1.0f / sqrtf( 2.0f );
	const float ph = PhillipsSpectrum( K, parms );
	const float rootPH = sqrtf( ph );

	Complex c;
	c.r = xir * invRoot2 * rootPH;
	c.i = xii * invRoot2 * rootPH;
	return c;
}

/*
====================================================
BuildSpectrum
Builds the k-space Phillip's spectrum
====================================================
*/
static void BuildSpectrum( Complex * gH0, const OceanParms_t & parms ) {
	const float pi = acosf( -1.0f );

	const float pi2 = pi * 2.0f;
	const float invLength = 1.0f / parms.m_dimPhysical;

	//
	//	Build the initial spectrum
	//
	for ( int i = 0; i < parms.m_dimSamples; ++i ) {
		for ( int j = 0; j < parms.m_dimSamples; ++j ) {
			const int idx = i + parms.m_dimSamples * j;

			float real = 0;
			float imaginary = 0;

			Vec2 K = GetK( idx, parms.m_dimSamples, parms.m_dimPhysical );
			Complex c = H0( K, parms );
			real = c.r;
			imaginary = c.i;
			if ( real != real ) {
				real = 0;
			}
			if ( imaginary != imaginary ) {
				imaginary = 0;
			}

			gH0[ idx ].r = real;
			gH0[ idx ].i = imaginary;
		}
	}
}










/*
========================================================================================================

Tessendorf Ocean Rendering

========================================================================================================
*/

struct oceanUniformParms_t {
	int patch_dim;
	float patch_length;
	float gravity;
	float time;
	float choppinness;
	float pad[ 3 ];
};
struct oceanDrawVertParms_t {
	Vec4 matBiasRow0;
	Vec4 matBiasRow1;
	Vec4 matBiasRow2;
	Vec4 matBiasRow3;

	Vec4 projectorPos;

	Vec3 viewLook;
	float pad0;
	Vec3 viewUp;
	float pad1;
	Vec3 viewLeft;
	float pad2;

	Vec4 matProjectorRow0;
	Vec4 matProjectorRow1;
	Vec4 matProjectorRow2;
	Vec4 matProjectorRow3;

	Vec4 matProjectorViewRow0;
	Vec4 matProjectorViewRow1;
	Vec4 matProjectorViewRow2;
	Vec4 matProjectorViewRow3;

	Vec4 dimSamples;
	Vec4 dimPhysical;
	
	Vec4 oceanPlane;
};
struct oceanDrawFragParms_t {
	Vec4 camPos;
	Vec4 dirToSun;
	Vec4 camPos2;
};
oceanDrawVertParms_t g_oceanParmsVerts;
oceanDrawFragParms_t g_oceanParmsFrag;

Buffer			g_oceanUniformBuffer;
Model			g_oceanGridModel;


Complex g_oceanData[ 512 * 512 * 3 ];

/*
====================================================
InitOceanSimulation
====================================================
*/
bool InitOceanSimulation( DeviceContext * device ) {
	bool result;

	// Build uniform Buffer for ocean
	{
		int byteSize = device->GetAlignedUniformByteOffset( sizeof( oceanUniformParms_t ) ) * g_NumOceanParms;
		byteSize += device->GetAlignedUniformByteOffset( sizeof( oceanDrawVertParms_t ) );
		byteSize += device->GetAlignedUniformByteOffset( sizeof( oceanDrawFragParms_t ) );
		g_oceanUniformBuffer.Allocate( device, NULL, byteSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	}
	
	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		BuildSpectrum( g_oceanData, g_OceanParms[ i ] );

		Image::CreateParms_t parms;
		parms.width = g_OceanParms[ i ].m_dimSamples;
		parms.height = g_OceanParms[ i ].m_dimSamples;
		parms.depth = 1;
		parms.mipLevels = 1;
		parms.format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
		parms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		parms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		//VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		g_oceanImageH0[ i ].Create( device, parms );
		g_oceanImageH0[ i ].UploadData( device, g_oceanData );

		parms.height = g_OceanParms[ i ].m_dimSamples * 3;
		parms.format = VkFormat::VK_FORMAT_R16G16_SFLOAT;
		g_oceanImageHT[ i ].Create( device, parms );
		g_oceanFFTA[ i ].Create( device, parms );
		g_oceanFFTB[ i ].Create( device, parms );

		parms.height = g_OceanParms[ i ].m_dimSamples;
		parms.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		parms.mipLevels = Image::CalculateMipLevels( parms.width, parms.width );
		parms.usageFlags = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		g_oceanHeights[ i ].Create( device, parms );
		g_oceanNormals[ i ].Create( device, parms );
	}

	//
	//	Transition image layouts
	//
	{
//		VkCommandBuffer cmdBuffer = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );
		for ( int i = 0; i < g_NumOceanParms; i++ ) {
//			g_oceanImageH0[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
// 			g_oceanImageHT[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
// 			g_oceanFFTA[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
// 			g_oceanFFTB[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
// 			g_oceanHeights[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
// 			g_oceanNormals[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );

			g_oceanImageH0[ i ].TransitionLayout( device ); // transition to general layout, required by spec for load/store operations on storage images
			g_oceanImageHT[ i ].TransitionLayout( device );
			g_oceanFFTA[ i ].TransitionLayout( device );
			g_oceanFFTB[ i ].TransitionLayout( device );
			g_oceanHeights[ i ].TransitionLayout( device );
			g_oceanNormals[ i ].TransitionLayout( device );
		}
//		device->FlushCommandBuffer( cmdBuffer, device->m_vkGraphicsQueue );
	}

	//
	//	Spectrum Update Compute
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 2;
		descriptorParms.numUniformsCompute = 1;
		result = g_oceanDescriptorsUpdateSpectrum.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_oceanDescriptorsUpdateSpectrum;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/SpectrumUpdate" );
		result = g_oceanPipelineComputeUpdateSpectrum.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	FFT Rows 512
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 2;
		result = g_oceanDescriptorsFFTRows512.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_oceanDescriptorsFFTRows512;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/FFTCooleyTukey512_rows" );
		result = g_oceanPipelineComputeRows512.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	FFT Columns 512
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 2;
		result = g_oceanDescriptorsFFTCols512.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_oceanDescriptorsFFTCols512;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/FFTCooleyTukey512_cols" );
		result = g_oceanPipelineComputeCols512.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	FFT Rows 256
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 2;
		result = g_oceanDescriptorsFFTRows256.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_oceanDescriptorsFFTRows256;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/FFTCooleyTukey256_rows" );
		result = g_oceanPipelineComputeRows256.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	FFT Columns 256
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 2;
		result = g_oceanDescriptorsFFTCols256.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_oceanDescriptorsFFTCols256;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/FFTCooleyTukey256_cols" );
		result = g_oceanPipelineComputeCols256.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Heights
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 2;
		descriptorParms.numUniformsCompute = 1;
		result = g_oceanDescriptorsHeights.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_oceanDescriptorsHeights;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/CopyHeights" );
		result = g_oceanPipelineComputeHeights.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Normals
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 2;
		descriptorParms.numUniformsCompute = 1;
		result = g_oceanDescriptorsNormals.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_oceanDescriptorsNormals;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/BuildNormals" );
		result = g_oceanPipelineComputeNormals.CreateCompute( device, pipelineParms );
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Ocean Draw
	//
	{
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_GRAPHICS;
		descriptorParms.numUniformsVertex = 2;
		descriptorParms.numStorageImagesVertex = 4;
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numSamplerImagesFragment = 4;
		result = g_oceanDescriptorsDraw.Create( device, descriptorParms );
		if ( !result ) {
			printf( "Unable to build compute descriptors!\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.framebuffer = &g_offscreenFrameBuffer;
		pipelineParms.descriptors = &g_oceanDescriptorsDraw;
		pipelineParms.shader = g_shaderManager->GetShader( "Ocean/DrawProjected" );
		pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
		pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
		//pipelineParms.cullMode = Pipeline::CULL_MODE_FRONT;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
//		pipelineParms.fillMode = Pipeline::FILL_MODE_FILL;
		pipelineParms.depthTest = true;
		pipelineParms.depthWrite = true;
		result = g_oceanPipelineDraw.Create( device, pipelineParms ); 
		if ( !result ) {
			printf( "Unable to build compute pipeline!\n" );
			assert( 0 );
			return false;
		}
	}

	//
	//	Build the ocean screen space grid
	//
	{
		const unsigned int dimSize = 1024;
		std::vector< vert_t > verts;
		std::vector< vert_t > vertsPlane;
		std::vector< unsigned int > indices;
		verts.reserve( dimSize * dimSize );
		verts.clear();
		for ( unsigned int i = 0; i < dimSize * dimSize; i++ ) {
			vert_t vert;
		
			unsigned int x;
			unsigned int y;
			Morton::MortonOrder2D( i, x, y );

			Vec2 st = Vec2( (float)x, (float)y );
			st /= float( dimSize - 1 );

			vert.st[ 0 ] = st[ 0 ];
			vert.st[ 1 ] = st[ 1 ];

			st *= 2.0f;
			st += Vec2( -1, -1 );

			Vec3 pos = Vec3( st.x, st.y, -1.0f );
			vert.pos[ 0 ] = pos[ 0 ];
			vert.pos[ 1 ] = pos[ 1 ];
			vert.pos[ 2 ] = pos[ 2 ];

			vert.pos[ 0 ] = vert.pos[ 0 ] * 2.0f + 1.0f;
			vert.pos[ 1 ] = vert.pos[ 1 ] * 2.0f + 1.0f;
			vert.pos[ 2 ] = 0.0f;

			vert_t::Vec3ToByte4( Vec3( 0, 0, 1 ), vert.norm );
			vert_t::Vec3ToByte4( Vec3( 1, 0, 0 ), vert.tang );

			verts.push_back( vert );
		}

 		indices.reserve( dimSize * dimSize * 3 * 2 );
		for ( unsigned int x = 0; x < dimSize - 1; x++ ) {
			for ( unsigned int y = 0; y < dimSize - 1; y++ ) {
				unsigned int idx0 = Morton::MortonOrder2D( x + 0, y + 0 );
				unsigned int idx1 = Morton::MortonOrder2D( x + 1, y + 0 );
				unsigned int idx2 = Morton::MortonOrder2D( x + 1, y + 1 );

				unsigned int idx4 = Morton::MortonOrder2D( x + 0, y + 0 );
				unsigned int idx5 = Morton::MortonOrder2D( x + 1, y + 1 );
				unsigned int idx6 = Morton::MortonOrder2D( x + 0, y + 1 );

				indices.push_back( idx0 );
				indices.push_back( idx1 );
				indices.push_back( idx2 );

				indices.push_back( idx4 );
				indices.push_back( idx5 );
				indices.push_back( idx6 );
			}
		}

		verts.clear();
		indices.clear();
		for ( unsigned int x = 0; x < dimSize; x++ ) {
			for ( unsigned int y = 0; y < dimSize; y++ ) {
				Vec2 st[ 4 ];
				st[ 0 ] = Vec2( x, y );
				st[ 1 ] = Vec2( x + 1, y );
				st[ 2 ] = Vec2( x, y + 1 );
				st[ 3 ] = Vec2( x + 1, y + 1 );

				for ( int i = 0; i < 4; i++ ) {
					vert_t vert;

					Vec3 pos = Vec3( st[ i ].x, st[ i ].y, 0.0f ) * ( 1.0f / float( dimSize ) );
					vert.pos[ 0 ] = pos[ 0 ];
					vert.pos[ 1 ] = pos[ 1 ];
					vert.pos[ 2 ] = pos[ 2 ];

					vert.pos[ 0 ] = vert.pos[ 0 ] * 2.0f - 1.0f;
					vert.pos[ 1 ] = vert.pos[ 1 ] * 2.0f - 1.0f;
					vert.pos[ 2 ] = 0.0f;

					verts.push_back( vert );
				}

				int offset = verts.size() - 4;
				indices.push_back( offset + 0 );
				indices.push_back( offset + 1 );
				indices.push_back( offset + 3 );

				indices.push_back( offset + 0 );
				indices.push_back( offset + 2 );
				indices.push_back( offset + 3 );
			}
		}

		//g_oceanGridModel.LoadFromData( verts.ToPtr(), verts.Num(), indices.ToPtr(), indices.Num() );
		g_oceanGridModel.m_vertices = verts;
		g_oceanGridModel.m_indices = indices;
		g_oceanGridModel.MakeVBO( device );
	}

	return true;
}

/*
====================================================
CleanupOceanSimulation
====================================================
*/
bool CleanupOceanSimulation( DeviceContext * device ) {
	g_oceanGridModel.Cleanup();
	g_oceanUniformBuffer.Cleanup( device );

	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		g_oceanImageH0[ i ].Cleanup( device );
		g_oceanImageHT[ i ].Cleanup( device );
		g_oceanFFTA[ i ].Cleanup( device );
		g_oceanFFTB[ i ].Cleanup( device );
		g_oceanHeights[ i ].Cleanup( device );
		g_oceanNormals[ i ].Cleanup( device );
	}

	g_oceanPipelineComputeUpdateSpectrum.Cleanup( device );
	g_oceanDescriptorsUpdateSpectrum.Cleanup( device );

	g_oceanPipelineComputeRows512.Cleanup( device );
	g_oceanDescriptorsFFTRows512.Cleanup( device );

	g_oceanPipelineComputeCols512.Cleanup( device );
	g_oceanDescriptorsFFTCols512.Cleanup( device );

	g_oceanPipelineComputeRows256.Cleanup( device );
	g_oceanDescriptorsFFTRows256.Cleanup( device );

	g_oceanPipelineComputeCols256.Cleanup( device );
	g_oceanDescriptorsFFTCols256.Cleanup( device );

	g_oceanPipelineComputeHeights.Cleanup( device );
	g_oceanDescriptorsHeights.Cleanup( device );

	g_oceanPipelineComputeNormals.Cleanup( device );
	g_oceanDescriptorsNormals.Cleanup( device );

	g_oceanPipelineDraw.Cleanup( device );
	g_oceanDescriptorsDraw.Cleanup( device );

	return true;
}

/*
====================================================
IntersectSegmentPlaneZ
====================================================
*/
bool IntersectSegmentPlaneZ( const Vec3 & ptA, const Vec3 & ptB, const float planeZ, Vec3 & ptOut ) {
	if ( ptA.z >= planeZ && ptB.z >= planeZ ) {
		return false;
	}
	if ( ptA.z <= planeZ && ptB.z <= planeZ ) {
		return false;
	}

	Vec3 dir = ptB - ptA;
	dir.Normalize();

	// z = ptA.z + t * dir.z = planeZ
	float t = ( planeZ - ptA.z ) / dir.z;
	ptOut = ptA + dir * t;

	return true;
}

/*
====================================================
IntersectOceanPlaneViewFrustum
====================================================
*/
const float g_oceanPlaneDisplacement = 10.0f;	// The delta height above and below the plane
bool IntersectOceanPlaneViewFrustum( const Mat4 & matView, const Mat4 & matProj, const float oceanPlaneZ, Mat4 & matProjector, Mat4 & matBias, Vec3 & projectorPos, Vec2 & yScaleOffset ) {
	Vec4 pts[ 8 ] = {
		Vec4(-1,-1, 1, 1 ),
		Vec4( 1,-1, 1, 1 ),
		Vec4( 1, 1, 1, 1 ),
		Vec4(-1, 1, 1, 1 ),

		Vec4(-1,-1,-1, 1 ),
		Vec4( 1,-1,-1, 1 ),
		Vec4( 1, 1,-1, 1 ),
		Vec4(-1, 1,-1, 1 ),
	};

	struct edge_t {
		int a;
		int b;
		edge_t( int _a, int _b ) : a( _a ), b( _b ) {}
	};

	const edge_t edges[ 12 ] = {
		edge_t( 0, 1 ),
		edge_t( 1, 2 ),
		edge_t( 2, 3 ),
		edge_t( 3, 0 ),

		edge_t( 4, 5 ),
		edge_t( 5, 6 ),
		edge_t( 6, 7 ),
		edge_t( 7, 4 ),

		edge_t( 0, 4 ),
		edge_t( 1, 5 ),
		edge_t( 2, 6 ),
		edge_t( 3, 7 ),
	};

	// Get the inverse of the view projection matrix
	Mat4 matViewProj = matProj * matView;
	Mat4 matInverse = matViewProj.Inverse();

	// Project the viewport points to world space
	for ( int i = 0; i < 8; i++ ) {
		Vec4 corner = matInverse * pts[ i ];
		corner /= corner.w;
		pts[ i ] = corner;
		pts[ i ].w = 1.0f;
	}

	// Check for intersections with the ocean
	Vec3 oceanPts[ 24 * 3 ];
	int numIntersections = 0;
	for ( int z = -1; z <= 1; z++ ) {
		const float dz = g_oceanPlaneDisplacement;
		const float planeZ = float( z ) * dz + oceanPlaneZ;

		for ( int i = 0; i < 12; i++ ) {
			const int a = edges[ i ].a;
			const int b = edges[ i ].b;
			const Vec3 ptA = Vec3( pts[ a ].x, pts[ a ].y, pts[ a ].z );
			const Vec3 ptB= Vec3( pts[ b ].x, pts[ b ].y, pts[ b ].z );

			const bool doesIntersect = IntersectSegmentPlaneZ( ptA, ptB, planeZ, oceanPts[ numIntersections ] );
			if ( doesIntersect ) {
				++numIntersections;
			}
		}
	}

	if ( numIntersections < 3 ) {
		return false;
	}

	const Vec3 upDir = Vec3( 0, 0, 1 );
	// Get the Bounds of the intersection point with the ocean plane
	Bounds oceanBounds;
	for ( int i = 0; i < numIntersections; i++ ) {
		oceanBounds.Expand( oceanPts[ i ] );
	}
// 	if ( projectorPos.z < oceanPlaneZ + 100.0f ) {
// 		projectorPos.z = oceanPlaneZ + 100.0f;
// 	}

	
	// Put the ocean pts back into screen space
	Vec4 screenSpacePts[ 24 * 3 ];
	Mat4 mvp = matProj * matView;
	for ( int i = 0; i < numIntersections; i++ ) {
		screenSpacePts[ i ] = Vec4( oceanPts[ i ].x, oceanPts[ i ].y, oceanPts[ i ].z, 1.0f );
		
		Vec4 corner = matViewProj * screenSpacePts[ i ];
		corner /= corner.w;
		screenSpacePts[ i ] = corner;
	}

	// Make the bounds
	Bounds bounds;
	for ( int i = 0; i < numIntersections; i++ ) {
		Vec3 pt2d = Vec3( screenSpacePts[ i ].x, screenSpacePts[ i ].y, 0.0f );
		bounds.Expand( pt2d );
	}
// 	if ( bounds.mins.x <= -0.99f ) {
// 		bounds.mins.x = -1.2f;
// 		bounds.mins.x = -1.0f;
// 	}
// 	if ( bounds.mins.y <= -0.99f ) {
// 		bounds.mins.y = -1.2f;
// 		bounds.mins.y = -1.0f;
// 	}
// 	if ( bounds.maxs.x >= 0.99f ) {
// 		bounds.maxs.x = 1.2f;
// 		bounds.maxs.x = 1.0f;
// 	}

	// Calculate the bias matrix: (we might need to transpose it)
	matBias.Identity();
	matBias.rows[ 0 ][ 0 ] = ( bounds.maxs.x - bounds.mins.x ) * 0.5f;
	matBias.rows[ 1 ][ 1 ] = ( bounds.maxs.y - bounds.mins.y ) * 0.5f;
 	matBias.rows[ 0 ][ 3 ] = ( bounds.maxs.x + bounds.mins.x ) * 0.5f;
 	matBias.rows[ 1 ][ 3 ] = ( bounds.maxs.y + bounds.mins.y ) * 0.5f;

	yScaleOffset[ 0 ] = ( bounds.maxs.y - bounds.mins.y ) * 0.5f;
	yScaleOffset[ 1 ] = ( bounds.maxs.y + bounds.mins.y ) * 0.5f;

// 	Mat4 invView = matView.Inverse().Transpose();
// 	Mat4 invProj = matProj.Inverse().Transpose();
// 	Mat4 invViewProj = invProj * invView;
	matViewProj = matProj * matView;
	matProjector = matViewProj;//.Transpose();
 	matProjector = matProjector.Inverse();
// 	matProjector = invViewProj;
	return true;
}

/*
====================================================
UpdateOceanParms
====================================================
*/
bool g_oceanDoesIntersectCamera = false;
float g_oceanTimeAccumulator = 0.0f;
void UpdateOceanParms( DeviceContext * device, float dt_sec, Mat4 matView, Mat4 matProj, Vec3 camPos, Vec3 camLookAt ) {
	g_oceanParmsFrag.camPos = Vec4( camPos.x, camPos.y, camPos.z, 0.0f );
	g_oceanParmsFrag.dirToSun = Vec4( 1, 1, 1, 0 );
	g_oceanParmsFrag.dirToSun.Normalize();
	g_oceanParmsFrag.camPos2 = Vec4( camPos.x, camPos.y, camPos.z, 0.0f );

//	g_oceanParmsVerts.oceanPlane = -100.0f;
	g_oceanParmsVerts.projectorPos = Vec4( camPos.x, camPos.y, camPos.z, 0.0f );
	g_oceanParmsVerts.dimSamples = Vec4( g_OceanParms[ 0 ].m_dimSamples, g_OceanParms[ 1 ].m_dimSamples, g_OceanParms[ 2 ].m_dimSamples, g_OceanParms[ 3 ].m_dimSamples );
	g_oceanParmsVerts.dimPhysical = Vec4( g_OceanParms[ 0 ].m_dimPhysical, g_OceanParms[ 1 ].m_dimPhysical, g_OceanParms[ 2 ].m_dimPhysical, g_OceanParms[ 3 ].m_dimPhysical );
// 	g_oceanParmsVerts.dimensions[ 0 ] = Vec2( (float)g_OceanParms[ 0 ].m_dimSamples, g_OceanParms[ 0 ].m_dimPhysical );
// 	g_oceanParmsVerts.dimensions[ 1 ] = Vec2( (float)g_OceanParms[ 1 ].m_dimSamples, g_OceanParms[ 1 ].m_dimPhysical );
// 	g_oceanParmsVerts.dimensions[ 2 ] = Vec2( (float)g_OceanParms[ 2 ].m_dimSamples, g_OceanParms[ 2 ].m_dimPhysical );
// 	g_oceanParmsVerts.dimensions[ 3 ] = Vec2( (float)g_OceanParms[ 3 ].m_dimSamples, g_OceanParms[ 3 ].m_dimPhysical );

	Vec3 tmp = camPos;
	Mat4 matBias;
	Vec2 yScaleOffset;
	Mat4 tmpProjector;
	//float oceanPlane = -75.0f;
	float oceanPlane = -10;
	g_oceanDoesIntersectCamera = IntersectOceanPlaneViewFrustum( matView, matProj, oceanPlane, tmpProjector, matBias, tmp, yScaleOffset );
	if ( !g_oceanDoesIntersectCamera ) {
		return;
	}

	g_oceanParmsVerts.oceanPlane = Vec4( oceanPlane );

	Vec3 dir = camLookAt - camPos;
	dir.Normalize();
	g_oceanParmsVerts.viewLook = dir;
	g_oceanParmsVerts.viewLeft = Vec3( 0, 0, 1 ).Cross( dir );
	g_oceanParmsVerts.viewLeft.Normalize();
	g_oceanParmsVerts.viewUp = dir.Cross( g_oceanParmsVerts.viewLeft );
	g_oceanParmsVerts.viewUp.Normalize();

// 	g_oceanParmsVerts.viewLeft = Vec3( 0, 1, 0 );
// 	g_oceanParmsVerts.viewUp = Vec3( 0, 0, 1 );

	Mat4 tmpBias = matBias;
	g_oceanParmsVerts.matBiasRow0 = tmpBias.rows[ 0 ];
	g_oceanParmsVerts.matBiasRow1 = tmpBias.rows[ 1 ];
	g_oceanParmsVerts.matBiasRow2 = tmpBias.rows[ 2 ];
	g_oceanParmsVerts.matBiasRow3 = tmpBias.rows[ 3 ];

	g_oceanParmsVerts.matProjectorRow0 = tmpProjector.rows[ 0 ];
	g_oceanParmsVerts.matProjectorRow1 = tmpProjector.rows[ 1 ];
	g_oceanParmsVerts.matProjectorRow2 = tmpProjector.rows[ 2 ];
	g_oceanParmsVerts.matProjectorRow3 = tmpProjector.rows[ 3 ];

	g_oceanParmsVerts.matProjectorViewRow0 = matView.rows[ 0 ];
	g_oceanParmsVerts.matProjectorViewRow0 = matView.rows[ 1 ];
	g_oceanParmsVerts.matProjectorViewRow0 = matView.rows[ 2 ];
	g_oceanParmsVerts.matProjectorViewRow0 = matView.rows[ 3 ];

	Vec3 projectorPos = Vec3( 0, 0, 20 );
	g_oceanParmsVerts.projectorPos = Vec4( 0, 0, 20, 0 );
	g_oceanParmsVerts.projectorPos.x = camPos.x;
	g_oceanParmsVerts.projectorPos.y = camPos.y;
	g_oceanParmsVerts.projectorPos.z = camPos.z;

	Mat4 viewView;
	viewView.LookAt( projectorPos, projectorPos + Vec3( 1, 0, 0 ), Vec3( 0, 0, 1 ) );
	
	//
	// Copy the data into the uniform buffer
	//
	unsigned char * ptr = (unsigned char *)g_oceanUniformBuffer.MapBuffer( device );
	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		oceanUniformParms_t parms;

		parms.patch_dim		= g_OceanParms[ i ].m_dimSamples;
		parms.patch_length	= g_OceanParms[ i ].m_dimPhysical;
		parms.gravity		= g_OceanParms[ i ].m_gravity;
		parms.choppinness	= g_OceanParms[ i ].m_choppiness;

		g_oceanTimeAccumulator += dt_sec * 0.1f;

		parms.time = g_oceanTimeAccumulator * g_OceanParms[ i ].m_timeScale;

		memcpy( ptr, &parms, sizeof( oceanUniformParms_t ) );
		ptr += device->GetAlignedUniformByteOffset( sizeof( oceanUniformParms_t ) );
	}
	memcpy( ptr, &g_oceanParmsVerts, sizeof( oceanDrawVertParms_t ) );
	ptr += device->GetAlignedUniformByteOffset( sizeof( oceanDrawVertParms_t ) );
	memcpy( ptr, &g_oceanParmsFrag, sizeof( oceanDrawFragParms_t ) );
	ptr += device->GetAlignedUniformByteOffset( sizeof( oceanDrawFragParms_t ) );
	g_oceanUniformBuffer.UnmapBuffer( device );
}

/*
====================================================
UpdateOceanSimulation
====================================================
*/
void UpdateOceanSimulation( DrawParms_t & parms ) {
	if ( !g_oceanDoesIntersectCamera ) {
		return;
	}

	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const bool useBarriers = true;

	//
	//	Update the spectrum from H0 to H( t )
	//
	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
		g_oceanPipelineComputeUpdateSpectrum.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_oceanPipelineComputeUpdateSpectrum.GetFreeDescriptor();
		int uniformOffset = device->GetAlignedUniformByteOffset( sizeof( oceanUniformParms_t ) ) * i;
		int uniformSize = sizeof( oceanUniformParms_t );
		descriptor.BindBuffer( &g_oceanUniformBuffer, uniformOffset, uniformSize, 0 );
		descriptor.BindStorageImage(  g_oceanImageH0[ i ], Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage(  g_oceanImageHT[ i ], Samplers::m_samplerStandard, 2 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineComputeUpdateSpectrum );
		g_oceanPipelineComputeUpdateSpectrum.DispatchCompute( cmdBuffer, g_OceanParms[ i ].m_dimSamples / 32, g_OceanParms[ i ].m_dimSamples / 32, 1 );
	}

	//
	//	Image barrier
	//
	if ( useBarriers ) {
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_oceanImageHT[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );

		MemoryBarriers::CreateMemoryBarrier( cmdBuffer );
	}

	//
	//	FFT rows
	//
	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		if ( 512 == g_OceanParms[ i ].m_dimSamples ) {
			int numSets = 3;
			int workGroupSize = 64;
			int numThreads = 512 * 512 / 32;	// 32 is N1 from the shader

			// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
			g_oceanPipelineComputeRows512.BindPipelineCompute( cmdBuffer );

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_oceanPipelineComputeRows512.GetFreeDescriptor();
			descriptor.BindStorageImage(  g_oceanImageHT[ i ], Samplers::m_samplerStandard, 0 );
			descriptor.BindStorageImage(  g_oceanFFTA[ i ], Samplers::m_samplerStandard, 1 );
			descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineComputeRows512 );
			g_oceanPipelineComputeRows512.DispatchCompute( cmdBuffer, numThreads * numSets / workGroupSize, 1, 1 );
		} else if ( 256 == g_OceanParms[ i ].m_dimSamples ) {
			int numSets = 3;
			int workGroupSize = 64;
			int numThreads = 256 * 256 / 16;	// 16 is N1 from the shader

			// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
			g_oceanPipelineComputeRows256.BindPipelineCompute( cmdBuffer );

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_oceanPipelineComputeRows256.GetFreeDescriptor();
			descriptor.BindStorageImage( g_oceanImageHT[ i ], Samplers::m_samplerStandard, 0 );
			descriptor.BindStorageImage( g_oceanFFTA[ i ], Samplers::m_samplerStandard, 1 );
			descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineComputeRows256 );
			g_oceanPipelineComputeRows256.DispatchCompute( cmdBuffer, numThreads * numSets / workGroupSize, 1, 1 );
		}
	}

	//
	//	Image barrier
	//
	if ( useBarriers ) {
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_oceanFFTA[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );

		MemoryBarriers::CreateMemoryBarrier( cmdBuffer );
	}

	//
	//	FFT columns
	//
	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		if ( 512 == g_OceanParms[ i ].m_dimSamples ) {
			int numSets = 3;
			int workGroupSize = 64;
			int numThreads = 512 * 512 / 32;	// 32 is N1 from the shader

			// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
			g_oceanPipelineComputeCols512.BindPipelineCompute( cmdBuffer );

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_oceanPipelineComputeCols512.GetFreeDescriptor();
			descriptor.BindStorageImage( g_oceanFFTA[ i ], Samplers::m_samplerStandard, 0 );
			descriptor.BindStorageImage( g_oceanFFTB[ i ], Samplers::m_samplerStandard, 1 );
			descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineComputeCols512 );
			g_oceanPipelineComputeCols512.DispatchCompute( cmdBuffer, numThreads * numSets / workGroupSize, 1, 1 );
		} else if ( 256 == g_OceanParms[ i ].m_dimSamples ) {
			int numSets = 3;
			int workGroupSize = 64;
			int numThreads = 256 * 256 / 16;	// 16 is N1 from the shader

			// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
			g_oceanPipelineComputeCols256.BindPipelineCompute( cmdBuffer );

			// Descriptor is how we bind our buffers and images
			Descriptor descriptor = g_oceanPipelineComputeCols256.GetFreeDescriptor();
			descriptor.BindStorageImage( g_oceanFFTA[ i ], Samplers::m_samplerStandard, 0 );
			descriptor.BindStorageImage( g_oceanFFTB[ i ], Samplers::m_samplerStandard, 1 );
			descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineComputeCols256 );
			g_oceanPipelineComputeCols256.DispatchCompute( cmdBuffer, numThreads * numSets / workGroupSize, 1, 1 );
		}
	}

	//
	//	Image barrier
	//
	if ( useBarriers ) {
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_oceanFFTB[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );

		MemoryBarriers::CreateMemoryBarrier( cmdBuffer );
	}

	//
	//	Copy Heights
	//
	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
		g_oceanPipelineComputeHeights.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_oceanPipelineComputeHeights.GetFreeDescriptor();
		int uniformOffset = device->GetAlignedUniformByteOffset( sizeof( oceanUniformParms_t ) ) * i;
		int uniformSize = sizeof( oceanUniformParms_t );
		descriptor.BindBuffer( &g_oceanUniformBuffer, uniformOffset, uniformSize, 0 );
		descriptor.BindStorageImage( g_oceanFFTB[ i ], Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( g_oceanHeights[ i ], Samplers::m_samplerStandard, 2 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineComputeHeights );
		const int num = g_OceanParms[ i ].m_dimSamples;
		g_oceanPipelineComputeHeights.DispatchCompute( cmdBuffer, num / 32, num / 32, 1 );
	}

	//
	//	Image barrier
	//
	if ( useBarriers ) {
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_oceanHeights[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );

		MemoryBarriers::CreateMemoryBarrier( cmdBuffer );
	}

	//
	//	Generate Normals
	//
	for ( int i = 0; i < g_NumOceanParms; i++ ) {
		// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
		g_oceanPipelineComputeNormals.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_oceanPipelineComputeNormals.GetFreeDescriptor();
		int uniformOffset = device->GetAlignedUniformByteOffset( sizeof( oceanUniformParms_t ) ) * i;
		int uniformSize = sizeof( oceanUniformParms_t );
		descriptor.BindBuffer( &g_oceanUniformBuffer, uniformOffset, uniformSize, 0 );
		descriptor.BindStorageImage( g_oceanHeights[ i ], Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( g_oceanNormals[ i ], Samplers::m_samplerStandard, 2 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineComputeNormals );
		const int num = g_OceanParms[ i ].m_dimSamples;
		g_oceanPipelineComputeNormals.DispatchCompute( cmdBuffer, num / 32, num / 32, 1 );
	}

	//
	//	Image barrier
	//
	if ( useBarriers ) {
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_oceanNormals[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );

		MemoryBarriers::CreateMemoryBarrier( cmdBuffer );
	}

	//
	//	Generate Mipmaps
	//
//	for ( int i = 0; i < g_NumOceanParms; i++ ) {
// 		g_oceanHeights[ i ].GenerateMipMaps( device, cmdBuffer );
// 		g_oceanNormals[ i ].GenerateMipMaps( device, cmdBuffer );
//	}
}

/*
====================================================
DrawOcean
====================================================
*/
void DrawOcean( DrawParms_t & parms ) {
	if ( !g_oceanDoesIntersectCamera ) {
		return;
	}

	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	//
	//	Now Draw the Ocean!
	//
	{
		const int camOffset = 0;
		const int camSize = sizeof( float ) * 16 * 4;

// 		const int shadowCamOffset = camOffset + camSize;
// 		const int shadowCamSize = camSize;

		const int uboVertOffset = device->GetAlignedUniformByteOffset( sizeof( oceanUniformParms_t ) ) * 4;
		const int uboVertSize = sizeof( oceanDrawVertParms_t );

		const int uboFragOffset = device->GetAlignedUniformByteOffset( uboVertOffset + uboVertSize );
		const int uboFragSize = sizeof( oceanDrawFragParms_t );

		// Binding the pipeline is effectively the "use shader" we had back in our opengl apps
		g_oceanPipelineDraw.BindPipeline( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_oceanPipelineDraw.GetFreeDescriptor();
 		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );
 		descriptor.BindBuffer( &g_oceanUniformBuffer, uboVertOffset, uboVertSize, 1 );

		descriptor.BindStorageImage( g_oceanHeights[ 0 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_oceanHeights[ 1 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_oceanHeights[ 2 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindStorageImage( g_oceanHeights[ 3 ], Samplers::m_samplerStandard, 5 );

		descriptor.BindBuffer( &g_oceanUniformBuffer, uboFragOffset, uboFragSize, 6 );

		descriptor.BindImage( g_oceanNormals[ 0 ], Samplers::m_samplerRepeat, 7 );
		descriptor.BindImage( g_oceanNormals[ 1 ], Samplers::m_samplerRepeat, 8 );
		descriptor.BindImage( g_oceanNormals[ 2 ], Samplers::m_samplerRepeat, 9 );
		descriptor.BindImage( g_oceanNormals[ 3 ], Samplers::m_samplerRepeat, 10 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_oceanPipelineDraw );

		g_oceanGridModel.DrawIndexed( cmdBuffer );
	}
}