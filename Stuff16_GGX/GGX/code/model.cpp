//
//  model.cpp
//
#include "model.h"
#include "Vector.h"
#include "Fileio.h"
#include <string.h>

#pragma warning( disable : 4996 )

/*
====================================================
FloatToByte
// Assumes a float between [-1,1]
====================================================
*/
unsigned char FloatToByte_n11( const float f ) {
	int i = (int)(f * 127 + 128);
	return (unsigned char)i;
}
/*
====================================================
FloatToByte
// Assumes a float between [0,1]
====================================================
*/
unsigned char FloatToByte_01( const float f ) {
	int i = (int)(f * 255);
	return (unsigned char)i;
}

/*
====================================================
FillFullScreenQuad
====================================================
*/
void FillFullScreenQuad( Model & model ) {
	const int numVerts = 4;
	const int numIdxs = 6;
	vert_t	screenVerts[ numVerts ];
	int		screenIndices[ numIdxs ];

	memset( screenVerts, 0, sizeof( vert_t ) * 4 );

	screenVerts[ 0 ].xyz[ 0 ] = -1.0f;
	screenVerts[ 0 ].xyz[ 1 ] = -1.0f;
	screenVerts[ 0 ].xyz[ 2 ] = 0.0f;

	screenVerts[ 1 ].xyz[ 0 ] = 1.0f;
	screenVerts[ 1 ].xyz[ 1 ] = -1.0f;
	screenVerts[ 1 ].xyz[ 2 ] = 0.0f;

	screenVerts[ 2 ].xyz[ 0 ] = 1.0f;
	screenVerts[ 2 ].xyz[ 1 ] = 1.0f;
	screenVerts[ 2 ].xyz[ 2 ] = 0.0f;

	screenVerts[ 3 ].xyz[ 0 ] = -1.0f;
	screenVerts[ 3 ].xyz[ 1 ] = 1.0f;
	screenVerts[ 3 ].xyz[ 2 ] = 0.0f;


	screenVerts[ 0 ].st[ 0 ] = 0.0f;
	screenVerts[ 0 ].st[ 1 ] = 1.0f;

	screenVerts[ 1 ].st[ 0 ] = 1.0f;
	screenVerts[ 1 ].st[ 1 ] = 1.0f;

	screenVerts[ 2 ].st[ 0 ] = 1.0f;
	screenVerts[ 2 ].st[ 1 ] = 0.0f;

	screenVerts[ 3 ].st[ 0 ] = 0.0f;
	screenVerts[ 3 ].st[ 1 ] = 0.0f;

	screenVerts[ 0 ].buff[ 0 ] = 255;
	screenVerts[ 1 ].buff[ 0 ] = 255;
	screenVerts[ 2 ].buff[ 0 ] = 255;
	screenVerts[ 3 ].buff[ 0 ] = 255;


	screenIndices[ 0 ] = 0;
	screenIndices[ 1 ] = 1;
	screenIndices[ 2 ] = 2;

	screenIndices[ 3 ] = 0;
	screenIndices[ 4 ] = 2;
	screenIndices[ 5 ] = 3;

	for ( int i = 0; i < numVerts; i++ ) {
		model.m_vertices.push_back( screenVerts[ i ] );
	}

	for ( int i = 0; i < numIdxs; i++ ) {
		model.m_indices.push_back( screenIndices[ i ] );
	}
}

