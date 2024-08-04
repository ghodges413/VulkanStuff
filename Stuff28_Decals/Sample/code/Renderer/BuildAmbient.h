//
//  BuildAmbient.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"
#include "Math/Bounds.h"

#define AMBIENT_MAGIC 42069420
#define AMBIENT_VERSION 1

struct ambientHeader_t {
	int magic;
	int version;
	int numX;
	int numY;
	int numZ;
	Bounds bounds;
};

struct legendrePolynomials_t {
	Vec3 mL00;	// stored in rgb
	Vec3 mL11;
	Vec3 mL10;
	Vec3 mL1n1;
	Vec3 mL22;
	Vec3 mL21;
	Vec3 mL20;
	Vec3 mL2n1;
	Vec3 mL2n2;
};

// #define AMBIENT_NODES_X 128
// #define AMBIENT_NODES_Y 128
// #define AMBIENT_NODES_Z 32

#define AMBIENT_NODES_X 32
#define AMBIENT_NODES_Y 32
#define AMBIENT_NODES_Z 16

void BuildAmbient( DeviceContext * device, FrameBuffer & frameBuffer );

void FrameBufferCaptureAmbient( DeviceContext * device, FrameBuffer & frameBuffer );
void BuildAmbient();

Vec3 GetAmbientPos();
Vec3 GetAmbientLookAt();