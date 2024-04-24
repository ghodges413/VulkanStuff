//
//  ModelManager.h
//
#pragma once
#include <map>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include "DeviceContext.h"
#include "Buffer.h"
#include "Math/Vector.h"
#include "Math/Quat.h"

class Model;

/*
====================================================
ModelManager
====================================================
*/
class ModelManager {
public:
	explicit ModelManager( DeviceContext * device );
	~ModelManager();

	Model * GetModel( const char * name );
	Model * AllocateModel();

private:
	typedef std::map< void *, Model * > ModelShapes_t;
	ModelShapes_t	m_modelShapes;

	typedef std::map< std::string, Model * > Models_t;
	Models_t	m_modelMap;

	std::vector< Model * > m_models;

	DeviceContext * m_device;
};

extern ModelManager * g_modelManager;