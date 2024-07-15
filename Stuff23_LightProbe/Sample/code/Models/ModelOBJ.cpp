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
Model::LoadOBJ
====================================================
*/
bool Model::LoadOBJ( const char * localFileName ) {
	char fileName[ 2048 ];
	RelativePathToFullPath( localFileName, fileName );

	FILE * fp = fopen( fileName, "rb" );
	if ( !fp ) {
		fprintf( stderr, "Error: couldn't open \"%s\"!\n", fileName );
		return false;
	}

	char buff[ 512 ] = { 0 };

	Vec3 point;
	Vec3 norm;
	Vec2 st;

	int i[ 4 ];
	int j[ 4 ];
	int k[ 4 ];

	std::vector< Vec3 > mPositions;
	std::vector< Vec3 > mNormals;
	std::vector< Vec2 > mST;

	while ( !feof( fp ) ) {
		// Read whole line
		fgets( buff, sizeof( buff ), fp );

		if ( sscanf( buff, "v %f %f %f", &point.x, &point.y, &point.z ) == 3 ) {
			// Add this point to the list of positions
			mPositions.push_back( point );
		} else if ( sscanf( buff, "vn %f %f %f", &norm.x, &norm.y, &norm.z ) == 3 ) {
			// Add this norm to the list of normals
			norm.Normalize();
			mNormals.push_back( norm );
		} else if ( sscanf( buff, "vt %f %f", &st.x, &st.y ) == 2 ) {
			// Add this texture coordinate to the list of texture coordinates
			mST.push_back( st );
		} else if ( sscanf( buff, "f %i/%i/%i %i/%i/%i %i/%i/%i %i/%i/%i", &i[ 0 ], &j[ 0 ], &k[ 0 ], &i[ 1 ], &j[ 1 ], &k[ 1 ], &i[ 2 ], &j[ 2 ], &k[ 2 ], &i[ 3 ], &j[ 3 ], &k[ 3 ] ) == 12 ) {
			// Convert this quad to two triangles and append
			vert_t vert[ 4 ];
			for ( int it = 0; it < 4; ++it ) {
				//Vec3ToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].pos );
				vert[ it ].pos = mPositions[ i[ it ] - 1 ];
				Vec3ToByte4( mNormals[ k[ it ] - 1 ], vert[ it ].norm );
// 				vert[ it ].norm[ 0 ] = mNormals[ k[ it ] - 1 ][ 0 ];
// 				vert[ it ].norm[ 1 ] = mNormals[ k[ it ] - 1 ][ 1 ];
// 				vert[ it ].norm[ 2 ] = mNormals[ k[ it ] - 1 ][ 2 ];
// 				vert[ it ].norm[ 3 ] = 0.0f;
				//Vec2ToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
				vert[ it ].st = mST[ j[ it ] - 1 ];
			}
			m_vertices.push_back( vert[ 0 ] );
			m_vertices.push_back( vert[ 1 ] );
			m_vertices.push_back( vert[ 2 ] );

			m_vertices.push_back( vert[ 2 ] );
			m_vertices.push_back( vert[ 3 ] );
			m_vertices.push_back( vert[ 0 ] );
		} else if ( sscanf( buff, "f %i/%i/%i %i/%i/%i %i/%i/%i", &i[ 0 ], &j[ 0 ], &k[ 0 ], &i[ 1 ], &j[ 1 ], &k[ 1 ], &i[ 2 ], &j[ 2 ], &k[ 2 ] ) == 9 ) {
			// Add this triangle to the list of verts
			vert_t vert[ 3 ];
			for ( int it = 0; it < 3; ++it ) {
				//Vec3ToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].pos );
				vert[ it ].pos = mPositions[ i[ it ] - 1 ];
				Vec3ToByte4( mNormals[ k[ it ] - 1 ], vert[ it ].norm );
// 				vert[ it ].norm[ 0 ] = mNormals[ k[ it ] - 1 ][ 0 ];
// 				vert[ it ].norm[ 1 ] = mNormals[ k[ it ] - 1 ][ 1 ];
// 				vert[ it ].norm[ 2 ] = mNormals[ k[ it ] - 1 ][ 2 ];
// 				vert[ it ].norm[ 3 ] = 0.0f;
				//Vec2ToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
				vert[ it ].st = mST[ j[ it ] - 1 ];
			}
			m_vertices.push_back( vert[ 0 ] );
			m_vertices.push_back( vert[ 1 ] );
			m_vertices.push_back( vert[ 2 ] );
		} else if ( sscanf( buff, "f %i/%i %i/%i %i/%i %i/%i", &i[ 0 ], &j[ 0 ], &i[ 1 ], &j[ 1 ], &i[ 2 ], &j[ 2 ], &i[ 3 ], &j[ 3 ] ) == 8 ) {
			// Convert this quad to two triangles and append
			vert_t vert[ 4 ];
			for ( int it = 0; it < 4; ++it ) {
				//Vec3ToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].pos );
				vert[ it ].pos = mPositions[ i[ it ] - 1 ];
				//Vec2ToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
				vert[ it ].st = mST[ j[ it ] - 1 ];
			}

			// Calculate normal from face geometry
			Vec3 a = Vec3( vert[ 0 ].pos ) - Vec3( vert[ 1 ].pos );
			Vec3 b = Vec3( vert[ 2 ].pos ) - Vec3( vert[ 1 ].pos );
			Vec3 norm = b.Cross( a );
			norm.Normalize();
			for ( int it = 0; it < 4; ++it ) {
				Vec3ToByte4( norm, vert[ it ].norm );
// 				vert[ it ].norm[ 0 ] = norm[ 0 ];
// 				vert[ it ].norm[ 1 ] = norm[ 1 ];
// 				vert[ it ].norm[ 2 ] = norm[ 2 ];
// 				vert[ it ].norm[ 3 ] = 0.0f;
			}

			m_vertices.push_back( vert[ 0 ] );
			m_vertices.push_back( vert[ 1 ] );
			m_vertices.push_back( vert[ 2 ] );

			m_vertices.push_back( vert[ 2 ] );
			m_vertices.push_back( vert[ 3 ] );
			m_vertices.push_back( vert[ 0 ] );
		} else if ( sscanf( buff, "f %i/%i %i/%i %i/%i", &i[ 0 ], &j[ 0 ], &i[ 1 ], &j[ 1 ], &i[ 2 ], &j[ 2 ] ) == 6 ) {
			// Add this triangle to the list of verts
			vert_t vert[ 3 ];
			for ( int it = 0; it < 3; ++it ) {
				//Vec3ToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].pos );
				//Vec2ToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
				vert[ it ].pos = mPositions[ i[ it ] - 1 ];
				vert[ it ].st = mST[ j[ it ] - 1 ];
			}

			// Calculate normal from face geometry
			Vec3 a = Vec3( vert[ 0 ].pos ) - Vec3( vert[ 1 ].pos );
			Vec3 b = Vec3( vert[ 2 ].pos ) - Vec3( vert[ 1 ].pos );
			Vec3 norm = b.Cross( a );
			norm.Normalize();
			for ( int it = 0; it < 3; ++it ) {
				Vec3ToByte4( norm, vert[ it ].norm );
// 				vert[ it ].norm[ 0 ] = norm[ 0 ];
// 				vert[ it ].norm[ 1 ] = norm[ 1 ];
// 				vert[ it ].norm[ 2 ] = norm[ 2 ];
// 				vert[ it ].norm[ 3 ] = 0.0f;
			}

			m_vertices.push_back( vert[ 0 ] );
			m_vertices.push_back( vert[ 1 ] );
			m_vertices.push_back( vert[ 2 ] );
		}
	}

	Vec3 min = m_vertices[ 0 ].pos;
	Vec3 max = m_vertices[ 0 ].pos;

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		const vert_t & vert = m_vertices[ i ];
		for ( int j = 0; j < 3; j++ ) {
			if ( vert.pos[ j ] < min[ j ] ) {
				min[ j ] = vert.pos[ j ];
			}
			if ( vert.pos[ j ] > max[ j ] ) {
				max[ j ] = vert.pos[ j ];
			}
		}
	}

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		vert_t & vert = m_vertices[ i ];
		vert.st[ 1 ] = 1.0f - vert.st[ 1 ];
	}

	printf( "----------------------\n" );
	printf( "Debug Extents:\n" );
	printf( "min = %f %f %f\n", min.x, min.y, min.z );
	printf( "max = %f %f %f\n", max.x, max.y, max.z );
	printf( "----------------------\n" );

	CalculateTangents();

	// Build the index list.  Something we should do in the future is optimize it
	// so that we don't duplicate vertex data

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		m_indices.push_back( i );
	}

	m_bounds.Clear();
	for ( int i = 0; i < m_vertices.size(); i++ ) {
		m_bounds.Expand( m_vertices[ i ].pos );
	}

	// Hack: Re-center the model around its geometric center
	Vec3 center = m_bounds.Center();
	for ( int i = 0; i < m_vertices.size(); i++ ) {
		m_vertices[ i ].pos = m_vertices[ i ].pos - center;
	}

	m_bounds.Clear();
	for ( int i = 0; i < m_vertices.size(); i++ ) {
		m_bounds.Expand( m_vertices[ i ].pos );
	}

	return true;
}








