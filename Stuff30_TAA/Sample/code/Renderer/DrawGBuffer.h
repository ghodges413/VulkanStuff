//
//  DrawGBuffer.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

bool InitGBuffer( DeviceContext * device, int width, int height );
void CleanupGBuffer( DeviceContext * device );
void DrawGBuffer( DrawParms_t & parms );
void DrawPreDepth( DrawParms_t & parms );