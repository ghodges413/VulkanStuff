//
//  DrawOcean.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"
#include "Math/Bounds.h"

class Buffer;
struct RenderModel;

bool InitOceanSimulation( DeviceContext * device );
bool CleanupOceanSimulation( DeviceContext * device );
void UpdateOceanParms( DeviceContext * device, float dt_sec, Mat4 matView, Mat4 matProj, Vec3 camPos, Vec3 camLookAt );
void UpdateOceanSimulation( DrawParms_t & parms );
void DrawOcean( DrawParms_t & parms );