/*
====================================================
OBJ_SaveSimple
====================================================
*/
bool OBJ_SaveSimple( const char * localFileName, const Vec3 * verts, const int numVerts, const int * indices, const int numIndices ) {
	char fileName[ 2048 ];
	RelativePathToFullPath( localFileName, fileName );

	FILE * fp = fopen( fileName, "wb" );
	if ( !fp ) {
		fprintf( stderr, "Error: couldn't open \"%s\"!\n", fileName );
		return false;
	}

	char buff[ 512 ] = { 0 };

	// Write verts
	for ( int i = 0; i < numVerts; i++ ) {
		const Vec3 & v = verts[ i ];

		memset( buff, 0, 512 );
		sprintf_s( buff, 512, "v %f %f %f\n", v.x, v.y, v.z );
		fwrite( buff, 1, strlen( buff ), fp );
	}

	// Write indices
	for ( int i = 0; i < numIndices / 3; i++ ) {
		// obj indices start 1, instead of 0
		const int a = indices[ 3 * i + 0 ] + 1;
		const int b = indices[ 3 * i + 1 ] + 1;
		const int c = indices[ 3 * i + 2 ] + 1;

		memset( buff, 0, 512 );
		sprintf_s( buff, 512, "f %i %i %i\n", a, b, c );
		fwrite( buff, 1, strlen( buff ), fp );
	}

	fclose( fp );
	return true;
}
