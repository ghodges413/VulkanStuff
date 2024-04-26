//
//  Shader.cpp
//
#include "Graphics/Shader.h"
#include "Miscellaneous/Fileio.h"
#include <assert.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <string.h>

#include "Graphics/FrameBuffer.h"

ShaderManager * g_shaderManager = NULL;

/*
========================================================================================================

GLSlang related

========================================================================================================
*/

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
========================================================================================================

Shader

========================================================================================================
*/

/*
====================================================
Shader::Shader
====================================================
*/
Shader::Shader() {
	memset( m_vkShaderModules, 0, sizeof( VkShaderModule ) * SHADER_STAGE_NUM );
	m_isMeshShader = false;
	m_isRtxShader = false;
}

/*
====================================================
Shader::Load
====================================================
*/
bool Shader::Load( DeviceContext * device, const char * name ) {
	const char * fileExtensions[ SHADER_STAGE_NUM ];
	fileExtensions[ SHADER_STAGE_TASK ]						= "task";
	fileExtensions[ SHADER_STAGE_MESH ]						= "mesh";
	fileExtensions[ SHADER_STAGE_VERTEX ]					= "vert";
	fileExtensions[ SHADER_STAGE_TESSELLATION_CONTROL ]		= "tess";
	fileExtensions[ SHADER_STAGE_TESSELLATION_EVALUATION ]	= "tval";
	fileExtensions[ SHADER_STAGE_GEOMETRY ]					= "geom";
	fileExtensions[ SHADER_STAGE_FRAGMENT ]					= "frag";
	fileExtensions[ SHADER_STAGE_COMPUTE ]					= "comp";
	fileExtensions[ SHADER_STAGE_RAYGEN ]					= "rgen";
	fileExtensions[ SHADER_STAGE_ANY_HIT ]					= "ahit";
	fileExtensions[ SHADER_STAGE_CLOSEST_HIT ]				= "chit";
	fileExtensions[ SHADER_STAGE_MISS ]						= "miss";
	fileExtensions[ SHADER_STAGE_INTERSECTION ]				= "rint";
	fileExtensions[ SHADER_STAGE_CALLABLE ]					= "call";

	bool hasShaderModules = false;

	for ( int i = 0; i < SHADER_STAGE_NUM; i++ ) {
		unsigned char * code = NULL;
		unsigned int size = 0;

		// Get the spirv shader name
		char nameSpirv[ 1024 ];
		sprintf_s( nameSpirv, 1024, "../../common/data/shaders/spirv/%s.%s.spirv", name, fileExtensions[ i ] );

		// Get the ascii shader name
		char nameShader[ 1024 ];
		sprintf_s( nameShader, 1024, "../../common/data/shaders/%s.%s", name, fileExtensions[ i ] );

		//
		// Try loading the spirv code first
		//

		// Get the last write time stamp first
		int lastWriteTimeSpirv = GetFileTimeStampWrite( nameSpirv );
		int lastWriteTimeAscii = GetFileTimeStampWrite( nameShader );

		// If the spirv is newer than the ascii, load the spirv
		if ( lastWriteTimeSpirv > lastWriteTimeAscii ) {
			if ( GetFileData( nameSpirv, &code, size ) ) {
				m_vkShaderModules[ i ] = Shader::CreateShaderModule( device->m_vkDevice, (char*)code, size );

				// We've found at least one shader module
				hasShaderModules = true;
				continue;
			}
		}

		//
		// SPIRV wasn't found (or is outdated), try loading the glsl
		//
		if ( !GetFileData( nameShader, &code, size ) ) {
			continue;
		}

		// Compile to spirv
		std::vector< unsigned int > spirv;
		if ( !Shader::CompileToSPIRV( (ShaderStage_t)i, (char *)code, spirv ) ) {
			assert( 0 );
			free( code );
			return false;
		}
		free( code );

		code = (unsigned char *)spirv.data();
		size = (unsigned int)( spirv.size() * sizeof( unsigned int ) );

		// Save the spirv to file
		SaveFileData( nameSpirv, code, size );

		// Create a shader module from the spirv
		m_vkShaderModules[ i ] = Shader::CreateShaderModule( device->m_vkDevice, (char*)code, size );

		// We've built at least one shader module
		hasShaderModules = true;
	}

	if ( hasShaderModules ) {
		BuildShaderPipelineStages( device );
	}

	return hasShaderModules;
}

