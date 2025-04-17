//
//  DrawRaytracing.cpp
//
#include "Renderer/Common.h"
#include "Renderer/DrawRaytracing.h"
#include "Models/ModelStatic.h"
#include "Models/ModelManager.h"
#include "Graphics/Samplers.h"
#include "Graphics/Targa.h"
#include "Graphics/Barrier.h"
#include "Graphics/AccelerationStructure.h"
#include "Math/Math.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

static int g_numFrames = 0;

Pipeline	g_rtxSimplePipeline;
Descriptors	g_rtxSimpleDescriptors;

Pipeline	g_rtxMonteCarloPipeline;
Descriptors	g_rtxMonteCarloDescriptors;

Pipeline	g_rtxImportanceSamplingPipeline;
Descriptors	g_rtxImportanceSamplingDescriptors;

Pipeline	g_rtxDenoiserPipeline;
Descriptors	g_rtxDenoiserDescriptors;

Pipeline	g_rtxSVGFDenoiserAccumulatorPipeline;
Descriptors	g_rtxSVGFDenoiserAccumulatorDescriptors;

Pipeline	g_rtxSVGFDenoiserMomentsPipeline;
Descriptors	g_rtxSVGFDenoiserMomentsDescriptors;

Pipeline	g_rtxSVGFDenoiserWaveletPipeline;
Descriptors	g_rtxSVGFDenoiserWaveletDescriptors;

Pipeline	g_rtxDenoiserHolgerWaveletPipeline;
Descriptors	g_rtxDenoiserHolgerWaveletDescriptors;

Pipeline	g_rtxApplyDiffusePipeline;
Descriptors	g_rtxApplyDiffuseDescriptors;

struct path_t {
	// Whatever this is?
	// I guess it's pos + dir + color sample
	Vec3 start;
	int lightIdx;

	Vec3 end;
};
struct reservoir_t {
	path_t path;
	float weightSum;

	void AddSample( path_t x, float w ) {
		// Which is the correct one?
#if 0
		float weight = weightSum + w;
		if ( rand() < ( w / weight ) ) {
			path = x;
			weightSum = weight;
		}
#else
		weightSum = weightSum + w;
		if ( rand() < ( w / weightSum ) ) {
			path = x;
		}
#endif
	}
};
Buffer g_rtxReservoirBuffer;


Image * g_rtxImageOut = NULL;

extern FrameBuffer g_taaVelocityBuffer;

Image g_rtxImage;	// Lighting buffer
Image g_rtxImageAccumulated;
Image g_rtxImageDenoised;

Image g_gbufferHistory[ 3 ];

Image g_rtxImageHistory;
Image g_rtxImageHistoryCounters[ 2 ];

Image g_rtxImageLumaA;
Image g_rtxImageLumaB;
Image g_rtxImageLumaHistory;

AccelerationStructure g_rtxAccelerationStructure;

//#define ENABLE_RAYTRACING

extern const int g_numBaseLights;// = 128;
extern const int g_maxWorldLights;// = 1024;	// This should be enough lights for our needs
extern Buffer	g_lightsStorageBuffer;		// Storage buffer containing the list of point lights


//
//	Global Illumination Shaders + Images
//

Pipeline	g_rtxGIRaysPipeline;
Descriptors	g_rtxGIRaysDescriptors;

Pipeline	g_rtxGIAccumulatorPipeline;
Descriptors	g_rtxGIAccumulatorDescriptors;

Pipeline	g_rtxGIMomentsPipeline;
Descriptors	g_rtxGIMomentsDescriptors;

Pipeline	g_rtxGIWaveletPipeline;
Descriptors	g_rtxGIWaveletDescriptors;

Image g_rtxGIRawImages[ 3 ];	// The three raw SH images generated from the ray casting

Image g_rtxGIAccumulatedImages[ 3 ];
Image g_rtxGIAccumulatedImageHistory[ 3 ];
Image g_rtxGIImageHistoryCounters[ 2 ];

Image g_rtxGIImageDenoised[ 3 ];
Image * g_rtxGIImageOut[ 3 ];

Image g_rtxGIImageLumaA;
Image g_rtxGIImageLumaB;
Image g_rtxGIImageLumaHistory;

Buffer g_rtxGIReservoirBuffer;



