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

			// HACK: default the vertex color to white for ray traced GI
			//  In a real renderer, we would use the texture data to calculate the color
			vert.buff[ 0 ] = 255;//101;
			vert.buff[ 1 ] = 255;//48;
			vert.buff[ 2 ] = 255;//36;
			vert.buff[ 3 ] = 255;

			m_vertices.push_back( vert );
			m_indices.push_back( count );
			count++;
		}
	}

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		vert_t & vert = m_vertices[ i ];
		vert.st[ 1 ] = 1.0f - vert.st[ 1 ];
	}

	// Build the index list.  Something we should do in the future is optimize it
	// so that we don't duplicate vertex data

	for ( int i = 0; i < m_vertices.size(); i++ ) {
		m_indices.push_back( i );
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
