//
//  ModelStatic.cpp
//
#include "Models/ModelStatic.h"
#include "Math/Vector.h"
#include "Miscellaneous/Fileio.h"
#include <string.h>
#include <algorithm>

#pragma warning( disable : 4996 )

/*
====================================================
FloatToByte
// Assumes a float between [-1,1]
====================================================
*/
unsigned char vert_t::FloatToByte_n11( const float f ) {
	int i = (int)(f * 127 + 128);
	return (unsigned char)i;
}

/*
====================================================
FloatToByte
// Assumes a float between [0,1]
====================================================
*/
unsigned char vert_t::FloatToByte_01( const float f ) {
	int i = (int)(f * 255);
	return (unsigned char)i;
}
Vec3 vert_t::Byte4ToVec3( const unsigned char * data ) {
	Vec3 pt;
	pt.x = float( data[ 0 ] ) / 255.0f;	// 0,1
	pt.y = float( data[ 1 ] ) / 255.0f;	// 0,1
	pt.z = float( data[ 2 ] ) / 255.0f;	// 0,1

	pt.x = 2.0f * ( pt.x - 0.5f ); //-1,1
	pt.y = 2.0f * ( pt.y - 0.5f ); //-1,1
	pt.z = 2.0f * ( pt.z - 0.5f ); //-1,1
	return pt;
}
void vert_t::Vec3ToFloat3( const Vec3 & v, float * f ) {
	f[ 0 ] = v.x;
	f[ 1 ] = v.y;
	f[ 2 ] = v.z;
}
void vert_t::Vec2ToFloat2( const Vec2 & v, float * f ) {
	f[ 0 ] = v.x;
	f[ 1 ] = v.y;
}
void vert_t::Vec3ToByte4( const Vec3 & v, unsigned char * b ) {
	Vec3 tmp = v;
	tmp.Normalize();
	b[ 0 ] = FloatToByte_n11( tmp.x );
	b[ 1 ] = FloatToByte_n11( tmp.y );
	b[ 2 ] = FloatToByte_n11( tmp.z );
	b[ 3 ] = 0;
}

/*
================================
Model::CalculateTangents
================================
*/
void Model::CalculateTangents() {
	for ( int i = 0; i < m_vertices.size(); i += 3 ) {
		vert_t & v0 = m_vertices[ i + 0 ];
		vert_t & v1 = m_vertices[ i + 1 ];
		vert_t & v2 = m_vertices[ i + 2 ];

		// calculate tangents
		Vec3 tangent;
		{
			// grab the vertices to the triangle
			Vec3 vA = v0.xyz;
			Vec3 vB = v1.xyz;
			Vec3 vC = v2.xyz;

			Vec3 vAB = vB - vA;
			Vec3 vAC = vC - vA;

			// get the ST mapping values for the triangle
			Vec2 stA = v0.st;
			Vec2 stB = v1.st;
			Vec2 stC = v2.st;
			Vec2 stAB = stB - stA;
			Vec2 stAC = stC - stA;

			//Vec2 stTangent = Vec2( 1, 0 );
			// find alpha, beta such that stTangent = alpha * stAB + beta * stAC;
			// 1 = alpha * stAB.x + beta * stAC.x;
			// 0 = alpha * stAB.y + beta * stAC.y;
			// beta = -alpha * stAB.y / stAC.y;
			// 1 = alpha * stAB.x - alpha * stAB.y / stAC.y * stAC.x;
			// 1 = alpha * ( stAB.x - stAB.y * stAC.x / stAC.y );
			// alpha = 1.0f / ( stAB.x - stAB.y * stAC.x / stAC.y );
			//float alpha = 1.0f / ( stAB.x - stAB.y * stAC.x / stAC.y );
			//float beta = -alpha * stAB.y / stAC.y;
			//tangent = alpha * vAB + beta * vAC;

			float denom = ( stAB.x * stAC.y - stAB.y * stAC.x );
			if ( denom < 0.00001f && denom > -0.00001f ) {
				denom = 1.0f;
			}
			float invDenom = 1.0f / denom;
			float alpha = stAC.y * invDenom;
			float beta = -stAB.y * invDenom;
			tangent = vAB * alpha + vAC * beta;
			tangent.Normalize();
		}

		Vec3 normal0 = vert_t::Byte4ToVec3( v0.norm );
		Vec3 normal1 = vert_t::Byte4ToVec3( v1.norm );
		Vec3 normal2 = vert_t::Byte4ToVec3( v2.norm );

		Vec3 bitangent0 = normal0.Cross( tangent );
		Vec3 bitangent1 = normal1.Cross( tangent );
		Vec3 bitangent2 = normal2.Cross( tangent );

		Vec3 tangent0 = bitangent0.Cross( normal0 );
		Vec3 tangent1 = bitangent0.Cross( normal1 );
		Vec3 tangent2 = bitangent0.Cross( normal2 );

		tangent0.Normalize();
		tangent1.Normalize();
		tangent2.Normalize();

		vert_t::Vec3ToByte4( tangent0, v0.tang );
		vert_t::Vec3ToByte4( tangent1, v1.tang );
		vert_t::Vec3ToByte4( tangent2, v2.tang );
	}
}