/*
====================================================
InitGlobalIllumination
====================================================
*/
bool InitGlobalIllumination( DeviceContext * device, int width, int height ) {
	bool result = true;

	//
	//	GI Ray tracing Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_RAYTRACE;
		descriptorParms.numAccelerationStructures = 1;
		descriptorParms.numStorageImagesRayGen = 6;
		descriptorParms.numStorageBuffersRayGen = 2;
		descriptorParms.numStorageBuffersClosestHit = 3;
		descriptorParms.numStorageBuffersClosestHitArraySize = 8192;	// The max array size of the descriptors (the max number of render models in this case)
		result = g_rtxGIRaysDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxGIRaysDescriptors;
		pipelineParms.width = width;
		pipelineParms.height = height;
		pipelineParms.pushConstantSize = sizeof( int );
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_NV;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxGIRays" );
		result = g_rtxGIRaysPipeline.CreateRayTracing( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	GI Accumulator Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 20;
		result = g_rtxGIAccumulatorDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxGIAccumulatorDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxGIAccumulator" );
		result = g_rtxGIAccumulatorPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	GI Moments Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numUniformsCompute = 1;
		descriptorParms.numStorageImagesCompute = 12;
		result = g_rtxGIMomentsDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxGIMomentsDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxGIMoments" );
		result = g_rtxGIMomentsPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	GI Wavelet Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numUniformsCompute = 1;
		descriptorParms.numStorageImagesCompute = 12;
		result = g_rtxGIWaveletDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxGIWaveletDescriptors;
		pipelineParms.pushConstantSize = sizeof( int ) * 4;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxGIWavelet" );
		result = g_rtxGIWaveletPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	Create ReSTIR Reservoir
	//
	g_rtxGIReservoirBuffer.Allocate( device, NULL, width * height * sizeof( reservoir_t ), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	//
	//	Create Lighting Buffer
	//
	Image::CreateParms_t imageParms;
	imageParms.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageParms.width = width;
	imageParms.height = height;
	imageParms.depth = 1;
	imageParms.mipLevels = 1;
	imageParms.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
	for ( int i = 0; i < 3; i++ ) {
		result = g_rtxGIRawImages[ i ].Create( device, imageParms );
		if ( !result ) {
			printf( "Unable to build rtx gi image!\n" );
			assert( 0 );
			return false;
		}
	}

	for ( int i = 0; i < 3; i++ ) {
		result = g_rtxGIAccumulatedImages[ i ].Create( device, imageParms );
		if ( !result ) {
			printf( "Unable to build rtx gi accumulated image!\n" );
			assert( 0 );
			return false;
		}
	}

	for ( int i = 0; i < 3; i++ ) {
		result = g_rtxGIImageDenoised[ i ].Create( device, imageParms );
		if ( !result ) {
			printf( "Unable to build rtx gi accumulated image!\n" );
			assert( 0 );
			return false;
		}
	}

	for ( int i = 0; i < 3; i++ ) {
		result = g_rtxGIAccumulatedImageHistory[ i ].Create( device, imageParms );
		if ( !result ) {
			printf( "Unable to build rtx gi accumulated history image!\n" );
			assert( 0 );
			return false;
		}
	}

	result = g_rtxGIImageLumaA.Create( device, imageParms );
	result = result && g_rtxGIImageLumaB.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx gi luma image!\n" );
		assert( 0 );
		return false;
	}

	result = g_rtxGIImageLumaHistory.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx gi luma history image!\n" );
		assert( 0 );
		return false;
	}

	imageParms.format = VkFormat::VK_FORMAT_R32_UINT;
	result = g_rtxGIImageHistoryCounters[ 0 ].Create( device, imageParms );
	result = result && g_rtxGIImageHistoryCounters[ 1 ].Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx gi history image!\n" );
		assert( 0 );
		return false;
	}
	
	return true;
}


/*
====================================================
InitRaytracing
====================================================
*/
bool InitRaytracing( DeviceContext * device, int width, int height ) {
	bool result = false;
#if defined( ENABLE_RAYTRACING )
	result = InitGlobalIllumination( device, width, height );
	if ( !result ) {
		return false;
	}

	//
	//	Simple Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_RAYTRACE;
		descriptorParms.numAccelerationStructures = 1;
		descriptorParms.numUniformsRayGen = 1;
		descriptorParms.numStorageImagesRayGen = 1;
		//descriptorParms.numStorageBuffersRayGen = 1;
		descriptorParms.numStorageBuffersClosestHit = 2;
		descriptorParms.numStorageBuffersClosestHitArraySize = 8192;	// The max array size of the descriptors (the max number of render models in this case)
		result = g_rtxSimpleDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxSimpleDescriptors;
		pipelineParms.width = width;
		pipelineParms.height = height;
		pipelineParms.pushConstantSize = sizeof( int );
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_NV;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxCheckerboard" );
		result = g_rtxSimplePipeline.CreateRayTracing( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	Monte Carlo Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_RAYTRACE;
		descriptorParms.numAccelerationStructures = 1;
		descriptorParms.numUniformsRayGen = 1;
		descriptorParms.numStorageImagesRayGen = 4;
		descriptorParms.numStorageBuffersRayGen = 1;
		//descriptorParms.numStorageBuffersClosestHit = 2;
		result = g_rtxMonteCarloDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxMonteCarloDescriptors;
		pipelineParms.width = width;
		pipelineParms.height = height;
		pipelineParms.pushConstantSize = sizeof( int );
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_NV;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxMonteCarloLighting" );
		result = g_rtxMonteCarloPipeline.CreateRayTracing( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	ReSTIR Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_RAYTRACE;
		descriptorParms.numAccelerationStructures = 1;
		//descriptorParms.numUniformsRayGen = 1;
		descriptorParms.numStorageImagesRayGen = 4;
		descriptorParms.numStorageBuffersRayGen = 2;
		result = g_rtxImportanceSamplingDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxImportanceSamplingDescriptors;
		pipelineParms.width = width;
		pipelineParms.height = height;
		pipelineParms.pushConstantSize = sizeof( int );
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_NV;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxImportanceSampling" );
		result = g_rtxImportanceSamplingPipeline.CreateRayTracing( device, pipelineParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create ReSTIR Reservoir
		//
		g_rtxReservoirBuffer.Allocate( device, NULL, width * height * sizeof( reservoir_t ), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	}

	//
	//	Denoiser Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 5;
		result = g_rtxDenoiserDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxDenoiserDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxDenoiser" );
		result = g_rtxDenoiserPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	SVGF Denoiser Accumulator Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 14;
		result = g_rtxSVGFDenoiserAccumulatorDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxSVGFDenoiserAccumulatorDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxDenoiserSVGFAccumulator" );
		result = g_rtxSVGFDenoiserAccumulatorPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	SVGF Denoiser Moments Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numUniformsCompute = 1;
		descriptorParms.numStorageImagesCompute = 8;
		result = g_rtxSVGFDenoiserMomentsDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxSVGFDenoiserMomentsDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxDenoiserSVGFMoments" );
		result = g_rtxSVGFDenoiserMomentsPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	SVGF Denoiser Wavelet Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numUniformsCompute = 1;
		descriptorParms.numStorageImagesCompute = 7;
		result = g_rtxSVGFDenoiserWaveletDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxSVGFDenoiserWaveletDescriptors;
		pipelineParms.pushConstantSize = sizeof( int ) * 4;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxDenoiserSVGFWavelet" );
		result = g_rtxSVGFDenoiserWaveletPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}
	
	//
	//	Holger Denoiser Wavelet Pipeline
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 5;
		result = g_rtxDenoiserHolgerWaveletDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxDenoiserHolgerWaveletDescriptors;
		pipelineParms.pushConstantSize = sizeof( int ) * 4;
		pipelineParms.pushConstantShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxDenoiserHolgerWavelet" );
		result = g_rtxDenoiserHolgerWaveletPipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}
	
	//
	//	Apply Diffuse Channel
	//
	{
		//
		//	Create descriptors
		//
		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.type = Descriptors::TYPE_COMPUTE;
		descriptorParms.numStorageImagesCompute = 7;
		result = g_rtxApplyDiffuseDescriptors.Create( device, descriptorParms );
		if ( !result ) {
			return false;
		}

		//
		//	Create Pipeline
		//
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.descriptors = &g_rtxApplyDiffuseDescriptors;
		pipelineParms.shader = g_shaderManager->GetShader( "Raytracing/rtxApplyDiffuse" );
		result = g_rtxApplyDiffusePipeline.CreateCompute( device, pipelineParms );
		if ( !result ) {
			return false;
		}
	}

	//
	//	Create Lighting Buffer
	//
	Image::CreateParms_t imageParms;
	imageParms.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageParms.width = width;
	imageParms.height = height;
	imageParms.depth = 1;
	imageParms.mipLevels = 1;
	imageParms.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
	result = g_rtxImage.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx image!\n" );
		assert( 0 );
		return false;
	}

	result = g_rtxImageAccumulated.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx accumulator image!\n" );
		assert( 0 );
		return false;
	}

	result = g_rtxImageDenoised.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx denoise image!\n" );
		assert( 0 );
		return false;
	}

	result = g_rtxImageHistory.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx history image!\n" );
		assert( 0 );
		return false;
	}

	result = g_rtxImageLumaA.Create( device, imageParms );
	result = result && g_rtxImageLumaB.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx luma image!\n" );
		assert( 0 );
		return false;
	}

	result = g_rtxImageLumaHistory.Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx luma history image!\n" );
		assert( 0 );
		return false;
	}

	imageParms.format = VkFormat::VK_FORMAT_R32_UINT;
	result = g_rtxImageHistoryCounters[ 0 ].Create( device, imageParms );
	result = result && g_rtxImageHistoryCounters[ 1 ].Create( device, imageParms );
	if ( !result ) {
		printf( "Unable to build rtx history image!\n" );
		assert( 0 );
		return false;
	}

	for ( int i = 0; i < 3; i++ ) {
		imageParms.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		if ( 0 == i ) {
			imageParms.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		}

		result = g_gbufferHistory[ i ].Create( device, imageParms );
		if ( !result ) {
			printf( "Unable to build rtx gbuffer history image!\n" );
			assert( 0 );
			return false;
		}
	}
