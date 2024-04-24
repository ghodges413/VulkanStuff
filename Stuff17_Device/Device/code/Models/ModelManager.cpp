//
//  ModelManager.cpp
//
#include "ModelManager.h"
#include "ModelStatic.h"
#include "Math/Vector.h"
//#include "Math/Random.h"
#include "Fileio.h"
#include <string.h>
//#include "../../Physics/Shapes.h"
#include <algorithm>

#pragma warning( disable : 4996 )

/*
========================================================================================================

ModelManager

========================================================================================================
*/

ModelManager * g_modelManager = NULL;

/*
====================================================
ModelManager::ModelManager
====================================================
*/
ModelManager::ModelManager( DeviceContext * device ) : m_device( device ) {
}

/*
====================================================
ModelManager::~ModelManager
====================================================
*/
ModelManager::~ModelManager() {
	for ( int i = 0; i < m_models.size(); i++ ) {
		Model * model = m_models[ i ];

		if ( model->m_isVBO ) {
			model->Cleanup();
		}
		delete model;
	}

	m_models.clear();
}

/*
====================================================
ModelManager::GetModel
====================================================
*/
Model * ModelManager::GetModel( const char * name ) {
	assert( NULL != name );
	std::string nameStr = name;
    
	Models_t::iterator it = m_modelMap.find( nameStr.data() );
	if ( it != m_modelMap.end() ) {
		// model found!  return it!
        assert( NULL != it->second );
		return it->second;
	}

    Model * model = AllocateModel();
	const bool result = model->LoadOBJ( name );
    assert( result );
	if ( false == result ) {
        // model not found
		printf( "Unable to Load/Find model: %s\n", name );
        return NULL;
	}

	model->MakeVBO( m_device );
	
	// store model and return
	m_modelMap[ nameStr ] = model;
	return model;
}

/*
====================================================
ModelManager::AllocateModel
====================================================
*/
Model * ModelManager::AllocateModel() {
	Model * model = new Model;
	model->m_device = m_device;

	m_models.push_back( model );
	return model;
}
