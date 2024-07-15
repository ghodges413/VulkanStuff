//
//  Pipeline.cpp
//
#include "Graphics/Pipeline.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/Shader.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Descriptor.h"
#include "Models/ModelStatic.h"
#include <assert.h>

#pragma optimize( "", off )

/*
========================================================================================================

Pipeline

========================================================================================================
*/

/*
====================================================
Pipeline::Create
====================================================
*/
bool Pipeline::Create( DeviceContext * device, const CreateParms_t & parms ) {
	VkResult result;

	m_parms = parms;

	const int width = parms.width;
	const int height = parms.height;

	const bool isMeshShader = parms.shader->m_isMeshShader;
	std::vector< VkPipelineShaderStageCreateInfo > shaderStages = parms.shader->m_shaderStages;


	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	if ( !isMeshShader ) {	// Mesh Shaders don't use the input assembler, and therefore don't pass in vertices through the traditional pipe.
		if ( NULL != parms.vertexAttributeDescriptions ) {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = parms.vertexAttributeStride;
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.vertexAttributeDescriptionCount = parms.numVertexAttributeDescriptions;
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions = parms.vertexAttributeDescriptions;
		} else {
			VkVertexInputBindingDescription bindingDescription = vert_t::GetBindingDescription();
			std::array< VkVertexInputAttributeDescription, 5 > attributeDescriptions = vert_t::GetAttributeDescriptions();

			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast< uint32_t >( attributeDescriptions.size() );
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();		
		}
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)width;
	viewport.height = (float)height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { (uint32_t)width, (uint32_t)height };

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	if ( FILL_MODE_LINE == parms.fillMode ) {
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	} else if ( FILL_MODE_POINT == parms.fillMode ) {
		rasterizer.polygonMode = VK_POLYGON_MODE_POINT;
	}
	rasterizer.lineWidth = 1.0f;

	if ( CULL_MODE_FRONT == parms.cullMode ) {
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	} else if ( CULL_MODE_BACK == parms.cullMode ) {
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	} else {
		rasterizer.cullMode = VK_CULL_MODE_NONE;
	}

	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = parms.depthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = parms.depthWrite ? VK_TRUE : VK_FALSE;
	//depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	// This is for MRT.  If a framebuffer has multiple render targets and but doesn't setup an equal number of
	// blend attachment states.  Then only the first color attachment will be rendered into.
	VkPipelineColorBlendAttachmentState colorBlendAttachments[ 10 ];
	for ( int i = 0; i < m_parms.numColorAttachments; i++ ) {
		colorBlendAttachments[ i ].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachments[ i ].blendEnable = VK_FALSE;
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	if ( m_parms.numColorAttachments > 0 ) {
		colorBlending.attachmentCount = m_parms.numColorAttachments;
		colorBlending.pAttachments = colorBlendAttachments;
	}
	colorBlending.blendConstants[ 0 ] = 0.0f;
	colorBlending.blendConstants[ 1 ] = 0.0f;
	colorBlending.blendConstants[ 2 ] = 0.0f;
	colorBlending.blendConstants[ 3 ] = 0.0f;

	colorBlendAttachment.blendEnable = VK_FALSE;
	if ( parms.blendMode == BLEND_MODE_ALPHA ) {
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	} else if ( parms.blendMode == BLEND_MODE_ADDITIVE ) {
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	// Check for push constants
	VkPushConstantRange pushConstantRange = {};
	if ( parms.pushConstantSize > 0 ) {
		//m_parms.pushConstantShaderStages = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.stageFlags = parms.pushConstantShaderStages;
		pushConstantRange.size = parms.pushConstantSize;
		pushConstantRange.offset = 0;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &parms.descriptors->m_vkDescriptorSetLayout;
	if ( parms.pushConstantSize > 0 ) {
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	}


	result = vkCreatePipelineLayout( device->m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create pipeline layout\n" );
		assert( 0 );
		return false;
	}

	VkDynamicState dynamicSate[ 3 ] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS
	};



	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = 3;
	dynamicState.pDynamicStates = dynamicSate;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = (uint32_t)shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_vkPipelineLayout;
	if ( NULL == parms.framebuffer ) {
		pipelineInfo.renderPass = parms.renderPass;
	} else {
		pipelineInfo.renderPass = parms.framebuffer->m_vkRenderPass;
	}
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines( device->m_vkDevice, pipelineCache, 1, &pipelineInfo, nullptr, &m_vkPipeline );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create pipeline\n" );
		assert( 0 );
		return false;
	}

	return true;
}

/*
====================================================
Pipeline::CreateCompute
====================================================
*/
bool Pipeline::CreateCompute( DeviceContext * device, const CreateParms_t & parms ) {
	VkResult result;

	m_parms = parms;

	VkPipelineShaderStageCreateInfo shaderStageInfo = {};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = parms.shader->m_vkShaderModules[ Shader::SHADER_STAGE_COMPUTE ];
	shaderStageInfo.pName = "main";

	// Check for push constants
	VkPushConstantRange pushConstantRange = {};
	if ( parms.pushConstantSize > 0 ) {
		m_parms.pushConstantShaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.size = parms.pushConstantSize;
		pushConstantRange.offset = 0;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &parms.descriptors->m_vkDescriptorSetLayout;
	if ( parms.pushConstantSize > 0 ) {
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	}

	result = vkCreatePipelineLayout( device->m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create pipeline layout\n" );
		assert( 0 );
		return false;
	}

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStageInfo;
	pipelineInfo.layout = m_vkPipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	
	

	result = vkCreateComputePipelines( device->m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkPipeline );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create pipeline\n" );
		assert( 0 );
		return false;
	}

	return true;
}

/*
====================================================
Pipeline::CreateRayTracing
====================================================
*/
#define INDEX_RAYGEN 0
#define INDEX_CLOSEST_HIT 1
#define INDEX_MISS 2
bool Pipeline::CreateRayTracing( DeviceContext * device, const CreateParms_t & parms ) {
	VkResult result;

	m_parms = parms;

	// Check for push constants
	VkPushConstantRange pushConstantRange = {};
	if ( parms.pushConstantSize > 0 ) {
		pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;
		pushConstantRange.size = parms.pushConstantSize;
		pushConstantRange.offset = 0;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &parms.descriptors->m_vkDescriptorSetLayout;
	if ( parms.pushConstantSize > 0 ) {
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	}

	result = vkCreatePipelineLayout( device->m_vkDevice, &pipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Unable to create ray tracing pipeline\n" );
		assert( 0 );
		return false;
	}

	std::vector< VkPipelineShaderStageCreateInfo > shaderStages = parms.shader->m_shaderStages;
	std::vector< VkRayTracingShaderGroupCreateInfoNV > groups = parms.shader->m_rtxShaderGroups;

	VkRayTracingPipelineCreateInfoNV rayPipelineInfo = {};
	rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	rayPipelineInfo.stageCount = (uint32_t)shaderStages.size();
	rayPipelineInfo.pStages = shaderStages.data();
	rayPipelineInfo.groupCount = (uint32_t)groups.size();
	rayPipelineInfo.pGroups = groups.data();
	rayPipelineInfo.maxRecursionDepth = 1;
	rayPipelineInfo.layout = m_vkPipelineLayout;
	result = vfs::vkCreateRayTracingPipelinesNV( device->m_vkDevice, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_vkPipeline );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Unable to create ray tracing pipeline\n" );
		assert( 0 );
		return false;
	}

	//
	//	Shader Binding Table
	//
	{
		// Query the ray tracing properties of the current implementation, we will need them later on
		{
			memset( &m_rayTracingProperties, 0, sizeof( m_rayTracingProperties ) );
			m_rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
			VkPhysicalDeviceProperties2 deviceProps2{};
			deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProps2.pNext = &m_rayTracingProperties;
			vkGetPhysicalDeviceProperties2( device->m_vkPhysicalDevice, &deviceProps2 );
		}

		const uint32_t numShaders = (uint32_t)shaderStages.size();
		const uint32_t shaderGroupHandleSize = m_rayTracingProperties.shaderGroupHandleSize;
		const uint32_t shaderGroupBaseAlignment = m_rayTracingProperties.shaderGroupBaseAlignment;
		//const uint32_t storageBufferSize = shaderGroupHandleSize * numShaders;
		const uint32_t storageBufferSize = shaderGroupBaseAlignment * numShaders;
		if ( !m_shaderBindingTable.Allocate( device, NULL, storageBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
			printf( "Unable to allocate shader binding table\n" );
			assert( 0 );
			return false;
		}

		unsigned char * shaderHandleStorage = (unsigned char *)alloca( shaderGroupHandleSize * numShaders );

		// Get shader identifiers
		result = vfs::vkGetRayTracingShaderGroupHandlesNV( device->m_vkDevice, m_vkPipeline, 0, groups.size(), shaderGroupHandleSize * numShaders, shaderHandleStorage );
		if ( VK_SUCCESS != result ) {
			printf( "ERROR: vkGetRayTracingShaderGroupHandlesNV failed\n" );
			assert( 0 );
			return false;
		}

		// Copy the shader identifiers to the shader binding table
		unsigned char * data = (unsigned char*)m_shaderBindingTable.MapBuffer( device );
		for ( int i = 0; i < numShaders; i++ ) {
			memcpy( data, shaderHandleStorage + i * shaderGroupHandleSize, shaderGroupHandleSize );
			data += shaderGroupBaseAlignment;
		}
		m_shaderBindingTable.UnmapBuffer( device );
	}

	return true;
}

/*
====================================================
Pipeline::Cleanup
====================================================
*/
void Pipeline::Cleanup( DeviceContext * device ) {
	if ( m_vkPipeline > 0 ) {
		vkDestroyPipeline( device->m_vkDevice, m_vkPipeline, nullptr );
		m_vkPipeline = 0;
	}
	if ( m_vkPipelineLayout > 0 ) {
		vkDestroyPipelineLayout( device->m_vkDevice, m_vkPipelineLayout, nullptr );
		m_vkPipelineLayout = 0;
	}

	m_shaderBindingTable.Cleanup( device );
}

/*
====================================================
Pipeline::BindPipeline
====================================================
*/
void Pipeline::BindPipeline( VkCommandBuffer cmdBuffer ) {
	vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline );
}
void Pipeline::BindPipelineCompute( VkCommandBuffer cmdBuffer ) {
	vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_vkPipeline );
}
void Pipeline::BindPipelineRayTracing( VkCommandBuffer cmdBuffer ) {
	vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_vkPipeline );
}