/*
====================================================
Shader::BuildShaderPipelineStages
====================================================
*/
void Shader::BuildShaderPipelineStages( DeviceContext * device ) {
	m_shaderStages.clear();

	for ( int i = 0; i < Shader::SHADER_STAGE_NUM; i++ ) {
		if ( NULL == m_vkShaderModules[ i ] ) {
			continue;
		}

		VkShaderStageFlagBits stage;
		switch ( i ) {
			default:
			case Shader::SHADER_STAGE_TASK: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_TASK_BIT_NV; } break;
			case Shader::SHADER_STAGE_MESH: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_MESH_BIT_NV; m_isMeshShader = true; } break;
			case Shader::SHADER_STAGE_VERTEX: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT; } break;
			case Shader::SHADER_STAGE_TESSELLATION_CONTROL: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; } break;
			case Shader::SHADER_STAGE_TESSELLATION_EVALUATION: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; } break;
			case Shader::SHADER_STAGE_GEOMETRY: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT; } break;
			case Shader::SHADER_STAGE_FRAGMENT: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT; } break;
			case Shader::SHADER_STAGE_COMPUTE: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT; } break;
			case Shader::SHADER_STAGE_RAYGEN: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_NV; m_isRtxShader = true; } break;
			case Shader::SHADER_STAGE_ANY_HIT: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_NV; } break;
			case Shader::SHADER_STAGE_CLOSEST_HIT: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV; } break;
			case Shader::SHADER_STAGE_MISS: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_NV; } break;
			case Shader::SHADER_STAGE_INTERSECTION: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_NV; } break;
			case Shader::SHADER_STAGE_CALLABLE: { stage = VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_NV; } break;			
		}

		VkPipelineShaderStageCreateInfo shaderStageInfo = {};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = stage;
		shaderStageInfo.module = m_vkShaderModules[ i ];
		shaderStageInfo.pName = "main";

		m_shaderStages.push_back( shaderStageInfo );
	}


	//
	//	Setup ray tracing shader groups
	//
	if ( m_isRtxShader ) {		
		m_rtxShaderGroups.clear();
		for ( int i = 0; i < m_shaderStages.size(); i++ ) {
			VkRayTracingShaderGroupCreateInfoNV group = {};
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
			group.generalShader = VK_SHADER_UNUSED_NV;
			group.closestHitShader = VK_SHADER_UNUSED_NV;
			group.anyHitShader = VK_SHADER_UNUSED_NV;
			group.intersectionShader = VK_SHADER_UNUSED_NV;

			const VkPipelineShaderStageCreateInfo & stage = m_shaderStages[ i ];
			switch ( stage.stage ) {
				case VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_NV: { group.generalShader = i; } break;
				case VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_NV: { group.anyHitShader = i; group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV; } break;
				case VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV: { group.closestHitShader = i; group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV; } break;
				case VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_NV: { group.generalShader = i; } break;
				case VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_NV: { group.intersectionShader = i; group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV; } break;
				case VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_NV: { group.generalShader = i; } break;		
			};

			m_rtxShaderGroups.push_back( group );
		}
	}
}

