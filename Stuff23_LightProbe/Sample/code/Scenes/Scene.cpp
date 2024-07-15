//
//  Scene.cpp
//
#include "Scene.h"
#include "Miscellaneous/Input.h"
#include "Physics/Intersections.h"

Scene * g_scene = NULL;

/*
========================================================================================================

Scene

========================================================================================================
*/

/*
====================================================
Scene::Reset
====================================================
*/
void Scene::Reset() {
	for ( int i = 0; i < m_entities.size(); i++ ) {
		delete m_entities[ i ];
	}
	m_entities.clear();

	m_physicsWorld.Reset();

	//Initialize();
}
