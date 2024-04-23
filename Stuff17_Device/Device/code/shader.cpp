//
//  shader.cpp
//
#include "shader.h"
#include "Fileio.h"
#include <assert.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>

/*
====================================================
Shader::CreateShaderModule
====================================================
*/
VkShaderModule Shader::CreateShaderModule( VkDevice vkDevice, const char * code, const int size ) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = (const uint32_t *)code;

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule( vkDevice, &createInfo, nullptr, &shaderModule );
	if ( VK_SUCCESS != result ) {
		printf( "Failed to create shader module\n" );
		assert( 0 );
	}

	return shaderModule;
}

/*
====================================================
InitBuiltInResource
These values are borrowed from a sample in the Vulkan SDK Samples\API-Samples\utils\util.cpp
====================================================
*/
void InitBuiltInResource( TBuiltInResource & Resources ) {
	Resources.maxLights = 32;
	Resources.maxClipPlanes = 6;
	Resources.maxTextureUnits = 32;
	Resources.maxTextureCoords = 32;
	Resources.maxVertexAttribs = 64;
	Resources.maxVertexUniformComponents = 4096;
	Resources.maxVaryingFloats = 64;
	Resources.maxVertexTextureImageUnits = 32;
	Resources.maxCombinedTextureImageUnits = 80;
	Resources.maxTextureImageUnits = 32;
	Resources.maxFragmentUniformComponents = 4096;
	Resources.maxDrawBuffers = 32;
	Resources.maxVertexUniformVectors = 128;
	Resources.maxVaryingVectors = 8;
	Resources.maxFragmentUniformVectors = 16;
	Resources.maxVertexOutputVectors = 16;
	Resources.maxFragmentInputVectors = 15;
	Resources.minProgramTexelOffset = -8;
	Resources.maxProgramTexelOffset = 7;
	Resources.maxClipDistances = 8;
	Resources.maxComputeWorkGroupCountX = 65535;
	Resources.maxComputeWorkGroupCountY = 65535;
	Resources.maxComputeWorkGroupCountZ = 65535;
	Resources.maxComputeWorkGroupSizeX = 1024;
	Resources.maxComputeWorkGroupSizeY = 1024;
	Resources.maxComputeWorkGroupSizeZ = 64;
	Resources.maxComputeUniformComponents = 1024;
	Resources.maxComputeTextureImageUnits = 16;
	Resources.maxComputeImageUniforms = 8;
	Resources.maxComputeAtomicCounters = 8;
	Resources.maxComputeAtomicCounterBuffers = 1;
	Resources.maxVaryingComponents = 60;
	Resources.maxVertexOutputComponents = 64;
	Resources.maxGeometryInputComponents = 64;
	Resources.maxGeometryOutputComponents = 128;
	Resources.maxFragmentInputComponents = 128;
	Resources.maxImageUnits = 8;
	Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	Resources.maxCombinedShaderOutputResources = 8;
	Resources.maxImageSamples = 0;
	Resources.maxVertexImageUniforms = 0;
	Resources.maxTessControlImageUniforms = 0;
	Resources.maxTessEvaluationImageUniforms = 0;
	Resources.maxGeometryImageUniforms = 0;
	Resources.maxFragmentImageUniforms = 8;
	Resources.maxCombinedImageUniforms = 8;
	Resources.maxGeometryTextureImageUnits = 16;
	Resources.maxGeometryOutputVertices = 256;
	Resources.maxGeometryTotalOutputComponents = 1024;
	Resources.maxGeometryUniformComponents = 1024;
	Resources.maxGeometryVaryingComponents = 64;
	Resources.maxTessControlInputComponents = 128;
	Resources.maxTessControlOutputComponents = 128;
	Resources.maxTessControlTextureImageUnits = 16;
	Resources.maxTessControlUniformComponents = 1024;
	Resources.maxTessControlTotalOutputComponents = 4096;
	Resources.maxTessEvaluationInputComponents = 128;
	Resources.maxTessEvaluationOutputComponents = 128;
	Resources.maxTessEvaluationTextureImageUnits = 16;
	Resources.maxTessEvaluationUniformComponents = 1024;
	Resources.maxTessPatchComponents = 120;
	Resources.maxPatchVertices = 32;
	Resources.maxTessGenLevel = 64;
	Resources.maxViewports = 16;
	Resources.maxVertexAtomicCounters = 0;
	Resources.maxTessControlAtomicCounters = 0;
	Resources.maxTessEvaluationAtomicCounters = 0;
	Resources.maxGeometryAtomicCounters = 0;
	Resources.maxFragmentAtomicCounters = 8;
	Resources.maxCombinedAtomicCounters = 8;
	Resources.maxAtomicCounterBindings = 1;
	Resources.maxVertexAtomicCounterBuffers = 0;
	Resources.maxTessControlAtomicCounterBuffers = 0;
	Resources.maxTessEvaluationAtomicCounterBuffers = 0;
	Resources.maxGeometryAtomicCounterBuffers = 0;
	Resources.maxFragmentAtomicCounterBuffers = 1;
	Resources.maxCombinedAtomicCounterBuffers = 1;
	Resources.maxAtomicCounterBufferSize = 16384;
	Resources.maxTransformFeedbackBuffers = 4;
	Resources.maxTransformFeedbackInterleavedComponents = 64;
	Resources.maxCullDistances = 8;
	Resources.maxCombinedClipAndCullDistances = 8;
	Resources.maxSamples = 4;
	Resources.maxMeshOutputVerticesNV = 256;
	Resources.maxMeshOutputPrimitivesNV = 512;
	Resources.maxMeshWorkGroupSizeX_NV = 32;
	Resources.maxMeshWorkGroupSizeY_NV = 1;
	Resources.maxMeshWorkGroupSizeZ_NV = 1;
	Resources.maxTaskWorkGroupSizeX_NV = 32;
	Resources.maxTaskWorkGroupSizeY_NV = 1;
	Resources.maxTaskWorkGroupSizeZ_NV = 1;
	Resources.maxMeshViewCountNV = 4;
	Resources.limits.nonInductiveForLoops = 1;
	Resources.limits.whileLoops = 1;
	Resources.limits.doWhileLoops = 1;
	Resources.limits.generalUniformIndexing = 1;
	Resources.limits.generalAttributeMatrixVectorIndexing = 1;
	Resources.limits.generalVaryingIndexing = 1;
	Resources.limits.generalSamplerIndexing = 1;
	Resources.limits.generalVariableIndexing = 1;
	Resources.limits.generalConstantMatrixVectorIndexing = 1;
}

