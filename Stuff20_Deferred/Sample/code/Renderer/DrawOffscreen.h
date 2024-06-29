//
//  DrawOffscreen.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

bool InitOffscreen( DeviceContext * device, int width, int height );
bool CleanupOffscreen( DeviceContext * device );
void DrawOffscreen( DrawParms_t & parms );


bool InitDepthPrePass( DeviceContext * device );
void CleanupDepthPrePass( DeviceContext * device );
void DrawDepthPrePass( DrawParms_t & parms );
