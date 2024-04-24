//
//  shader.h
//
#pragma once
#include <map>
#include "DeviceContext.h"
//#include "Pipeline.h"
//#include "Descriptor.h"
#include <vulkan/vulkan.h>

/*
====================================================
Shader
====================================================
*/
class Shader {
public:
	enum ShaderStage_t {
		SHADER_STAGE_TASK = 0,
		SHADER_STAGE_MESH,
		SHADER_STAGE_VERTEX,// = 0,
		SHADER_STAGE_TESSELLATION_CONTROL,
		SHADER_STAGE_TESSELLATION_EVALUATION,
		SHADER_STAGE_GEOMETRY,
		SHADER_STAGE_FRAGMENT,
		SHADER_STAGE_COMPUTE,
		SHADER_STAGE_RAYGEN,
		SHADER_STAGE_ANY_HIT,
		SHADER_STAGE_CLOSEST_HIT,
		SHADER_STAGE_MISS,
		SHADER_STAGE_INTERSECTION,
		SHADER_STAGE_CALLABLE,

		SHADER_STAGE_NUM,
	};

public:
	Shader();
	~Shader() {}

	bool Load( DeviceContext * device, const char * name );
	void Cleanup( DeviceContext * device );

	static bool LoadShader( DeviceContext * device, const char * path, VkShaderStageFlagBits flags, VkPipelineShaderStageCreateInfo & stageOut, VkShaderModule & shaderModuleOut );

private:
	void BuildShaderPipelineStages( DeviceContext * device );
	static bool CompileToSPIRV( const VkShaderStageFlagBits stage, const char * glslCode, std::vector< unsigned int > & spirvCode );
	static bool CompileToSPIRV( const ShaderStage_t stage, const char * glslCode, std::vector< unsigned int > & spirvCode );
	static VkShaderModule CreateShaderModule( VkDevice vkDevice, const char * code, const int size );

public:
	//
	//	Shader Modules
	//
	VkShaderModule m_vkShaderModules[ SHADER_STAGE_NUM ];

	std::vector< VkPipelineShaderStageCreateInfo > m_shaderStages;
	bool m_isMeshShader;
	bool m_isRtxShader;

	std::vector< VkRayTracingShaderGroupCreateInfoNV > m_rtxShaderGroups;	// Only used by ray tracing shaders
};

/*
====================================================
ShaderManager
====================================================
*/
class ShaderManager {
public:
	explicit ShaderManager( DeviceContext * device );
	~ShaderManager();

	Shader * GetShader( const char * name );

private:
	typedef std::map< std::string, Shader * > Shaders_t;
	Shaders_t	m_shaders;

	DeviceContext * m_device;
};

extern ShaderManager * g_shaderManager;