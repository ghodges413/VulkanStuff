//
//  Common.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Graphics/Image.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Shader.h"
#include "Graphics/Descriptor.h"
#include "Graphics/FrameBuffer.h"

class Buffer;
struct RenderModel;

struct DrawParms_t {
	DeviceContext * device;
	int cmdBufferIndex;
	Buffer * uniforms;
	RenderModel * renderModels;
	int numModels;
	float time;
};


extern FrameBuffer	g_offscreenFrameBuffer;