/*
====================================================
FindLanguage
====================================================
*/
EShLanguage FindLanguage( const VkShaderStageFlagBits shaderStage ) {
	EShLanguage language;
	switch ( shaderStage ) {
		default:
		case VK_SHADER_STAGE_VERTEX_BIT: { language = EShLangVertex; } break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: { language = EShLangTessControl; } break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: { language = EShLangTessEvaluation; } break;
		case VK_SHADER_STAGE_GEOMETRY_BIT: { language = EShLangGeometry; } break;
		case VK_SHADER_STAGE_FRAGMENT_BIT: { language = EShLangFragment; } break;
		case VK_SHADER_STAGE_COMPUTE_BIT: { language = EShLangCompute; } break;
		// New Stuff, time to buy a new graphics card :(
		case VK_SHADER_STAGE_RAYGEN_BIT_NV: { language = EShLangRayGenNV; } break;
		case VK_SHADER_STAGE_ANY_HIT_BIT_NV: { language = EShLangAnyHitNV; } break;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV: { language = EShLangClosestHitNV; } break;
		case VK_SHADER_STAGE_MISS_BIT_NV: { language = EShLangMissNV; } break;
		case VK_SHADER_STAGE_INTERSECTION_BIT_NV: { language = EShLangIntersectNV; } break;
		case VK_SHADER_STAGE_CALLABLE_BIT_NV: { language = EShLangCallableNV; } break;
		case VK_SHADER_STAGE_TASK_BIT_NV: { language = EShLangTaskNV; } break;
		case VK_SHADER_STAGE_MESH_BIT_NV: { language = EShLangMeshNV; } break;
	}
	return language;
}

