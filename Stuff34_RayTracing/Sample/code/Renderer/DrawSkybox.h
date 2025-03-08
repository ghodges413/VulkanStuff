//
//  DrawSkybox.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

bool InitSkybox( DeviceContext * device, int width, int height );
bool CleanupSkybox( DeviceContext * device );

void DrawSkybox( DrawParms_t & parms );
