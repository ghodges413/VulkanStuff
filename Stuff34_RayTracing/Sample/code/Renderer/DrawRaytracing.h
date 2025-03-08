//
//  DrawRaytracing.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

bool InitRaytracing( DeviceContext * device, int width, int height );
void CleanupRaytracing( DeviceContext * device );
void UpdateRaytracing( DeviceContext * device, const RenderModel * models, int numModels );
void DrawRaytracing( DrawParms_t & parms );
