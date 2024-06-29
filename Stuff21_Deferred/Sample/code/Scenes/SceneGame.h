//
//  SceneGame.h
//
#pragma once
#include "Scene.h"
#include "Physics/PhysicsWorld.h"

/*
====================================================
SceneGame
====================================================
*/
class SceneGame : public Scene {
public:
	SceneGame();
	~SceneGame();

	virtual void Initialize( DeviceContext * device );
	virtual void Update( const float dt_sec );
	void Draw( VkCommandBuffer cmdBuffer ) override;

	float m_timeAccumulator;

private:
	void AddPlayerBody();
};