/*
================================
Model::MakeVBO
================================
*/
bool Model::MakeVBO( DeviceContext * device ) {
	m_device = device;

	VkCommandBuffer vkCommandBuffer = device->m_vkCommandBuffers[ 0 ];

	int bufferSize;

	// Create Vertex Buffer
	bufferSize = (int)sizeof( m_vertices[ 0 ] ) * m_vertices.size();
	if ( !m_vertexBuffer.Allocate( device, m_vertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
		printf( "failed to allocate vertex buffer!\n" );
		assert( 0 );
		return false;
	}

	// Create Index Buffer
	bufferSize = (int)sizeof( m_indices[ 0 ] ) * m_indices.size();
	if ( !m_indexBuffer.Allocate( device, m_indices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
		printf( "failed to allocate index buffer!\n" );
		assert( 0 );
		return false;
	}

	//
	//	Manifold Buffer
	//
	if ( m_manifoldVertices.size() > 0 ) {
		// Create Vertex Buffer
		bufferSize = (int)sizeof( m_manifoldVertices[ 0 ] ) * m_manifoldVertices.size();
		if ( !m_manifoldVertexBuffer.Allocate( device, m_manifoldVertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
			printf( "failed to allocate vertex buffer!\n" );
			assert( 0 );
			return false;
		}

		// Create Index Buffer
		bufferSize = (int)sizeof( m_manifoldIndices[ 0 ] ) * m_manifoldIndices.size();
		if ( !m_manifoldIndexBuffer.Allocate( device, m_manifoldIndices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
			printf( "failed to allocate index buffer!\n" );
			assert( 0 );
			return false;
		}

		// Create Meshlet Buffer
		bufferSize = (int)sizeof( m_manifoldMeshlets[ 0 ] ) * m_manifoldMeshlets.size();
		if ( !m_manifoldMeshletBuffer.Allocate( device, m_manifoldMeshlets.data(), bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
			printf( "failed to allocate meshlet buffer!\n" );
			assert( 0 );
			return false;
		}

		// Create Meshlet Bounds Buffer
		bufferSize = (int)sizeof( m_manifoldMeshletBounds[ 0 ] ) * m_manifoldMeshletBounds.size();
		if ( !m_manifoldBoundsBuffer.Allocate( device, m_manifoldMeshletBounds.data(), bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
			printf( "failed to allocate meshlet bounds buffer!\n" );
			assert( 0 );
			return false;
		}
	}

	m_isVBO = true;
	return true;
}

/*
================================
Model::Cleanup
================================
*/
void Model::Cleanup() {
	if ( !m_isVBO ) {
		return;
	}

	m_vertexBuffer.Cleanup( m_device );
	m_indexBuffer.Cleanup( m_device );

	m_manifoldVertexBuffer.Cleanup( m_device );
	m_manifoldIndexBuffer.Cleanup( m_device );
	m_manifoldMeshletBuffer.Cleanup( m_device );
	m_manifoldBoundsBuffer.Cleanup( m_device );
}

/*
================================
Model::GetVerts
================================
*/
void Model::GetVerts( std::vector< Vec3 > & verts ) {
	verts.clear();
	verts.reserve( m_vertices.size() );

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		Vec3 pt = Vec3( m_vertices[ i ].xyz );
		verts.push_back( pt );
	}
}

/*
====================================================
Model::DrawIndexed
====================================================
*/
void Model::DrawIndexed( VkCommandBuffer vkCommandBuffer ) {
	if ( m_manifoldVertexBuffer.m_vkBufferSize > 0 && m_manifoldIndexBuffer.m_vkBufferSize > 0 ) {
		// Bind the model
		VkBuffer vertexBuffers[] = { m_manifoldVertexBuffer.m_vkBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers( vkCommandBuffer, 0, 1, vertexBuffers, offsets );
		vkCmdBindIndexBuffer( vkCommandBuffer, m_manifoldIndexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32 );

		// Issue draw command
		vkCmdDrawIndexed( vkCommandBuffer, static_cast< uint32_t >( m_manifoldIndices.size() ), 1, 0, 0, 0 );
	} else {
		// Bind the model
		VkBuffer vertexBuffers[] = { m_vertexBuffer.m_vkBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers( vkCommandBuffer, 0, 1, vertexBuffers, offsets );
		vkCmdBindIndexBuffer( vkCommandBuffer, m_indexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32 );

		// Issue draw command
		vkCmdDrawIndexed( vkCommandBuffer, static_cast< uint32_t >( m_indices.size() ), 1, 0, 0, 0 );
	}
}
