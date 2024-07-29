//
//  DrawSSR.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"


bool InitScreenSpaceReflections( DeviceContext * device, int width, int height );
void CleanupScreenSpaceReflections( DeviceContext * device );
void DrawScreenSpaceReflections( DrawParms_t & parms );