/*
====================================================
Shader::Cleanup
====================================================
*/
void Shader::Cleanup( DeviceContext * device ) {
	for ( int i = 0; i < SHADER_STAGE_NUM; i++ ) {
		if ( NULL != m_vkShaderModules[ i ] ) {
			vkDestroyShaderModule( device->m_vkDevice, m_vkShaderModules[ i ], nullptr );
		}
		m_vkShaderModules[ i ] = NULL;
	}
}

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
Shader::CompileToSPIRV
====================================================
*/
bool Shader::CompileToSPIRV( const ShaderStage_t stage, const char * glslCode, std::vector< unsigned int > & spirvCode ) {
	glslang::InitializeProcess();

	VkShaderStageFlagBits shaderStage;
	switch ( stage ) {
		default:
		case ShaderStage_t::SHADER_STAGE_VERTEX: { shaderStage = VK_SHADER_STAGE_VERTEX_BIT; } break;
		case ShaderStage_t::SHADER_STAGE_TESSELLATION_CONTROL: { shaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; } break;
		case ShaderStage_t::SHADER_STAGE_TESSELLATION_EVALUATION: { shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; } break;
		case ShaderStage_t::SHADER_STAGE_GEOMETRY: { shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT; } break;
		case ShaderStage_t::SHADER_STAGE_FRAGMENT: { shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT; } break;
		case ShaderStage_t::SHADER_STAGE_COMPUTE: { shaderStage = VK_SHADER_STAGE_COMPUTE_BIT; } break;
		case ShaderStage_t::SHADER_STAGE_RAYGEN: { shaderStage = VK_SHADER_STAGE_RAYGEN_BIT_NV; } break;
		case ShaderStage_t::SHADER_STAGE_ANY_HIT: { shaderStage = VK_SHADER_STAGE_ANY_HIT_BIT_NV; } break;
		case ShaderStage_t::SHADER_STAGE_CLOSEST_HIT: { shaderStage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV; } break;
		case ShaderStage_t::SHADER_STAGE_MISS: { shaderStage = VK_SHADER_STAGE_MISS_BIT_NV; } break;
		case ShaderStage_t::SHADER_STAGE_INTERSECTION: { shaderStage = VK_SHADER_STAGE_INTERSECTION_BIT_NV; } break;
		case ShaderStage_t::SHADER_STAGE_CALLABLE: { shaderStage = VK_SHADER_STAGE_CALLABLE_BIT_NV; } break;
		case ShaderStage_t::SHADER_STAGE_TASK: { shaderStage = VK_SHADER_STAGE_TASK_BIT_NV; } break;
		case ShaderStage_t::SHADER_STAGE_MESH: { shaderStage = VK_SHADER_STAGE_MESH_BIT_NV; } break;
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

/*
====================================================
Shader::CompileToSPIRV
====================================================
*/
bool Shader::CompileToSPIRV( const VkShaderStageFlagBits stage, const char * glslCode, std::vector< unsigned int > & spirvCode ) {
	glslang::InitializeProcess();

	const bool retVal = GLSLtoSPV( stage, glslCode, spirvCode );
	if ( !retVal ) {
		printf( "Unable to convert glsl to spirv!\n" );
		assert( 0 );
		return false;
	}

	glslang::FinalizeProcess();
	return true;
}

/*
====================================================
Shader::LoadShader
====================================================
*/
bool Shader::LoadShader( DeviceContext * device, const char * name, VkShaderStageFlagBits stageFlags, VkPipelineShaderStageCreateInfo & stageOut, VkShaderModule & shaderModuleOut ) {
	unsigned char * code = NULL;
	unsigned int size = 0;

	// Try loading the spirv code first
	char nameSpirv[ 1024 ];
	sprintf_s( nameSpirv, 1024, "../../common/data/shaders/spirv/%s.spirv", name );
	if ( GetFileData( nameSpirv, &code, size ) ) {
		shaderModuleOut = Shader::CreateShaderModule( device->m_vkDevice, (char*)code, size );

		memset( &stageOut, 0, sizeof( stageOut ) );
		stageOut.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageOut.stage = stageFlags;
		stageOut.module = shaderModuleOut;
		stageOut.pName = "main";
		return true;
	}

	char shaderName[ 1024 ];
	sprintf_s( shaderName, 1024, "../../common/data/shaders/%s", name );
	if ( !GetFileData( shaderName, &code, size ) ) {
		return false;
	}

	std::vector< unsigned int > spirv;
	if ( !Shader::CompileToSPIRV( stageFlags, (char *)code, spirv ) ) {
		free( code );
		return false;
	}

	code = (unsigned char *)spirv.data();
	size = (unsigned int)( spirv.size() * sizeof( unsigned int ) );

	// Save the spirv to file
	SaveFileData( nameSpirv, code, size );

	// Create a shader module from the spirv
	shaderModuleOut = Shader::CreateShaderModule( device->m_vkDevice, (char*)code, size );

	memset( &stageOut, 0, sizeof( stageOut ) );
	stageOut.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageOut.stage = stageFlags;
	stageOut.module = shaderModuleOut;
	stageOut.pName = "main";
	return true;
}

/*
========================================================================================================

ShaderManager

========================================================================================================
*/

/*
====================================================
ShaderManager::ShaderManager
====================================================
*/
ShaderManager::ShaderManager( DeviceContext * device ) {
	m_device = device;
}

/*
====================================================
ShaderManager::~ShaderManager
====================================================
*/
ShaderManager::~ShaderManager() {
	Shaders_t::iterator it = m_shaders.begin();
	while ( it != m_shaders.end() ) {
		it->second->Cleanup( m_device );

		delete it->second;
		it->second = NULL;
		
		++it;
	}
	m_shaders.clear();
}

/*
====================================================
ShaderManager::GetShader
====================================================
*/
Shader * ShaderManager::GetShader( const char * name ) {
	assert( NULL != name );
	std::string nameStr = name;
	//nameStr.ToLower();
    
	Shaders_t::iterator it = m_shaders.find( nameStr.data() );
	if ( it != m_shaders.end() ) {
		// shader found!  return it!
        assert( NULL != it->second );
		return it->second;
	}

    Shader * shader = new Shader();
	const bool result = shader->Load( m_device, name );
    assert( result );
	if ( false == result ) {
        // shader not found
		printf( "Unable to Load/Find shader: %s\n", name );
        delete shader;
        return NULL;
	}
	
	// store shader and return
	m_shaders[ nameStr ] = shader;
	return shader;
}