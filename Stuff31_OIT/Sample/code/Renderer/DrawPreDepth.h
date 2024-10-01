//
//  DrawGBuffer.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

bool InitPreDepth( DeviceContext * device, int width, int height );
void CleanupPreDepth( DeviceContext * device );
void DrawPreDepth( DrawParms_t & parms );