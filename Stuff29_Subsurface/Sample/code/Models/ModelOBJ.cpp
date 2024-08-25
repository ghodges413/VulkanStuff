//
//  ModelStatic.cpp
//
#include "Models/ModelStatic.h"
#include "Math/Vector.h"
#include "Miscellaneous/Fileio.h"
#include <string.h>
#include <algorithm>

#pragma warning( disable : 4996 )

bool Model::LoadOBJ2( const char * localFileName ) {
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

	std::vector< Vec3 > positions;
	std::vector< Vec3 > normals;
	std::vector< Vec2 > STs;

	struct tri_t {
		int pos[ 3 ];
		int norm[ 3 ];
		int st[ 3 ];

		Vec3 normal;
		Vec3 tangent;
		Vec3 bitangent;

		float area;
	};
	std::vector< tri_t > tris;

	while ( !feof( fp ) ) {
		// Read whole line
		fgets( buff, sizeof( buff ), fp );

		if ( sscanf( buff, "v %f %f %f", &point.x, &point.y, &point.z ) == 3 ) {
			// Add this point to the list of positions
			positions.push_back( point );
		} else if ( sscanf( buff, "vn %f %f %f", &norm.x, &norm.y, &norm.z ) == 3 ) {
			// Add this norm to the list of normals
			norm.Normalize();
			normals.push_back( norm );
		} else if ( sscanf( buff, "vt %f %f", &st.x, &st.y ) == 2 ) {
			// Add this texture coordinate to the list of texture coordinates
			STs.push_back( st );
		} else if ( sscanf( buff, "f %i/%i/%i %i/%i/%i %i/%i/%i %i/%i/%i", &i[ 0 ], &j[ 0 ], &k[ 0 ], &i[ 1 ], &j[ 1 ], &k[ 1 ], &i[ 2 ], &j[ 2 ], &k[ 2 ], &i[ 3 ], &j[ 3 ], &k[ 3 ] ) == 12 ) {
			// Convert this quad to two triangles and append
			tri_t tri;
			for ( int it = 0; it < 3; it++ ) {
				tri.pos[ it ] = i[ it ] - 1;
				tri.norm[ it ] = j[ it ] - 1;
				tri.st[ it ] = k[ it ] - 1;
			}
			tris.push_back( tri );

			for ( int it = 0; it < 3; it++ ) {
				int it2 = ( it + 2 ) % 4;
				tri.pos[ it ] = i[ it2 ] - 1;
				tri.norm[ it ] = j[ it2 ] - 1;
				tri.st[ it ] = k[ it2 ] - 1;
			}
			tris.push_back( tri );
		} else if ( sscanf( buff, "f %i/%i/%i %i/%i/%i %i/%i/%i", &i[ 0 ], &j[ 0 ], &k[ 0 ], &i[ 1 ], &j[ 1 ], &k[ 1 ], &i[ 2 ], &j[ 2 ], &k[ 2 ] ) == 9 ) {
			// Add this triangle to the list of verts
			tri_t tri;
			for ( int it = 0; it < 3; it++ ) {
				tri.pos[ it ] = i[ it ] - 1;
				tri.norm[ it ] = j[ it ] - 1;
				tri.st[ it ] = k[ it ] - 1;
			}
			tris.push_back( tri );
		} else if ( sscanf( buff, "f %i/%i %i/%i %i/%i %i/%i", &i[ 0 ], &j[ 0 ], &i[ 1 ], &j[ 1 ], &i[ 2 ], &j[ 2 ], &i[ 3 ], &j[ 3 ] ) == 8 ) {
			// Convert this quad to two triangles and append
			tri_t tri;
			for ( int it = 0; it < 3; it++ ) {
				tri.pos[ it ] = i[ it ] - 1;
				tri.norm[ it ] = -1;
				tri.st[ it ] = j[ it ] - 1;
			}
			tris.push_back( tri );

			for ( int it = 0; it < 3; it++ ) {
				int it2 = ( it + 2 ) % 4;
				tri.pos[ it ] = i[ it2 ] - 1;
				tri.norm[ it ] = -1;
				tri.st[ it ] = j[ it2 ] - 1;
			}
			tris.push_back( tri );
		} else if ( sscanf( buff, "f %i/%i %i/%i %i/%i", &i[ 0 ], &j[ 0 ], &i[ 1 ], &j[ 1 ], &i[ 2 ], &j[ 2 ] ) == 6 ) {
			// Add this triangle to the list of verts
			tri_t tri;
			for ( int it = 0; it < 3; it++ ) {
				tri.pos[ it ] = i[ it ] - 1;
				tri.norm[ it ] = -1;
				tri.st[ it ] = j[ it ] - 1;
			}
			tris.push_back( tri );
		}
	}


	// For now, let's assume that none of the normals have been stored in the obj by the artist.
	// Instead we must calculate all the normals.
	// We will assume a smooth manifold and that all shared vertices are to have their normal calculated smoothly
	// across the triangles that share that vertex.

	// First we will calculate the face normal and tangent for each triangle
	for ( int i = 0; i < tris.size(); i++ ) {
		tri_t & tri = tris[ i ];

		Vec3 a = positions[ tri.pos[ 0 ] ];
		Vec3 b = positions[ tri.pos[ 1 ] ];
		Vec3 c = positions[ tri.pos[ 2 ] ];

		Vec3 x1 = b - a;
		Vec3 x2 = c - a;

		tri.normal = x1.Cross( x2 );
		tri.area = tri.normal.GetMagnitude() * 0.5f;
		tri.normal.Normalize();

		// x1, x2 can act as the basis vectors of the triangle's plane in 3d model space.  They span a 2d plane in 3d space.
		// And can be used to describe any vector in the 2d plane via an equation:
		// xn = alpha * x1 + beta * x2
		//
		// u1, u2 are the basis vectors of the triangle in 2d texture space.
		// Similarly, any vector in the 3d plane of the model space corresponds to a 2d texture space vector:
		// un = alpha * u1 + beta * u2
		//
		// The tangent vector in texture space is ( 1, 0 ).  And for the tangent vector there is
		// a vector ( alpha, beta ) such that:
		// ( 1 ) = ( u1.x, u2.x ) * ( alpha )
		// ( 0 ) = ( u1.y, u2.y ) * ( beta )
		//
		// We can solve for ( alpha, beta ) by taking the inverse of the matrix.  Then, we can
		// use ( alpha, beta ) to get the tangent vector in 3d model space.

		Vec2 st0 = STs[ tri.st[ 0 ] ];
		Vec2 st1 = STs[ tri.st[ 1 ] ];
		Vec2 st2 = STs[ tri.st[ 2 ] ];

		Vec2 u1 = st1 - st0;
		Vec2 u2 = st2 - st0;

		Mat2 matrix;
		matrix.rows[ 0 ] = Vec2( u1.x, u2.x );
		matrix.rows[ 1 ] = Vec2( u1.y, u2.y );

		Mat2 invMat;
		invMat = matrix.Inverse();

		Vec2 alphaBeta = invMat * Vec2( 1, 0 );
		float alpha = alphaBeta.x;
		float beta = alphaBeta.y;

		tri.tangent = x1 * alpha + x2 * beta;
		tri.tangent.Normalize();

		alphaBeta = invMat * Vec2( 0, 1 );
		alpha = alphaBeta.x;
		beta = alphaBeta.y;

		tri.bitangent = x1 * alpha + x2 * beta;
		tri.bitangent.Normalize();

		float det = matrix.Determinant();
		if ( det < 0.0f ) {
			printf( "We got a negative determinant!\n" );
		}

		Vec3 bitang = tri.normal.Cross( tri.tangent );
		bitang.Normalize();

		float bidot = bitang.Dot( tri.bitangent );
		if ( bidot < 0.0f ) {
			printf( "The two bitangents point in opposite directions!\n" );
		}

// 		if ( matrix.Determinant() < 0.0f ) {
// 			tri.tangent = tri.tangent * -1.0f;
// 		}
	}

	// Now let's create the vertices.. and let's be lazy here right now
	m_vertices.clear();
	m_indices.clear();

	int count = 0;
	for ( int i = 0; i < tris.size(); i++ ) {
		const tri_t & tri = tris[ i ];

		for ( int j = 0; j < 3; j++ ) {
			vert_t vert;
			memset( &vert, 0, sizeof( vert_t ) );

			const int pid = tri.pos[ j ];
			const int stid = tri.st[ j ];
			vert.pos = positions[ pid ];
			vert.st = STs[ stid ];

			// Now we need to do the brute force accumulate normals
			Vec3 normal = tri.normal * tri.area;
			Vec3 tangent = tri.tangent * tri.area;
			Vec3 bitangent = tri.bitangent * tri.area;

			for ( int k = 0; k < tris.size(); k++ ) {
				if ( k == i ) {
					continue;
				}

				const tri_t & tri2 = tris[ k ];

				for ( int m = 0; m < 3; m++ ) {
					const int pid2 = tri2.pos[ m ];
					if ( pid == pid2 ) {
						normal += tri2.normal * tri2.area;

						const int stid2 = tri2.st[ m ];
						if ( stid == stid2 ) {
 							tangent += tri2.tangent * tri2.area;
 							bitangent += tri2.bitangent * tri2.area;
						}

						break;
					}
				}
			}

			normal.Normalize();
			tangent.Normalize();
			bitangent.Normalize();

			vert_t::Vec3ToByte4( normal, vert.norm );
			vert_t::Vec3ToByte4( tangent, vert.tang );
			vert_t::Vec3ToByte4( bitangent, vert.buff );

			m_vertices.push_back( vert );
			m_indices.push_back( count );
			count++;
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

		// Hack to curve the model
// 		Vec3 normal = m_vertices[ i ].pos;
// 		normal.Normalize();
//		Vec3ToByte4( normal, m_vertices[ i ].norm );
	}

	m_bounds.Clear();
	for ( int i = 0; i < m_vertices.size(); i++ ) {
		m_bounds.Expand( m_vertices[ i ].pos );
	}

// 	for ( int i = 0; i < m_vertices.size(); i++ ) {
// 		vert_t & v = m_vertices[ i ];
// 		v.pos.x = v.st.x;// * 3.0f;
// 		v.pos.y = -v.st.y;
// 		v.pos.z = 0;
// 	}

	return true;
}


/*
====================================================
Model::LoadOBJ
====================================================
*/
bool Model::LoadOBJ( const char * localFileName ) {
	return LoadOBJ2( localFileName );

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

		// Hack to curve the model
		Vec3 normal = m_vertices[ i ].pos;
		normal.Normalize();
//		Vec3ToByte4( normal, m_vertices[ i ].norm );
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
