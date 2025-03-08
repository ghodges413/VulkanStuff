//
//  DrawTAA.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"


bool InitTemporalAntiAliasing( DeviceContext * device, int width, int height );
void CleanupTemporalAntiAliasing( DeviceContext * device );
void CalculateVelocityBuffer( DrawParms_t & parms );
void DrawTemporalAntiAliasing( DrawParms_t & parms );