/*
====================================================
Pipeline::BindPushConstant
====================================================
*/
void Pipeline::BindPushConstant( VkCommandBuffer cmdBuffer, const int offset, const int size, const void * data ) {
	vkCmdPushConstants(
		cmdBuffer,
		m_vkPipelineLayout,
		m_parms.pushConstantShaderStages,
		offset,
		size,
		data
	);
}

/*
====================================================
Pipeline::DispatchCompute
====================================================
*/
void Pipeline::DispatchCompute( VkCommandBuffer cmdBuffer, int groupCountX, int groupCountY, int groupCountZ ) {
	vkCmdDispatch( cmdBuffer, groupCountX, groupCountY, groupCountZ );
}

/*
====================================================
Pipeline::TraceRays
====================================================
*/
void Pipeline::TraceRays( VkCommandBuffer cmdBuffer ) {
	// Calculate shader binding offsets, which is pretty straight forward in our example 
	VkDeviceSize rayGenOffset		= m_rayTracingProperties.shaderGroupBaseAlignment * INDEX_RAYGEN;
	VkDeviceSize hitShaderOffset	= m_rayTracingProperties.shaderGroupBaseAlignment * INDEX_CLOSEST_HIT;
	VkDeviceSize missShaderOffset	= m_rayTracingProperties.shaderGroupBaseAlignment * INDEX_MISS;
	VkDeviceSize stride				= m_rayTracingProperties.shaderGroupHandleSize;

	const int depth = 1;

	vfs::vkCmdTraceRaysNV(
		cmdBuffer,
		m_shaderBindingTable.m_vkBuffer, rayGenOffset,				// ray gen shader
		m_shaderBindingTable.m_vkBuffer, missShaderOffset, stride,	// miss shader
		m_shaderBindingTable.m_vkBuffer, hitShaderOffset, stride,	// hit shader
		VK_NULL_HANDLE, 0, 0,										// callable shader
		m_parms.width, m_parms.height, depth						// storage image dimensions
	);
}