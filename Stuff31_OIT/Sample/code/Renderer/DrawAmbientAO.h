//
//  DrawAmbientAO.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

bool InitAmbientAO( DeviceContext * device, int width, int height );
bool CleanupAmbientAO( DeviceContext * device );

void DrawAmbientAO( DrawParms_t & parms );
