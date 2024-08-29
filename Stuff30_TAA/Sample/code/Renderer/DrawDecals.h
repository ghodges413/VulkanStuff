//
//  DrawDecals.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"


bool InitDecals( DeviceContext * device );
void CleanupDecals( DeviceContext * device );
void DrawDecals( DrawParms_t & parms );
