//
//  Pipeline.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Graphics/Descriptor.h"
#include "Graphics/Buffer.h"

class DeviceContext;
class FrameBuffer;
class Descriptors;
class Shader;

/*
====================================================
Pipeline

Think of this as all of the state that's used to draw
====================================================
*/
class Pipeline {
public:
	Pipeline() { m_vkPipeline = 0; m_vkPipelineLayout = 0; }
	~Pipeline() {}

	enum CullMode_t {
		CULL_MODE_FRONT,
		CULL_MODE_BACK,
		CULL_MODE_NONE
	};
	enum FillMode_t {
		FILL_MODE_FILL,
		FILL_MODE_LINE,
		FILL_MODE_POINT		
	};
	enum BlendMode_t {
		BLEND_MODE_NONE,
		BLEND_MODE_ALPHA,
		BLEND_MODE_ADDITIVE,
	};

	struct CreateParms_t {
		CreateParms_t() {
			memset( this, 0, sizeof( CreateParms_t ) );
		}
		VkRenderPass	renderPass;
		FrameBuffer * framebuffer;	// We only need the renderpass in the framebuffer... we don't actually need the frambebuffer
		Descriptors * descriptors;
		Shader * shader;

		int width;
		int height;

		// TODO: Add more state (blending, depth, fillmode, etc)
		CullMode_t cullMode;
		FillMode_t fillMode;

		bool depthTest;
		bool depthWrite;
		BlendMode_t blendMode;

		int pushConstantSize;
		VkShaderStageFlagBits pushConstantShaderStages;

		VkVertexInputAttributeDescription * vertexAttributeDescriptions;
		int numVertexAttributeDescriptions;
		int vertexAttributeStride;
	};
	bool Create( DeviceContext * device, const CreateParms_t & parms );
	bool CreateCompute( DeviceContext * device, const CreateParms_t & parms );
	bool CreateRayTracing( DeviceContext * device, const CreateParms_t & parms );
	void Cleanup( DeviceContext * device );

	Descriptor GetFreeDescriptor() { return m_parms.descriptors->GetFreeDescriptor(); }
	Descriptor GetRTXDescriptor() { return m_parms.descriptors->GetRTXDescriptor(); }

	void BindPipeline( VkCommandBuffer cmdBuffer );
	void BindPipelineCompute( VkCommandBuffer cmdBuffer );
	void BindPipelineRayTracing( VkCommandBuffer cmdBuffer );
	void BindPushConstant( VkCommandBuffer cmdBuffer, const int offset, const int size, const void * data );
	void DispatchCompute( VkCommandBuffer cmdBuffer, int groupCountX, int groupCountY, int groupCountZ );
	void TraceRays( VkCommandBuffer cmdBuffer );

	CreateParms_t m_parms;

	//
	//	PipelineState
	//
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkPipeline;
	//VkPipelineCache m_vkPipelineCache;	// I don't think we'll ever use this, it's for saving pipelines between runs of the application



	// This should probably be a part of the device class.  So that we can just globally access stored properties later.
	VkPhysicalDeviceRayTracingPropertiesNV m_rayTracingProperties;
	Buffer m_shaderBindingTable;
};