/*
====================================================
GLSLtoSPV
====================================================
*/
bool GLSLtoSPV( const VkShaderStageFlagBits shaderStage, const char * pshader, std::vector< unsigned int > & spirv ) {
	EShLanguage stage = FindLanguage( shaderStage );

	glslang::TShader shader( stage );
	glslang::TProgram program;
	const char * shaderStrings[ 1 ];

	TBuiltInResource Resources;
	InitBuiltInResource( Resources );

	// Enable SPIR-V and Vulkan rules when parsing GLSL
	EShMessages messages = (EShMessages)( EShMsgSpvRules | EShMsgVulkanRules );

	shaderStrings[ 0 ] = pshader;
	shader.setStrings( shaderStrings, 1 );

	if ( !shader.parse( &Resources, 100, false, messages ) ) {
		printf( shader.getInfoLog() );
		printf( shader.getInfoDebugLog() );
		return false;  // something didn't work
	}
	printf( shader.getInfoLog() );

	program.addShader( &shader );

	//
	// Program-level processing...
	//

	if ( !program.link( messages ) ) {
		printf( shader.getInfoLog() );
		printf( shader.getInfoDebugLog() );
		fflush( stdout );
		return false;
	}

	glslang::GlslangToSpv( *program.getIntermediate( stage ), spirv );
	return true;
}

/*
====================================================
Shader::CompileToSPIRV
====================================================
*/
bool Shader::CompileToSPIRV( const shaderStage_t stage, const char * glslCode, std::vector< unsigned int > & spirvCode ) {
	glslang::InitializeProcess();

	VkShaderStageFlagBits shaderStage;
	switch ( stage ) {
		default:
		case shaderStage_t::SHADER_STAGE_VERTEX: { shaderStage = VK_SHADER_STAGE_VERTEX_BIT; } break;
		case shaderStage_t::SHADER_STAGE_TESSELLATION_CONTROL: { shaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; } break;
		case shaderStage_t::SHADER_STAGE_TESSELLATION_EVALUATION: { shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; } break;
		case shaderStage_t::SHADER_STAGE_GEOMETRY: { shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT; } break;
		case shaderStage_t::SHADER_STAGE_FRAGMENT: { shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT; } break;
		case shaderStage_t::SHADER_STAGE_COMPUTE: { shaderStage = VK_SHADER_STAGE_COMPUTE_BIT; } break;
		// New Stuff, time to buy a new graphics card :(
		case shaderStage_t::SHADER_STAGE_RAYGEN: { shaderStage = VK_SHADER_STAGE_RAYGEN_BIT_NV; } break;
		case shaderStage_t::SHADER_STAGE_ANY_HIT: { shaderStage = VK_SHADER_STAGE_ANY_HIT_BIT_NV; } break;
		case shaderStage_t::SHADER_STAGE_CLOSEST_HIT: { shaderStage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV; } break;
		case shaderStage_t::SHADER_STAGE_MISS: { shaderStage = VK_SHADER_STAGE_MISS_BIT_NV; } break;
		case shaderStage_t::SHADER_STAGE_INTERSECTION: { shaderStage = VK_SHADER_STAGE_INTERSECTION_BIT_NV; } break;
		case shaderStage_t::SHADER_STAGE_CALLABLE: { shaderStage = VK_SHADER_STAGE_CALLABLE_BIT_NV; } break;
		case shaderStage_t::SHADER_STAGE_TASK: { shaderStage = VK_SHADER_STAGE_TASK_BIT_NV; } break;
		case shaderStage_t::SHADER_STAGE_MESH: { shaderStage = VK_SHADER_STAGE_MESH_BIT_NV; } break;
	}

	const bool retVal = GLSLtoSPV( shaderStage, glslCode, spirvCode );
	if ( !retVal ) {
		printf( "Unable to convert glsl to spirv!\n" );
		assert( 0 );
		return false;
	}

	glslang::FinalizeProcess();
	return true;
}