/*
====================================================
FillCube
====================================================
*/
void FillCube( Model & model ) {
	const int numIdxs = 3 * 2 * 6;
	const int numVerts = 4 * 6;
	vert_t	cubeVerts[ numVerts ];
	int		cubeIdxs[ numIdxs ];

	memset( cubeVerts, 0, sizeof( vert_t ) * 4 * 6 );

	for ( int face = 0; face < 6; face++ ) {
		const int dim0 = face / 2;
		const int dim1 = ( dim0 + 1 ) % 3;
		const int dim2 = ( dim0 + 2 ) % 3;
		const float val = ( ( face & 1 ) == 0 ) ? -1.0f : 1.0f;

		cubeVerts[ face * 4 + 0 ].xyz[ dim0 ] = val;
		cubeVerts[ face * 4 + 0 ].xyz[ dim1 ] = val;
		cubeVerts[ face * 4 + 0 ].xyz[ dim2 ] = val;

		cubeVerts[ face * 4 + 1 ].xyz[ dim0 ] = val;
		cubeVerts[ face * 4 + 1 ].xyz[ dim1 ] = -val;
		cubeVerts[ face * 4 + 1 ].xyz[ dim2 ] = val;

		cubeVerts[ face * 4 + 2 ].xyz[ dim0 ] = val;
		cubeVerts[ face * 4 + 2 ].xyz[ dim1 ] = -val;
		cubeVerts[ face * 4 + 2 ].xyz[ dim2 ] = -val;

		cubeVerts[ face * 4 + 3 ].xyz[ dim0 ] = val;
		cubeVerts[ face * 4 + 3 ].xyz[ dim1 ] = val;
		cubeVerts[ face * 4 + 3 ].xyz[ dim2 ] = -val;


		cubeVerts[ face * 4 + 0 ].st[ 0 ] = 0.0f;
		cubeVerts[ face * 4 + 0 ].st[ 1 ] = 1.0f;

		cubeVerts[ face * 4 + 1 ].st[ 0 ] = 1.0f;
		cubeVerts[ face * 4 + 1 ].st[ 1 ] = 1.0f;

		cubeVerts[ face * 4 + 2 ].st[ 0 ] = 1.0f;
		cubeVerts[ face * 4 + 2 ].st[ 1 ] = 0.0f;

		cubeVerts[ face * 4 + 3 ].st[ 0 ] = 0.0f;
		cubeVerts[ face * 4 + 3 ].st[ 1 ] = 0.0f;


		cubeVerts[ face * 4 + 0 ].norm[ dim0 ] = FloatToByte_n11( val );
		cubeVerts[ face * 4 + 1 ].norm[ dim0 ] = FloatToByte_n11( val );
		cubeVerts[ face * 4 + 2 ].norm[ dim0 ] = FloatToByte_n11( val );
		cubeVerts[ face * 4 + 3 ].norm[ dim0 ] = FloatToByte_n11( val );


		cubeVerts[ face * 4 + 0 ].tang[ dim1 ] = FloatToByte_n11( val );
		cubeVerts[ face * 4 + 1 ].tang[ dim1 ] = FloatToByte_n11( val );
		cubeVerts[ face * 4 + 2 ].tang[ dim1 ] = FloatToByte_n11( val );
		cubeVerts[ face * 4 + 3 ].tang[ dim1 ] = FloatToByte_n11( val );


		cubeIdxs[ face * 6 + 0 ] = face * 4 + 0;
		cubeIdxs[ face * 6 + 1 ] = face * 4 + 1;
		cubeIdxs[ face * 6 + 2 ] = face * 4 + 2;

		cubeIdxs[ face * 6 + 3 ] = face * 4 + 0;
		cubeIdxs[ face * 6 + 4 ] = face * 4 + 2;
		cubeIdxs[ face * 6 + 5 ] = face * 4 + 3;
	}

	for ( int i = 0; i < numVerts; i++ ) {
		model.m_vertices.push_back( cubeVerts[ i ] );
	}

	for ( int i = 0; i < numIdxs; i++ ) {
		model.m_indices.push_back( cubeIdxs[ i ] );
	}
}

void Vec3dToFloat3( const Vec3d & v, float * f ) {
	f[ 0 ] = v.x;
	f[ 1 ] = v.y;
	f[ 2 ] = v.z;
}
void Vec2dToFloat2( const Vec2d & v, float * f ) {
	f[ 0 ] = v.x;
	f[ 1 ] = v.y;
}
void Vec3dToByte4( const Vec3d & v, unsigned char * b ) {
	b[ 0 ] = FloatToByte_n11( v.x );
	b[ 1 ] = FloatToByte_n11( v.y );
	b[ 2 ] = FloatToByte_n11( v.z );
	b[ 3 ] = 0;
}

