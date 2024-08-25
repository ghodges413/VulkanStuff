//
//  BuildLightTiles.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

struct storageLight_t {
	Vec4	m_sphere;
	Vec4	m_color;
};

struct buildLightTileParms_t {
	Mat4 matView;
	Mat4 matProjInv;

	int screenWidth;
	int screenHeight;
	int maxLights;
	int pad0;
};

bool InitLightTiles( DeviceContext * device, int width, int height );
void CleanupLightTiles( DeviceContext * device );

void UpdateLights( DeviceContext * device, const storageLight_t * lights, int numLights, const buildLightTileParms_t & buildParms );
void BuildLightTiles( DrawParms_t & parms );









bool InitDebugDrawLightTiles( DeviceContext * device, int width, int height );
void CleanupDebugDrawLightTiles( DeviceContext * device );
void DebugDrawLightTiles( DrawParms_t & parms );


bool InitDrawTiledGGX( DeviceContext * device, int width, int height );
void CleanupDrawTiledGGX( DeviceContext * device );
void DrawTiledGGX( DrawParms_t & parms );