#endif
	return result;
}

/*
====================================================
CleanupRaytracing
====================================================
*/
void CleanupRaytracing( DeviceContext * device ) {
#if defined( ENABLE_RAYTRACING )
	g_rtxSimplePipeline.Cleanup( device );
	g_rtxSimpleDescriptors.Cleanup( device );

	g_rtxMonteCarloPipeline.Cleanup( device );
	g_rtxMonteCarloDescriptors.Cleanup( device );

	g_rtxImportanceSamplingPipeline.Cleanup( device );
	g_rtxImportanceSamplingDescriptors.Cleanup( device );

	g_rtxDenoiserPipeline.Cleanup( device );
	g_rtxDenoiserDescriptors.Cleanup( device );

	g_rtxDenoiserHolgerWaveletPipeline.Cleanup( device );
	g_rtxDenoiserHolgerWaveletDescriptors.Cleanup( device );

	g_rtxSVGFDenoiserAccumulatorPipeline.Cleanup( device );
	g_rtxSVGFDenoiserAccumulatorDescriptors.Cleanup( device );

	g_rtxSVGFDenoiserMomentsPipeline.Cleanup( device );
	g_rtxSVGFDenoiserMomentsDescriptors.Cleanup( device );

	g_rtxSVGFDenoiserWaveletPipeline.Cleanup( device );
	g_rtxSVGFDenoiserWaveletDescriptors.Cleanup( device );

	g_rtxApplyDiffusePipeline.Cleanup( device );
	g_rtxApplyDiffuseDescriptors.Cleanup( device );

	g_rtxReservoirBuffer.Cleanup( device );

	g_rtxImage.Cleanup( device );
	g_rtxImageHistory.Cleanup( device );
	g_rtxImageHistoryCounters[ 0 ].Cleanup( device );
	g_rtxImageHistoryCounters[ 1 ].Cleanup( device );
	g_rtxImageAccumulated.Cleanup( device );
	g_rtxImageDenoised.Cleanup( device );

	g_rtxImageLumaA.Cleanup( device );
	g_rtxImageLumaB.Cleanup( device );
	g_rtxImageLumaHistory.Cleanup( device );

	g_rtxAccelerationStructure.Cleanup( device );

	for ( int i = 0; i < 3; i++ ) {
		g_gbufferHistory[ i ].Cleanup( device );
	}

	//
	//	GI assets
	//
	g_rtxGIRaysPipeline.Cleanup( device );
	g_rtxGIRaysDescriptors.Cleanup( device );

	g_rtxGIAccumulatorPipeline.Cleanup( device );
	g_rtxGIAccumulatorDescriptors.Cleanup( device );

	g_rtxGIMomentsPipeline.Cleanup( device );
	g_rtxGIMomentsDescriptors.Cleanup( device );

	g_rtxGIWaveletPipeline.Cleanup( device );
	g_rtxGIWaveletDescriptors.Cleanup( device );

	g_rtxGIReservoirBuffer.Cleanup( device );

	for ( int i = 0; i < 3; i++ ) {
		g_rtxGIRawImages[ i ].Cleanup( device );
		g_rtxGIAccumulatedImages[ i ].Cleanup( device );
		g_rtxGIAccumulatedImageHistory[ i ].Cleanup( device );

		g_rtxGIImageDenoised[ i ].Cleanup( device );
	}

	g_rtxGIImageHistoryCounters[ 0 ].Cleanup( device );
	g_rtxGIImageHistoryCounters[ 1 ].Cleanup( device );

	g_rtxGIImageLumaA.Cleanup( device );
	g_rtxGIImageLumaB.Cleanup( device );
	g_rtxGIImageLumaHistory.Cleanup( device );
	
#endif
}