/*
====================================================
Model::LoadOBJ
====================================================
*/
bool Model::LoadOBJ( const char * localFileName, bool doSwapYZ ) {
	char fileName[ 2048 ];
	RelativePathToFullPath( localFileName, fileName );

	FILE * fp = fopen( fileName, "rb" );
	if ( !fp ) {
		fprintf( stderr, "Error: couldn't open \"%s\"!\n", fileName );
		return false;
	}

	char buff[ 512 ] = { 0 };

	Vec3d point;
	Vec3d norm;
	Vec2d st;

	int i[ 4 ];
	int j[ 4 ];
	int k[ 4 ];

	std::vector< Vec3d > mPositions;
	std::vector< Vec3d > mNormals;
	std::vector< Vec2d > mST;

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
				Vec3dToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].xyz );
				//Vec3dToByte4( mNormals[ k[ it ] - 1 ], vert[ it ].norm );
				vert[ it ].norm[ 0 ] = mNormals[ k[ it ] - 1 ][ 0 ];
				vert[ it ].norm[ 1 ] = mNormals[ k[ it ] - 1 ][ 1 ];
				vert[ it ].norm[ 2 ] = mNormals[ k[ it ] - 1 ][ 2 ];
				vert[ it ].norm[ 3 ] = 0.0f;
				Vec2dToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
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
				Vec3dToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].xyz );
				//Vec3dToByte4( mNormals[ k[ it ] - 1 ], vert[ it ].norm );
				vert[ it ].norm[ 0 ] = mNormals[ k[ it ] - 1 ][ 0 ];
				vert[ it ].norm[ 1 ] = mNormals[ k[ it ] - 1 ][ 1 ];
				vert[ it ].norm[ 2 ] = mNormals[ k[ it ] - 1 ][ 2 ];
				vert[ it ].norm[ 3 ] = 0.0f;
				Vec2dToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
			}
			m_vertices.push_back( vert[ 0 ] );
			m_vertices.push_back( vert[ 1 ] );
			m_vertices.push_back( vert[ 2 ] );
		} else if ( sscanf( buff, "f %i/%i %i/%i %i/%i %i/%i", &i[ 0 ], &j[ 0 ], &i[ 1 ], &j[ 1 ], &i[ 2 ], &j[ 2 ], &i[ 3 ], &j[ 3 ] ) == 8 ) {
			// Convert this quad to two triangles and append
			vert_t vert[ 4 ];
			for ( int it = 0; it < 4; ++it ) {
				Vec3dToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].xyz );
				Vec2dToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
			}

			// Calculate normal from face geometry
			Vec3d a = Vec3d( vert[ 0 ].xyz ) - Vec3d( vert[ 1 ].xyz );
			Vec3d b = Vec3d( vert[ 2 ].xyz ) - Vec3d( vert[ 1 ].xyz );
			Vec3d norm = b.Cross( a );
			norm.Normalize();
			for ( int it = 0; it < 4; ++it ) {
				//Vec3dToByte4( norm, vert[ it ].norm );
				vert[ it ].norm[ 0 ] = norm[ 0 ];
				vert[ it ].norm[ 1 ] = norm[ 1 ];
				vert[ it ].norm[ 2 ] = norm[ 2 ];
				vert[ it ].norm[ 3 ] = 0.0f;
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
				Vec3dToFloat3( mPositions[ i[ it ] - 1 ], vert[ it ].xyz );
				Vec2dToFloat2( mST[ j[ it ] - 1 ], vert[ it ].st );
			}

			// Calculate normal from face geometry
			Vec3d a = Vec3d( vert[ 0 ].xyz ) - Vec3d( vert[ 1 ].xyz );
			Vec3d b = Vec3d( vert[ 2 ].xyz ) - Vec3d( vert[ 1 ].xyz );
			Vec3d norm = b.Cross( a );
			norm.Normalize();
			for ( int it = 0; it < 3; ++it ) {
				//Vec3dToByte4( norm, vert[ it ].norm );
				vert[ it ].norm[ 0 ] = norm[ 0 ];
				vert[ it ].norm[ 1 ] = norm[ 1 ];
				vert[ it ].norm[ 2 ] = norm[ 2 ];
				vert[ it ].norm[ 3 ] = 0.0f;
			}

			m_vertices.push_back( vert[ 0 ] );
			m_vertices.push_back( vert[ 1 ] );
			m_vertices.push_back( vert[ 2 ] );
		}
	}

	Vec3d min = m_vertices[ 0 ].xyz;
	Vec3d max = m_vertices[ 0 ].xyz;

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		const vert_t & vert = m_vertices[ i ];
		for ( int j = 0; j < 3; j++ ) {
			if ( vert.xyz[ j ] < min[ j ] ) {
				min[ j ] = vert.xyz[ j ];
			}
			if ( vert.xyz[ j ] > max[ j ] ) {
				max[ j ] = vert.xyz[ j ];
			}
		}
	}

	printf( "----------------------\n" );
	printf( "Debug Extents:\n" );
	printf( "min = %f %f %f\n", min.x, min.y, min.z );
	printf( "max = %f %f %f\n", max.x, max.y, max.z );
	printf( "----------------------\n" );

	CalculateTangents();

	if ( doSwapYZ ) {
		for ( int i = 0; i < m_vertices.size(); i++ ) {
			vert_t & vert = m_vertices[ i ];
			vert.st[ 1 ] = 1.0f - vert.st[ 1 ];
		}
	}

	// Print normals
