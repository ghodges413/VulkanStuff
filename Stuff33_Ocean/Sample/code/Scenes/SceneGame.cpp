//
//  SceneGame.cpp
//
#include "SceneGame.h"
#include "Physics/Intersections.h"
#include "Miscellaneous/Input.h"
#include "Graphics/DeviceContext.h"
#include "BSP/Map.h"

#include <algorithm>

Player * g_player;

/*
========================================================================================================

SceneGame

========================================================================================================
*/

/*
====================================================
SceneGame::SceneGame
====================================================
*/
SceneGame::SceneGame() : Scene() {
	g_physicsWorld = &m_physicsWorld;
	g_player = &m_playerEntity;
}

/*
====================================================
SceneGame::~SceneGame
====================================================
*/
SceneGame::~SceneGame() {
	for ( int i = 0; i < m_entities.size(); i++ ) {
		delete m_entities[ i ];
	}
	m_entities.clear();
}

/*
====================================================
SceneGame::Initialize
====================================================
*/
void SceneGame::Initialize( DeviceContext * device ) {
	AddPlayerBody();

	LoadMap( device );
}

/*
====================================================
SceneGame::Update
====================================================
*/
void SceneGame::Update( const float dt_sec ) {
	m_playerEntity.Update( dt_sec );
	m_physicsWorld.StepSimulation( dt_sec );
}

/*
====================================================
SceneGame::AddPlayerBody
====================================================
*/
void SceneGame::AddPlayerBody() {
	Body body;
	body.m_position = Vec3( -15, 0, 5.5f );
	body.m_orientation = Quat( 0, 0, 0, 1 );
	body.m_shape = new ShapeCapsule( 0.5f, 1.0f );
	body.m_invMass = 0.5f;
	body.m_elasticity = 0.0f;
	body.m_enableRotation = false;
	body.m_friction = 1.0f;

	body.m_bodyContents = BC_PLAYER;
	body.m_collidesWith = BC_ENEMY | BC_GENERIC;

	bodyID_t bodyid = m_physicsWorld.AllocateBody( body );

	m_playerBody = bodyid.body;
	m_playerEntity.m_bodyid = bodyid;
}

/*
====================================================
SceneGame::Draw
====================================================
*/
void SceneGame::Draw( VkCommandBuffer cmdBuffer ) {
}