/*
====================================================
UpdateRaytracing
====================================================
*/
static bool g_isAccelerationBuilt = false;
void UpdateRaytracing( DeviceContext * device, const RenderModel * models, int numModels ) {
#if defined( ENABLE_RAYTRACING )
	if ( !g_isAccelerationBuilt ) {
		for ( int i = 0; i < numModels; i++ ) {
			g_rtxAccelerationStructure.AddGeometry( device, models[ i ].modelDraw, models[ i ].pos, models[ i ].orient );
		}
		g_isAccelerationBuilt = g_rtxAccelerationStructure.Build( device );
	} else {
//		g_rtxAccelerationStructure.UpdateInstances( device, models );
	}
#endif
}

/*
====================================================
TraceSimple
Simple checkerboard rendering for debug testing
====================================================
*/
void TraceSimple( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	Buffer * orientRTX = parms.storageOrientsRTX;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 4;

	g_rtxImage.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );

	//
	//	Simple Ray Tracing
	//
	{
		g_rtxSimplePipeline.BindPipelineRayTracing( cmdBuffer );
		g_rtxSimplePipeline.BindPushConstant( cmdBuffer, 0, sizeof( g_numFrames ), &g_numFrames );

		Descriptor descriptor = g_rtxSimplePipeline.GetRTXDescriptor();
		descriptor.BindAccelerationStructure( &g_rtxAccelerationStructure, 0 );

		descriptor.BindStorageImage( g_rtxImage, Samplers::m_samplerStandard, 1 );

		descriptor.BindBuffer( uniforms, camOffset, camSize, 2 );
		
		descriptor.BindRenderModelsRTX( parms.notCulledRenderModels, parms.numNotCulledRenderModels, 3, 4 );
//		descriptor.BindStorageBuffer( orientRTX, 0, orientRTX->m_vkBufferSize, 5 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxSimplePipeline );
		g_rtxSimplePipeline.TraceRays( cmdBuffer );
	}
}

