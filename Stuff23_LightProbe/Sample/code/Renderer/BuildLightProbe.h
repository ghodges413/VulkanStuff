//
//  BuildLightProbe.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"


void BuildLightProbe( DeviceContext * device, FrameBuffer & frameBuffer );

void FrameBufferCapture( DeviceContext * device, FrameBuffer & frameBuffer );
void BuildLightProbe();