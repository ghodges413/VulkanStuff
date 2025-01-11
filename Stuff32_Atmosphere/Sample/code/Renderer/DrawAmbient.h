//
//  DrawAmbient.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

bool InitAmbient( DeviceContext * device, int width, int height );
bool CleanupAmbient( DeviceContext * device );

void DrawAmbient( DrawParms_t & parms );