/*
====================================================
TraceGI
Single bounce global illumination
====================================================
*/
void TraceGI( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	Buffer * orientRTX = parms.storageOrientsRTX;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 4;

	g_rtxGIRawImages[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIRawImages[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIRawImages[ 2 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIAccumulatedImages[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIAccumulatedImages[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIAccumulatedImages[ 2 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIAccumulatedImageHistory[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIAccumulatedImageHistory[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIAccumulatedImageHistory[ 2 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageDenoised[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageDenoised[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageDenoised[ 2 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageHistoryCounters[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageHistoryCounters[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageLumaA.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageLumaB.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );

	//
	//	GI Ray Tracing
	//
	{
		g_rtxGIRaysPipeline.BindPipelineRayTracing( cmdBuffer );
		g_rtxGIRaysPipeline.BindPushConstant( cmdBuffer, 0, sizeof( g_numFrames ), &g_numFrames );

		Descriptor descriptor = g_rtxGIRaysPipeline.GetRTXDescriptor();
		descriptor.BindAccelerationStructure( &g_rtxAccelerationStructure, 0 );

		descriptor.BindStorageImage( g_rtxGIRawImages[ 0 ], Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( g_rtxGIRawImages[ 1 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_rtxGIRawImages[ 2 ], Samplers::m_samplerStandard, 3 );

		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 5 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 6 );

		descriptor.BindStorageBuffer( &g_lightsStorageBuffer, 0, g_lightsStorageBuffer.m_vkBufferSize, 7 );
		descriptor.BindStorageBuffer( &g_rtxGIReservoirBuffer, 0, g_rtxGIReservoirBuffer.m_vkBufferSize, 8 );

		descriptor.BindRenderModelsRTX( parms.notCulledRenderModels, parms.numNotCulledRenderModels, 9, 10 );
		descriptor.BindStorageBuffer( orientRTX, 0, orientRTX->m_vkBufferSize, 11 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxGIRaysPipeline );
		g_rtxGIRaysPipeline.TraceRays( cmdBuffer );
	}

	g_rtxGIImageOut[ 0 ] = &g_rtxGIRawImages[ 0 ];
	g_rtxGIImageOut[ 1 ] = &g_rtxGIRawImages[ 1 ];
	g_rtxGIImageOut[ 2 ] = &g_rtxGIRawImages[ 2 ];
//	return;

	//
	//	GI Accumulator
	//
	{
		Image * imgHistoryCounterSrc = &g_rtxGIImageHistoryCounters[ ( g_numFrames + 0 ) & 1 ];
		Image * imgHistoryCounterDst = &g_rtxGIImageHistoryCounters[ ( g_numFrames + 1 ) & 1 ];

		g_rtxGIAccumulatorPipeline.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxGIAccumulatorPipeline.GetFreeDescriptor();
		descriptor.BindStorageImage( g_rtxGIAccumulatedImages[ 0 ], Samplers::m_samplerStandard, 0 );	// red
		descriptor.BindStorageImage( g_rtxGIAccumulatedImages[ 1 ], Samplers::m_samplerStandard, 1 );	// green
		descriptor.BindStorageImage( g_rtxGIAccumulatedImages[ 2 ], Samplers::m_samplerStandard, 2 );	// blue
		descriptor.BindStorageImage( g_rtxGIRawImages[ 0 ], Samplers::m_samplerStandard, 3 );	// red
		descriptor.BindStorageImage( g_rtxGIRawImages[ 1 ], Samplers::m_samplerStandard, 4 );	// green
		descriptor.BindStorageImage( g_rtxGIRawImages[ 2 ], Samplers::m_samplerStandard, 5 );	// blue
		
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 6 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 7 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 8 );
		descriptor.BindStorageImage( g_gbufferHistory[ 0 ], Samplers::m_samplerStandard, 9 );
		descriptor.BindStorageImage( g_gbufferHistory[ 1 ], Samplers::m_samplerStandard, 10 );
		descriptor.BindStorageImage( g_gbufferHistory[ 2 ], Samplers::m_samplerStandard, 11 );

		descriptor.BindStorageImage( g_taaVelocityBuffer.m_imageColor, Samplers::m_samplerStandard, 12 );

		descriptor.BindStorageImage( g_rtxGIAccumulatedImageHistory[ 0 ], Samplers::m_samplerStandard, 13 );	// red
		descriptor.BindStorageImage( g_rtxGIAccumulatedImageHistory[ 1 ], Samplers::m_samplerStandard, 14 );	// green
		descriptor.BindStorageImage( g_rtxGIAccumulatedImageHistory[ 2 ], Samplers::m_samplerStandard, 15 );	// blue
		descriptor.BindStorageImage( *imgHistoryCounterSrc, Samplers::m_samplerStandard, 16 );
		descriptor.BindStorageImage( *imgHistoryCounterDst, Samplers::m_samplerStandard, 17 );
		
		descriptor.BindStorageImage( g_rtxGIImageLumaA, Samplers::m_samplerStandard, 18 );
		descriptor.BindStorageImage( g_rtxGIImageLumaHistory, Samplers::m_samplerStandard, 19 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxGIAccumulatorPipeline );
		g_rtxGIAccumulatorPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );
	}

	MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_rtxGIAccumulatedImages[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );
	MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_rtxGIAccumulatedImages[ 1 ], VK_IMAGE_ASPECT_COLOR_BIT );
	MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_rtxGIAccumulatedImages[ 2 ], VK_IMAGE_ASPECT_COLOR_BIT );
	int startIdx = 0;

	g_rtxGIImageOut[ 0 ] = &g_rtxGIAccumulatedImages[ 0 ];
	g_rtxGIImageOut[ 1 ] = &g_rtxGIAccumulatedImages[ 1 ];
	g_rtxGIImageOut[ 2 ] = &g_rtxGIAccumulatedImages[ 2 ];
//	return;

	//
	//	GI Temporal Variance Filter
	//
	{
		int i = 0;
		Image * srcImages[ 3 ];
		Image * dstImages[ 3 ];
		srcImages[ 0 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIAccumulatedImages[ 0 ] : &g_rtxGIImageDenoised[ 0 ];
		dstImages[ 0 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageDenoised[ 0 ] : &g_rtxGIAccumulatedImages[ 0 ];
		srcImages[ 1 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIAccumulatedImages[ 1 ] : &g_rtxGIImageDenoised[ 1 ];
		dstImages[ 1 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageDenoised[ 1 ] : &g_rtxGIAccumulatedImages[ 1 ];
		srcImages[ 2 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIAccumulatedImages[ 2 ] : &g_rtxGIImageDenoised[ 2 ];
		dstImages[ 2 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageDenoised[ 2 ] : &g_rtxGIAccumulatedImages[ 2 ];

		Image * srcLuma = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageLumaA : &g_rtxGIImageLumaB;
		Image * dstLuma = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageLumaB : &g_rtxGIImageLumaA;

		// The source and destination history counters have swapped after the accumulator pass
		Image * imgHistoryCounterSrc = &g_rtxGIImageHistoryCounters[ ( g_numFrames + 1 ) & 1 ];
		Image * imgHistoryCounterDst = &g_rtxGIImageHistoryCounters[ ( g_numFrames + 0 ) & 1 ];

		g_rtxGIMomentsPipeline.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxGIMomentsPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );

		descriptor.BindStorageImage( *dstImages[ 0 ], Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( *dstImages[ 1 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( *dstImages[ 2 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( *srcImages[ 0 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindStorageImage( *srcImages[ 1 ], Samplers::m_samplerStandard, 5 );
		descriptor.BindStorageImage( *srcImages[ 2 ], Samplers::m_samplerStandard, 6 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 7 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 8 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 9 );

		descriptor.BindStorageImage( *srcLuma, Samplers::m_samplerStandard, 10 );
		descriptor.BindStorageImage( *dstLuma, Samplers::m_samplerStandard, 11 );

		descriptor.BindStorageImage( *imgHistoryCounterSrc, Samplers::m_samplerStandard, 12 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxGIMomentsPipeline );
		g_rtxGIMomentsPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );

		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImages[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImages[ 1 ], VK_IMAGE_ASPECT_COLOR_BIT );
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImages[ 2 ], VK_IMAGE_ASPECT_COLOR_BIT );

		startIdx = 1;

		// Copy the first temporal filtered image into the history buffer
		for ( int i = 0; i < 3; i++ ) {
			dstImages[ i ]->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
			g_rtxGIAccumulatedImageHistory[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

			Image::CopyImage( g_rtxGIAccumulatedImageHistory[ i ], *dstImages[ i ], cmdBuffer );

			dstImages[ i ]->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
			g_rtxGIAccumulatedImageHistory[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
		}

		// Copy the luma image into the luma history buffer
		{
			dstLuma->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
			g_rtxGIImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

			Image::CopyImage( g_rtxGIImageLumaHistory, *dstLuma, cmdBuffer );

			dstLuma->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
			g_rtxGIImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
		}

		g_rtxGIImageOut[ 0 ] = dstImages[ 0 ];
		g_rtxGIImageOut[ 1 ] = dstImages[ 1 ];
		g_rtxGIImageOut[ 2 ] = dstImages[ 2 ];
//		return;
	}

	//
	//	GI Wavelet Filter
	//
	for ( int i = startIdx; i < 5 + startIdx; i++ ) {
		int inters[4];
		inters[ 0 ] = i;
		inters[ 1 ] = i;
		inters[ 2 ] = i;
		inters[ 3 ] = i;

		Image * srcImages[ 3 ];
		Image * dstImages[ 3 ];
		srcImages[ 0 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIAccumulatedImages[ 0 ] : &g_rtxGIImageDenoised[ 0 ];
		dstImages[ 0 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageDenoised[ 0 ] : &g_rtxGIAccumulatedImages[ 0 ];
		srcImages[ 1 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIAccumulatedImages[ 1 ] : &g_rtxGIImageDenoised[ 1 ];
		dstImages[ 1 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageDenoised[ 1 ] : &g_rtxGIAccumulatedImages[ 1 ];
		srcImages[ 2 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIAccumulatedImages[ 2 ] : &g_rtxGIImageDenoised[ 2 ];
		dstImages[ 2 ] = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageDenoised[ 2 ] : &g_rtxGIAccumulatedImages[ 2 ];

		Image * srcLuma = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageLumaA : &g_rtxGIImageLumaB;
		Image * dstLuma = ( ( i & 1 ) == 0 ) ? &g_rtxGIImageLumaB : &g_rtxGIImageLumaA;

		g_rtxGIWaveletPipeline.BindPipelineCompute( cmdBuffer );
		g_rtxGIWaveletPipeline.BindPushConstant( cmdBuffer, 0, sizeof( int ) * 4, inters );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxGIWaveletPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );

		descriptor.BindStorageImage( *dstImages[ 0 ], Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( *dstImages[ 1 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( *dstImages[ 2 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( *srcImages[ 0 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindStorageImage( *srcImages[ 1 ], Samplers::m_samplerStandard, 5 );
		descriptor.BindStorageImage( *srcImages[ 2 ], Samplers::m_samplerStandard, 6 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 7 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 8 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 9 );

		descriptor.BindStorageImage( *srcLuma, Samplers::m_samplerStandard, 10 );
		descriptor.BindStorageImage( *dstLuma, Samplers::m_samplerStandard, 11 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxGIWaveletPipeline );
		g_rtxGIWaveletPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );

		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImages[ 0 ], VK_IMAGE_ASPECT_COLOR_BIT );
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImages[ 1 ], VK_IMAGE_ASPECT_COLOR_BIT );
		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImages[ 2 ], VK_IMAGE_ASPECT_COLOR_BIT );

		g_rtxGIImageOut[ 0 ] = dstImages[ 0 ];
		g_rtxGIImageOut[ 1 ] = dstImages[ 1 ];
		g_rtxGIImageOut[ 2 ] = dstImages[ 2 ];
	}
}

/*
====================================================
TraceDL
The direct lighting
====================================================
*/
void TraceDL( DrawParms_t & parms ) {
	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 4;

	const int shadowCamOffset = camOffset + camSize;
	const int shadowCamSize = camSize;

	const int timeOffset = shadowCamOffset + shadowCamSize;
	const int timeSize = sizeof( Vec4 );
	
	//
	//	Ray Tracing
	//
#if 0	// 1 = Monte Carlo, 0 = Importance Sampling
	{
		g_rtxMonteCarloPipeline.BindPipelineRayTracing( cmdBuffer );
		g_rtxMonteCarloPipeline.BindPushConstant( cmdBuffer, 0, sizeof( g_numFrames ), &g_numFrames );

		Descriptor descriptor = g_rtxMonteCarloPipeline.GetRTXDescriptor();
		descriptor.BindAccelerationStructure( &g_rtxAccelerationStructure, 0 );
		descriptor.BindStorageImage( g_rtxImage, Samplers::m_samplerStandard, 1 );

		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 4 );

		descriptor.BindStorageBuffer( &g_lightsStorageBuffer, 0, g_lightsStorageBuffer.m_vkBufferSize, 5 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxMonteCarloPipeline );
		g_rtxMonteCarloPipeline.TraceRays( cmdBuffer );
	}
#else
	{
		g_rtxImportanceSamplingPipeline.BindPipelineRayTracing( cmdBuffer );
		g_rtxImportanceSamplingPipeline.BindPushConstant( cmdBuffer, 0, sizeof( g_numFrames ), &g_numFrames );

		Descriptor descriptor = g_rtxImportanceSamplingPipeline.GetRTXDescriptor();
		descriptor.BindAccelerationStructure( &g_rtxAccelerationStructure, 0 );
		descriptor.BindStorageImage( g_rtxImage, Samplers::m_samplerStandard, 1 );

		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 4 );

		descriptor.BindStorageBuffer( &g_lightsStorageBuffer, 0, g_lightsStorageBuffer.m_vkBufferSize, 5 );
		descriptor.BindStorageBuffer( &g_rtxReservoirBuffer, 0, g_rtxReservoirBuffer.m_vkBufferSize, 6 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxImportanceSamplingPipeline );
		g_rtxImportanceSamplingPipeline.TraceRays( cmdBuffer );
	}
#endif
	g_rtxImageOut = &g_rtxImage;

	MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_rtxImage, VK_IMAGE_ASPECT_COLOR_BIT );

	//
	//	Denoising
	//
#if 0
	{
		g_rtxDenoiserPipeline.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxDenoiserPipeline.GetFreeDescriptor();
		descriptor.BindStorageImage( g_rtxImageDenoised, Samplers::m_samplerStandard, 0 );
		descriptor.BindStorageImage( g_rtxImage, Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxDenoiserPipeline );
		g_rtxDenoiserPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );
	}
#else
	// Accumulator
	{
		Image * imgHistoryCounterSrc = &g_rtxImageHistoryCounters[ ( g_numFrames + 0 ) & 1 ];
		Image * imgHistoryCounterDst = &g_rtxImageHistoryCounters[ ( g_numFrames + 1 ) & 1 ];

		g_rtxSVGFDenoiserAccumulatorPipeline.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxSVGFDenoiserAccumulatorPipeline.GetFreeDescriptor();
		descriptor.BindStorageImage( g_rtxImageAccumulated, Samplers::m_samplerStandard, 0 );
		descriptor.BindStorageImage( g_rtxImage, Samplers::m_samplerStandard, 1 );
		
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 4 );

		descriptor.BindStorageImage( g_rtxImageHistory, Samplers::m_samplerStandard, 5 );
		descriptor.BindStorageImage( *imgHistoryCounterSrc, Samplers::m_samplerStandard, 6 );
		descriptor.BindStorageImage( *imgHistoryCounterDst, Samplers::m_samplerStandard, 7 );

		descriptor.BindStorageImage( g_gbufferHistory[ 0 ], Samplers::m_samplerStandard, 8 );
		descriptor.BindStorageImage( g_gbufferHistory[ 1 ], Samplers::m_samplerStandard, 9 );
		descriptor.BindStorageImage( g_gbufferHistory[ 2 ], Samplers::m_samplerStandard, 10 );

		descriptor.BindStorageImage( g_taaVelocityBuffer.m_imageColor, Samplers::m_samplerStandard, 11 );

		descriptor.BindStorageImage( g_rtxImageLumaA, Samplers::m_samplerStandard, 12 );
		descriptor.BindStorageImage( g_rtxImageLumaHistory, Samplers::m_samplerStandard, 13 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxSVGFDenoiserAccumulatorPipeline );
		g_rtxSVGFDenoiserAccumulatorPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );
	}
	g_rtxImageOut = &g_rtxImageAccumulated;

	MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, g_rtxImageAccumulated, VK_IMAGE_ASPECT_COLOR_BIT );
	int startIdx = 0;

#if 0
	// Copy the accumulated image into the history buffer
	{
		g_rtxImageAccumulated.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
		g_rtxImageHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

		Image::CopyImage( g_rtxImageHistory, g_rtxImageAccumulated, cmdBuffer );

		g_rtxImageAccumulated.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
		g_rtxImageHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	}

	// Copy the luma image into the luma history buffer
	{
		g_rtxImageLumaA.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
		g_rtxImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

		Image::CopyImage( g_rtxImageLumaHistory, g_rtxImageLumaA, cmdBuffer );

		g_rtxImageLumaA.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
		g_rtxImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	}

	//g_rtxImageOut = &g_rtxImageLuma;

#else
	//
	//	SVGF Temporal Variance Filter
	//
	{
		int i = 0;
		Image * srcImage = ( ( i & 1 ) == 0 ) ? &g_rtxImageAccumulated : &g_rtxImageDenoised;
		Image * dstImage = ( ( i & 1 ) == 0 ) ? &g_rtxImageDenoised : &g_rtxImageAccumulated;

		Image * srcLuma = ( ( i & 1 ) == 0 ) ? &g_rtxImageLumaA : &g_rtxImageLumaB;
		Image * dstLuma = ( ( i & 1 ) == 0 ) ? &g_rtxImageLumaB : &g_rtxImageLumaA;

		// The source and destination history counters have swapped after the accumulator pass
		Image * imgHistoryCounterSrc = &g_rtxImageHistoryCounters[ ( g_numFrames + 1 ) & 1 ];
		Image * imgHistoryCounterDst = &g_rtxImageHistoryCounters[ ( g_numFrames + 0 ) & 1 ];

		g_rtxSVGFDenoiserMomentsPipeline.BindPipelineCompute( cmdBuffer );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxSVGFDenoiserMomentsPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );

		descriptor.BindStorageImage( *dstImage, Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( *srcImage, Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 5 );

		descriptor.BindStorageImage( *srcLuma, Samplers::m_samplerStandard, 6 );
		descriptor.BindStorageImage( *dstLuma, Samplers::m_samplerStandard, 7 );

		descriptor.BindStorageImage( *imgHistoryCounterSrc, Samplers::m_samplerStandard, 8 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxSVGFDenoiserMomentsPipeline );
		g_rtxSVGFDenoiserMomentsPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );

		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImage, VK_IMAGE_ASPECT_COLOR_BIT );

		g_rtxImageOut = dstImage;
		startIdx = 1;

		// Copy the first temporal filtered image into the history buffer
		{
			dstImage->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
			g_rtxImageHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

			Image::CopyImage( g_rtxImageHistory, *dstImage, cmdBuffer );

			dstImage->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
			g_rtxImageHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
		}

		// Copy the luma image into the luma history buffer
		{
			dstLuma->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
			g_rtxImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

			Image::CopyImage( g_rtxImageLumaHistory, *dstLuma, cmdBuffer );

			dstLuma->TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
			g_rtxImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
		}
	}
#endif
#if 1
	// SVGF Edge Avoiding Wavelet Filter
	for ( int i = startIdx; i < 5 + startIdx; i++ ) {
		int inters[4];
		inters[ 0 ] = i;
		inters[ 1 ] = i;
		inters[ 2 ] = i;
		inters[ 3 ] = i;

		Image * srcImage = ( ( i & 1 ) == 0 ) ? &g_rtxImageAccumulated : &g_rtxImageDenoised;
		Image * dstImage = ( ( i & 1 ) == 0 ) ? &g_rtxImageDenoised : &g_rtxImageAccumulated;

		Image * srcLuma = ( ( i & 1 ) == 0 ) ? &g_rtxImageLumaA : &g_rtxImageLumaB;
		Image * dstLuma = ( ( i & 1 ) == 0 ) ? &g_rtxImageLumaB : &g_rtxImageLumaA;

		g_rtxSVGFDenoiserWaveletPipeline.BindPipelineCompute( cmdBuffer );
		g_rtxSVGFDenoiserWaveletPipeline.BindPushConstant( cmdBuffer, 0, sizeof( int ) * 4, inters );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxSVGFDenoiserWaveletPipeline.GetFreeDescriptor();
		descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );

		descriptor.BindStorageImage( *dstImage, Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( *srcImage, Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 5 );

		descriptor.BindStorageImage( *srcLuma, Samplers::m_samplerStandard, 6 );
		descriptor.BindStorageImage( *dstLuma, Samplers::m_samplerStandard, 7 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxSVGFDenoiserWaveletPipeline );
		g_rtxSVGFDenoiserWaveletPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );

		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImage, VK_IMAGE_ASPECT_COLOR_BIT );

		g_rtxImageOut = dstImage;
		//g_rtxImageOut = dstLuma;
	}
#else
#if 0
	// Holger Edge Avoiding Wavelet Filter
	for ( int i = 0; i < 3; i++ ) {
		int inters[4];
		inters[ 0 ] = i;
		inters[ 1 ] = i;
		inters[ 2 ] = i;
		inters[ 3 ] = i;

		Image * srcImage = ( ( i & 1 ) == 0 ) ? &g_rtxImageAccumulated : &g_rtxImageDenoised;
		Image * dstImage = ( ( i & 1 ) == 0 ) ? &g_rtxImageDenoised : &g_rtxImageAccumulated;

		g_rtxDenoiserHolgerWaveletPipeline.BindPipelineCompute( cmdBuffer );
		g_rtxDenoiserHolgerWaveletPipeline.BindPushConstant( cmdBuffer, 0, sizeof( int ) * 4, inters );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxDenoiserHolgerWaveletPipeline.GetFreeDescriptor();

		descriptor.BindStorageImage( *dstImage, Samplers::m_samplerStandard, 0 );
		descriptor.BindStorageImage( *srcImage, Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 3 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 4 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxDenoiserHolgerWaveletPipeline );
		g_rtxDenoiserHolgerWaveletPipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );

		MemoryBarriers::CreateImageMemoryBarrier( cmdBuffer, *dstImage, VK_IMAGE_ASPECT_COLOR_BIT );

		g_rtxImageOut = dstImage;
	}
#endif
#endif
#endif
}

/*
====================================================
DrawRaytracing
====================================================
*/
void DrawRaytracing( DrawParms_t & parms ) {
#if defined( ENABLE_RAYTRACING )
#if 0	// enable to do simple ray tracing instead of lighting
	TraceSimple( parms );
	return;
#endif

	DeviceContext * device = parms.device;
	int cmdBufferIndex = parms.cmdBufferIndex;
	Buffer * uniforms = parms.uniforms;
	const RenderModel * renderModels = parms.renderModels;
	const int numModels = parms.numModels;
	VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[ cmdBufferIndex ];

	const int camOffset = 0;
	const int camSize = sizeof( float ) * 16 * 4;

	const int shadowCamOffset = camOffset + camSize;
	const int shadowCamSize = camSize;

	const int timeOffset = shadowCamOffset + shadowCamSize;
	const int timeSize = sizeof( Vec4 );

	g_numFrames++;
	
	// Make sure the rtx image is in general layout
	g_rtxImage.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxImageAccumulated.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxImageDenoised.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxImageHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxImageHistoryCounters[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxImageHistoryCounters[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_gbufferHistory[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_gbufferHistory[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_gbufferHistory[ 2 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );

	g_rtxImageLumaA.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxImageLumaB.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxImageLumaHistory.TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );

	g_rtxGIRawImages[ 0 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIRawImages[ 1 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	g_rtxGIRawImages[ 2 ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );

	g_rtxGIImageOut[ 0 ] = &g_rtxGIRawImages[ 0 ];
	g_rtxGIImageOut[ 1 ] = &g_rtxGIRawImages[ 1 ];
	g_rtxGIImageOut[ 2 ] = &g_rtxGIRawImages[ 2 ];

	TraceDL( parms );

	TraceGI( parms );
	
	//
	//	Apply the lighting to the diffuse channel of the gbuffer
	//
#if 1
	{
		g_rtxApplyDiffusePipeline.BindPipelineCompute( cmdBuffer );
		//g_rtxApplyDiffusePipeline.BindPushConstant( cmdBuffer, 0, sizeof( int ) * 4, inters );

		// Descriptor is how we bind our buffers and images
		Descriptor descriptor = g_rtxApplyDiffusePipeline.GetFreeDescriptor();

		descriptor.BindStorageImage( *g_rtxImageOut, Samplers::m_samplerStandard, 0 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 0 ], Samplers::m_samplerStandard, 1 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 1 ], Samplers::m_samplerStandard, 2 );
		descriptor.BindStorageImage( g_gbuffer.m_imageColor[ 2 ], Samplers::m_samplerStandard, 3 );

		descriptor.BindStorageImage( *g_rtxGIImageOut[ 0 ], Samplers::m_samplerStandard, 4 );
		descriptor.BindStorageImage( *g_rtxGIImageOut[ 1 ], Samplers::m_samplerStandard, 5 );
		descriptor.BindStorageImage( *g_rtxGIImageOut[ 2 ], Samplers::m_samplerStandard, 6 );

		descriptor.BindDescriptor( device, cmdBuffer, &g_rtxApplyDiffusePipeline );
		g_rtxApplyDiffusePipeline.DispatchCompute( cmdBuffer, ( 1920 + 7 ) / 8, ( 1080 + 7 ) / 8, 1 );
	}
#endif

	//g_rtxImageOut = &g_rtxImage;

	//
	//	Copy the gbuffer into the history gbuffer
	//
	for ( int i = 0; i < 3; i++ ) {
		g_gbuffer.m_imageColor[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
		g_gbufferHistory[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

		Image::CopyImage( g_gbufferHistory[ i ], g_gbuffer.m_imageColor[ i ], cmdBuffer );

 		g_gbuffer.m_imageColor[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
 		g_gbufferHistory[ i ].TransitionLayout( cmdBuffer, VK_IMAGE_LAYOUT_GENERAL );
	}
#endif
}
