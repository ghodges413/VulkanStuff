//
//  shader.h
//
#include <vulkan/vulkan.h>
#include <vector>

/*
====================================================
Shader
====================================================
*/
class Shader {
public:
	enum shaderStage_t {
		SHADER_STAGE_VERTEX,
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
		SHADER_STAGE_TASK,
		SHADER_STAGE_MESH,
	};

public:
	Shader() {}
	~Shader() {}

	static bool CompileToSPIRV( const shaderStage_t stage, const char * glslCode, std::vector< unsigned int > & spirvCode );
	static VkShaderModule CreateShaderModule( VkDevice vkDevice, const char * code, const int size );
};