// 	for ( int i = 0; i < m_vertices.size(); i++ ) {
// 		const vert_t & vert = m_vertices[ i ];
// 		printf( "%i: %f %f %f\n", i, vert.norm[ 0 ], vert.norm[ 1 ], vert.norm[ 2 ] );
// 	}

	// Build the index list.  Something we should do in the future is optimize it
	// so that we don't duplicate vertex data

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		m_indices.push_back( i );
	}

	return true;
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

		// grab the vertices to the triangle
		Vec3d vA = v0.xyz;
		Vec3d vB = v1.xyz;
		Vec3d vC = v2.xyz;

		Vec3d vAB = vB - vA;
		Vec3d vAC = vC - vA;

		// get the ST mapping values for the triangle
		Vec2d stA = v0.st;
		Vec2d stB = v1.st;
		Vec2d stC = v2.st;
		Vec2d stAB = stB - stA;
		Vec2d stAC = stC - stA;
//		stAB.Normalize();
//		stAC.Normalize();

		// calculate tangents
		Vec3d tangent;
#if 0
		{
			//Vec2d stTangent = Vec2d( 1, 0 );
			// find alpha, beta such that stTangent = alpha * stAB + beta * stAC;
			// 1 = alpha * stAB.x + beta * stAC.x;
			// 0 = alpha * stAB.y + beta * stAC.y;
			// beta = -alpha * stAB.y / stAC.y;
			// 1 = alpha * stAB.x - alpha * stAB.y / stAC.y * stAC.x;
			// 1 = alpha * ( stAB.x - stAB.y * stAC.x / stAC.y );
			// alpha = 1.0f / ( stAB.x - stAB.y * stAC.x / stAC.y );
			float alpha = 1.0f / ( stAB.x - stAB.y * stAC.x / stAC.y );
			float beta = -alpha * stAB.y / stAC.y;
			tangent = alpha * vAB + beta * vAC;
		}
#else
		{
			Vec3d v1 = vAC;
			Vec3d v2 = vAB;
			Vec2d st1 = stAC;
			Vec2d st2 = stAB;
			float coef = 1.0f / ( st1.x * st2.y - st2.x * st1.y );
			tangent.x = coef * ( ( v1.x * st2.y ) + ( v2.x * -st1.y ) );
			tangent.y = coef * ( ( v1.y * st2.y ) + ( v2.y * -st1.y ) );
			tangent.z = coef * ( ( v1.z * st2.y ) + ( v2.z * -st1.y ) );
		}
#endif
		tangent.Normalize();

		Vec3d normal0 = v0.norm;
		Vec3d normal1 = v1.norm;
		Vec3d normal2 = v2.norm;

		Vec3d bitangent0 = normal0.Cross( tangent );
		Vec3d bitangent1 = normal1.Cross( tangent );
		Vec3d bitangent2 = normal2.Cross( tangent );

		Vec3d tangent0 = bitangent0.Cross( normal0 );
		Vec3d tangent1 = bitangent0.Cross( normal1 );
		Vec3d tangent2 = bitangent0.Cross( normal2 );

		tangent0.Normalize();
		tangent1.Normalize();
		tangent2.Normalize();

		// store the tangents
// 		Vec3dToByte4( tangent, v0.tang );
// 		Vec3dToByte4( tangent, v1.tang );
// 		Vec3dToByte4( tangent, v2.tang );

		v0.tang[ 3 ] = 0;
		v1.tang[ 3 ] = 0;
		v2.tang[ 3 ] = 0;
		for ( int i = 0; i < 3; i++ ) {
			v0.tang[ i ] = tangent0[ i ];
			v1.tang[ i ] = tangent1[ i ];
			v2.tang[ i ] = tangent2[ i ];
		}
// 		v0.tang = tangent;
// 		v1.tang = tangent;
// 		v2.tang = tangent;
	}
}
