//
//  DrawTransparent.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"


bool InitTransparent( DeviceContext * device, int width, int height );
void CleanupTransparent( DeviceContext * device );
void DrawTransparent( DrawParms_t & parms );
