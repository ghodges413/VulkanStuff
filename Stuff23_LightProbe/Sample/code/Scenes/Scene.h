//
//  Scene.h
//
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <array>
#include <set>

#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Physics/Constraints.h"
#include "Physics/Shapes.h"
#include "Physics/Body.h"
#include "Physics/Contact.h"
#include "Physics/Manifold.h"
#include "Physics/BroadPhase.h"
#include "Physics/PhysicsWorld.h"

#include "Graphics/DeviceContext.h"
#include "Graphics/shader.h"

#include "Entities/Entity.h"
#include "Entities/Player.h"

/*
====================================================
Scene
====================================================
*/
class Scene {
public:
	Scene() { m_entities.reserve( 128 ); m_entities.clear(); }

	virtual void Initialize( DeviceContext * device ) = 0;
	virtual void Update( const float dt_sec ) = 0;
	virtual void Draw( VkCommandBuffer cmdBuffer ) = 0;

	PhysicsWorld	m_physicsWorld;

	std::vector< Entity * >	m_entities;
	Body *		m_playerBody;
	Player		m_playerEntity;

	void Reset();
};

class SceneBlank : public Scene {
public:
	SceneBlank() : Scene() {}

	void Initialize( DeviceContext * device ) override {}
	void Update( const float dt_sec ) override {}
	void Draw( VkCommandBuffer cmdBuffer ) override {}
};

extern Scene * g_scene;