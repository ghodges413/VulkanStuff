//
//  AccelerationStructure.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "../Math/Matrix.h"
#include "../Math/Quat.h"
#include "Buffer.h"

class DeviceContext;
class Texture;
class Pipeline;
class Model;
struct RenderModel;

/*
====================================================
AccelerationStructure
====================================================
*/
class AccelerationStructure {
public:
	AccelerationStructure() {}
	~AccelerationStructure() {}

	// Ray tracing acceleration structure
	struct structure {
		structure() {
			memory = 0;
			accelerationStructure = 0;
			handle = 0;
		}
		VkDeviceMemory memory;
		VkAccelerationStructureNV accelerationStructure;
		uint64_t handle;
	};

	// Ray tracing geometry instance
	struct GeometryInstance {
		//Mat4 transform;
		float transform[ 3 * 4 ];
		uint32_t instanceId : 24;
		uint32_t mask : 8;
		uint32_t instanceOffset : 24;
		uint32_t flags : 8;
		uint64_t accelerationStructureHandle;
	};

	structure m_bottomLevel;
	structure m_topLevel;

	Buffer m_instanceBuffer;

	bool CreateBottomLevel( DeviceContext * device, const VkGeometryNV * geometries );
	bool CreateTopLevel( DeviceContext * device );
	bool Finalize( DeviceContext * device, const VkGeometryNV * geometries );

	void Cleanup( DeviceContext * device );




	//
	//	Alternative method for building
	//
	std::vector< structure > m_bottomLevels;
	std::vector< VkGeometryNV > m_geometries;
	std::vector< GeometryInstance > m_instances;
	bool AddGeometry( DeviceContext * device, Model * model, const Vec3 & pos, const Quat & orient );	// used to create instances and build bottom levels
	bool Build( DeviceContext * device );	// used to create the top level

	void UpdateInstances( DeviceContext * device, const RenderModel * models );
};