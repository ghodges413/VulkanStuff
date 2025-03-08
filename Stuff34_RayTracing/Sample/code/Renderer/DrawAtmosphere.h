//
//  DrawAtmosphere.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"
#include "Math/Bounds.h"

class Buffer;
struct RenderModel;

bool InitAtmosphere( DeviceContext * device, int width, int height );
bool CleanupAtmosphere( DeviceContext * device );
void UpdateAtmosphere( DeviceContext * device, const Camera_t & camera, float dt_sec, Bounds mapBounds );
void UpdateSunShadowDescriptors( DrawParms_t & parms );
void UpdateSunShadow( DrawParms_t & parms );
void DrawAtmosphere( DrawParms_t & parms );
