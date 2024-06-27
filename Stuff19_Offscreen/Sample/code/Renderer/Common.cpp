//
//  Common.cpp
//
#include "Renderer/Common.h"
#include "Models/ModelStatic.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"

FrameBuffer		g_offscreenFrameBuffer;
