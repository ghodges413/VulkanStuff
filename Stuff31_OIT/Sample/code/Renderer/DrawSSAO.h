//
//  DrawSSAO.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"


bool InitSSAO( DeviceContext * device, int width, int height );
void CleanupSSAO( DeviceContext * device );
void DrawSSAO( DrawParms_t & parms );
