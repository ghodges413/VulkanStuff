//
//  DrawLightProbe.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

bool InitLightProbe( DeviceContext * device, int width, int height );
bool CleanupLightProbe( DeviceContext * device );

void DrawLightProbe( DrawParms_t & parms );
