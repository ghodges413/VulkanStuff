//
//  DrawSubsurface.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"


bool InitSubsurface( DeviceContext * device, int width, int height );
void CleanupSubsurface( DeviceContext * device );
void DrawSubsurface( DrawParms_t & parms );
