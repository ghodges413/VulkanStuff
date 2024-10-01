//
//  BuildSubsurface.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"
#include "Math/Bounds.h"

#define SUBSURFACE_MAGIC 5789
#define SUBSURFACE_VERSION 1

struct subsurfaceHeader_t {
	int magic;
	int version;

	int numRadii;
	int numAngles;

	float minRadius;
	float maxRadius;
};


void BuildSubsurfaceTable();