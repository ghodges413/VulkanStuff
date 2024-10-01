//
//  Samplers.h
//
#include <vulkan/vulkan.h>
#include "Graphics/DeviceContext.h"

/*
====================================================
Samplers
====================================================
*/
class Samplers {
public:
	static bool InitializeSamplers( DeviceContext * device );
	static void Cleanup( DeviceContext * device );

	static VkSampler m_samplerStandard;
	static VkSampler m_samplerRepeat;
	static VkSampler m_samplerDepth;
	static VkSampler m_samplerNearest;
	static VkSampler m_samplerLightProbe;
};
