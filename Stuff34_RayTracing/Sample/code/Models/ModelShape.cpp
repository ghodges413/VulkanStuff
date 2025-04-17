//
//  ModelShape.cpp
//
#include "Models/ModelShape.h"
#include "Physics/Shapes.h"
#include "BSP/Brush.h"

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

	screenVerts[ 0 ].pos[ 0 ] = -1.0f;
	screenVerts[ 0 ].pos[ 1 ] = -1.0f;
	screenVerts[ 0 ].pos[ 2 ] = 0.0f;

	screenVerts[ 1 ].pos[ 0 ] = 1.0f;
	screenVerts[ 1 ].pos[ 1 ] = -1.0f;
	screenVerts[ 1 ].pos[ 2 ] = 0.0f;

	screenVerts[ 2 ].pos[ 0 ] = 1.0f;
	screenVerts[ 2 ].pos[ 1 ] = 1.0f;
	screenVerts[ 2 ].pos[ 2 ] = 0.0f;

	screenVerts[ 3 ].pos[ 0 ] = -1.0f;
	screenVerts[ 3 ].pos[ 1 ] = 1.0f;
	screenVerts[ 3 ].pos[ 2 ] = 0.0f;


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
#if 0
	for ( int face = 0; face < 6; face++ ) {
		const int dim0 = face / 2;
		const int dim1 = ( dim0 + 1 ) % 3;
		const int dim2 = ( dim0 + 2 ) % 3;
		const float val = ( ( face & 1 ) == 0 ) ? -1.0f : 1.0f;

		cubeVerts[ face * 4 + 0 ].pos[ dim0 ] = val;
		cubeVerts[ face * 4 + 0 ].pos[ dim1 ] = val;
		cubeVerts[ face * 4 + 0 ].pos[ dim2 ] = val;

		cubeVerts[ face * 4 + 1 ].pos[ dim0 ] = val;
		cubeVerts[ face * 4 + 1 ].pos[ dim1 ] = -val;
		cubeVerts[ face * 4 + 1 ].pos[ dim2 ] = val;

		cubeVerts[ face * 4 + 2 ].pos[ dim0 ] = val;
		cubeVerts[ face * 4 + 2 ].pos[ dim1 ] = -val;
		cubeVerts[ face * 4 + 2 ].pos[ dim2 ] = -val;

		cubeVerts[ face * 4 + 3 ].pos[ dim0 ] = val;
		cubeVerts[ face * 4 + 3 ].pos[ dim1 ] = val;
		cubeVerts[ face * 4 + 3 ].pos[ dim2 ] = -val;


		cubeVerts[ face * 4 + 0 ].st[ 0 ] = 0.0f;
		cubeVerts[ face * 4 + 0 ].st[ 1 ] = 1.0f;

		cubeVerts[ face * 4 + 1 ].st[ 0 ] = 1.0f;
		cubeVerts[ face * 4 + 1 ].st[ 1 ] = 1.0f;

		cubeVerts[ face * 4 + 2 ].st[ 0 ] = 1.0f;
		cubeVerts[ face * 4 + 2 ].st[ 1 ] = 0.0f;

		cubeVerts[ face * 4 + 3 ].st[ 0 ] = 0.0f;
		cubeVerts[ face * 4 + 3 ].st[ 1 ] = 0.0f;


		cubeVerts[ face * 4 + 0 ].norm[ dim0 ] = vert_t::FloatToByte_n11( val );
		cubeVerts[ face * 4 + 1 ].norm[ dim0 ] = vert_t::FloatToByte_n11( val );
		cubeVerts[ face * 4 + 2 ].norm[ dim0 ] = vert_t::FloatToByte_n11( val );
		cubeVerts[ face * 4 + 3 ].norm[ dim0 ] = vert_t::FloatToByte_n11( val );


		cubeVerts[ face * 4 + 0 ].tang[ dim1 ] = vert_t::FloatToByte_n11( val );
		cubeVerts[ face * 4 + 1 ].tang[ dim1 ] = vert_t::FloatToByte_n11( val );
		cubeVerts[ face * 4 + 2 ].tang[ dim1 ] = vert_t::FloatToByte_n11( val );
		cubeVerts[ face * 4 + 3 ].tang[ dim1 ] = vert_t::FloatToByte_n11( val );


		cubeIdxs[ face * 6 + 0 ] = face * 4 + 0;
		cubeIdxs[ face * 6 + 1 ] = face * 4 + 1;
		cubeIdxs[ face * 6 + 2 ] = face * 4 + 2;

		cubeIdxs[ face * 6 + 3 ] = face * 4 + 0;
		cubeIdxs[ face * 6 + 4 ] = face * 4 + 2;
		cubeIdxs[ face * 6 + 5 ] = face * 4 + 3;
	}
#else
	// +x
	cubeVerts[ 0 ].pos = Vec3( 1,-1, 1 );
	cubeVerts[ 1 ].pos = Vec3( 1,-1,-1 );
	cubeVerts[ 2 ].pos = Vec3( 1, 1,-1 );
	cubeVerts[ 3 ].pos = Vec3( 1, 1, 1 );

	cubeVerts[ 0 ].st = Vec2( 0, 0 );
	cubeVerts[ 1 ].st = Vec2( 0, 1 );
	cubeVerts[ 2 ].st = Vec2( 1, 1 );
	cubeVerts[ 3 ].st = Vec2( 1, 0 );

	for ( int i = 0; i < 4; i++ ) {
		cubeVerts[ 0 + i ].norm[ 0 ] = vert_t::FloatToByte_n11( 1 );
		cubeVerts[ 0 + i ].norm[ 1 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 0 + i ].norm[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 0 + i ].norm[ 3 ] = vert_t::FloatToByte_n11( 0 );

		cubeVerts[ 0 + i ].tang[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 0 + i ].tang[ 1 ] = vert_t::FloatToByte_n11( 1 );
		cubeVerts[ 0 + i ].tang[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 0 + i ].tang[ 3 ] = vert_t::FloatToByte_n11( 0 );
	}

	// -x
	cubeVerts[ 4 + 0 ].pos = Vec3(-1, 1, 1 );
	cubeVerts[ 4 + 1 ].pos = Vec3(-1, 1,-1 );
	cubeVerts[ 4 + 2 ].pos = Vec3(-1,-1,-1 );
	cubeVerts[ 4 + 3 ].pos = Vec3(-1,-1, 1 );

	cubeVerts[ 4 + 1 ].st = Vec2( 0, 0 );
	cubeVerts[ 4 + 2 ].st = Vec2( 0, 1 );
	cubeVerts[ 4 + 3 ].st = Vec2( 1, 1 );
	cubeVerts[ 4 + 4 ].st = Vec2( 1, 0 );

	for ( int i = 0; i < 4; i++ ) {
		cubeVerts[ 4 + i ].norm[ 0 ] = vert_t::FloatToByte_n11(-1 );
		cubeVerts[ 4 + i ].norm[ 1 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 4 + i ].norm[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 4 + i ].norm[ 3 ] = vert_t::FloatToByte_n11( 0 );

		cubeVerts[ 4 + i ].tang[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 4 + i ].tang[ 1 ] = vert_t::FloatToByte_n11(-1 );
		cubeVerts[ 4 + i ].tang[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 4 + i ].tang[ 3 ] = vert_t::FloatToByte_n11( 0 );
	}

	// +y
	cubeVerts[ 8 + 0 ].pos = Vec3( 1, 1, 1 );
	cubeVerts[ 8 + 1 ].pos = Vec3( 1, 1,-1 );
	cubeVerts[ 8 + 2 ].pos = Vec3(-1, 1,-1 );
	cubeVerts[ 8 + 3 ].pos = Vec3(-1, 1, 1 );

	cubeVerts[ 8 + 0 ].st = Vec2( 0, 0 );
	cubeVerts[ 8 + 1 ].st = Vec2( 0, 1 );
	cubeVerts[ 8 + 2 ].st = Vec2( 1, 1 );
	cubeVerts[ 8 + 3 ].st = Vec2( 1, 0 );

	for ( int i = 0; i < 4; i++ ) {
		cubeVerts[ 8 + i ].norm[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 8 + i ].norm[ 1 ] = vert_t::FloatToByte_n11( 1 );
		cubeVerts[ 8 + i ].norm[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 8 + i ].norm[ 3 ] = vert_t::FloatToByte_n11( 0 );

		cubeVerts[ 8 + i ].tang[ 0 ] = vert_t::FloatToByte_n11(-1 );
		cubeVerts[ 8 + i ].tang[ 1 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 8 + i ].tang[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 8 + i ].tang[ 3 ] = vert_t::FloatToByte_n11( 0 );
	}

	// -y
	cubeVerts[ 12 + 0 ].pos = Vec3(-1,-1, 1 );
	cubeVerts[ 12 + 1 ].pos = Vec3(-1,-1,-1 );
	cubeVerts[ 12 + 2 ].pos = Vec3( 1,-1,-1 );
	cubeVerts[ 12 + 3 ].pos = Vec3( 1,-1, 1 );

	cubeVerts[ 12 + 0 ].st = Vec2( 0, 0 );
	cubeVerts[ 12 + 1 ].st = Vec2( 0, 1 );
	cubeVerts[ 12 + 2 ].st = Vec2( 1, 1 );
	cubeVerts[ 12 + 3 ].st = Vec2( 1, 0 );

	for ( int i = 0; i < 4; i++ ) {
		cubeVerts[ 12 + i ].norm[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 12 + i ].norm[ 1 ] = vert_t::FloatToByte_n11(-1 );
		cubeVerts[ 12 + i ].norm[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 12 + i ].norm[ 3 ] = vert_t::FloatToByte_n11( 0 );

		cubeVerts[ 12 + i ].tang[ 0 ] = vert_t::FloatToByte_n11( 1 );
		cubeVerts[ 12 + i ].tang[ 1 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 12 + i ].tang[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 12 + i ].tang[ 3 ] = vert_t::FloatToByte_n11( 0 );
	}

	// +z
	cubeVerts[ 16 + 0 ].pos = Vec3(-1,-1, 1 );
	cubeVerts[ 16 + 1 ].pos = Vec3( 1,-1, 1 );
	cubeVerts[ 16 + 2 ].pos = Vec3( 1, 1, 1 );
	cubeVerts[ 16 + 3 ].pos = Vec3(-1, 1, 1 );

	cubeVerts[ 16 + 0 ].st = Vec2( 0, 0 );
	cubeVerts[ 16 + 1 ].st = Vec2( 0, 1 );
	cubeVerts[ 16 + 2 ].st = Vec2( 1, 1 );
	cubeVerts[ 16 + 3 ].st = Vec2( 1, 0 );

	for ( int i = 0; i < 4; i++ ) {
		cubeVerts[ 16 + i ].norm[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 16 + i ].norm[ 1 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 16 + i ].norm[ 2 ] = vert_t::FloatToByte_n11( 1 );
		cubeVerts[ 16 + i ].norm[ 3 ] = vert_t::FloatToByte_n11( 0 );

		cubeVerts[ 16 + i ].tang[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 16 + i ].tang[ 1 ] = vert_t::FloatToByte_n11( 1 );
		cubeVerts[ 16 + i ].tang[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 16 + i ].tang[ 3 ] = vert_t::FloatToByte_n11( 0 );
	}

	// -z
	cubeVerts[ 20 + 0 ].pos = Vec3( 1,-1,-1 );
	cubeVerts[ 20 + 1 ].pos = Vec3(-1,-1,-1 );
	cubeVerts[ 20 + 2 ].pos = Vec3(-1, 1,-1 );
	cubeVerts[ 20 + 3 ].pos = Vec3( 1, 1,-1 );

	cubeVerts[ 20 + 0 ].st = Vec2( 0, 0 );
	cubeVerts[ 20 + 1 ].st = Vec2( 0, 1 );
	cubeVerts[ 20 + 2 ].st = Vec2( 1, 1 );
	cubeVerts[ 20 + 3 ].st = Vec2( 1, 0 );

	for ( int i = 0; i < 4; i++ ) {
		cubeVerts[ 20 + i ].norm[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 20 + i ].norm[ 1 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 20 + i ].norm[ 2 ] = vert_t::FloatToByte_n11(-1 );
		cubeVerts[ 20 + i ].norm[ 3 ] = vert_t::FloatToByte_n11( 0 );

		cubeVerts[ 20 + i ].tang[ 0 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 20 + i ].tang[ 1 ] = vert_t::FloatToByte_n11( 1 );
		cubeVerts[ 20 + i ].tang[ 2 ] = vert_t::FloatToByte_n11( 0 );
		cubeVerts[ 20 + i ].tang[ 3 ] = vert_t::FloatToByte_n11( 0 );
	}

	for ( int face = 0; face < 6; face++ ) {
		cubeIdxs[ face * 6 + 0 ] = face * 4 + 0;
		cubeIdxs[ face * 6 + 1 ] = face * 4 + 1;
		cubeIdxs[ face * 6 + 2 ] = face * 4 + 2;

		cubeIdxs[ face * 6 + 3 ] = face * 4 + 0;
		cubeIdxs[ face * 6 + 4 ] = face * 4 + 2;
		cubeIdxs[ face * 6 + 5 ] = face * 4 + 3;
	}
#endif

	for ( int i = 0; i < numVerts; i++ ) {
		model.m_vertices.push_back( cubeVerts[ i ] );
	}

	for ( int i = 0; i < numIdxs; i++ ) {
		model.m_indices.push_back( cubeIdxs[ i ] );
	}
}

/*
====================================================
FillCubeTessellated
====================================================
*/
void FillCubeTessellated( Model & model, int numDivisions ) {
	if ( numDivisions < 1 ) {
		numDivisions = 1;
	}

	const int numnum = ( numDivisions + 1 ) * ( numDivisions + 1 );
	Vec3 * v = (Vec3 *)malloc( numnum * sizeof( Vec3 ) );
	Vec2 * st = (Vec2 *)malloc( numnum * sizeof( Vec2 ) );
	for ( int y = 0; y < numDivisions + 1; y++ ) {
		for ( int x = 0; x < numDivisions + 1; x++ ) {
			float xf = ( ( (float)x / (float)numDivisions ) - 0.5f ) * 2.0f;
			float yf = ( ( (float)y / (float)numDivisions ) - 0.5f ) * 2.0f;
			v[ y * ( numDivisions + 1 ) + x ] = Vec3( xf, yf, 1.0f );
			//v[ y * ( numDivisions + 1 ) + x ] = Vec3( 1.0f, xf, yf );	// Face the positive x direction

			float sf = (float)x / (float)numDivisions;
			float tf = (float)y / (float)numDivisions;
			st[ y * ( numDivisions + 1 ) + x ] = Vec2( sf, tf );
		}
	}

	const int numFaces = numDivisions * numDivisions;

	int faceIdx = 0;
	int * faceIdxs = (int *)malloc( 3 * 2 * numFaces * sizeof( int ) );
	for ( int y = 0; y < numDivisions; y++ ) {
		for ( int x = 0; x < numDivisions; x++ ) {
			int y0 = y;
			int y1 = y + 1;
			int x0 = x;
			int x1 = x + 1;

			faceIdxs[ faceIdx * 6 + 1 ] = y0 * ( numDivisions + 1 ) + x0;
			faceIdxs[ faceIdx * 6 + 0 ] = y1 * ( numDivisions + 1 ) + x0;
			faceIdxs[ faceIdx * 6 + 2 ] = y1 * ( numDivisions + 1 ) + x1;

			faceIdxs[ faceIdx * 6 + 4 ] = y0 * ( numDivisions + 1 ) + x0;
			faceIdxs[ faceIdx * 6 + 3 ] = y1 * ( numDivisions + 1 ) + x1;
			faceIdxs[ faceIdx * 6 + 5 ] = y0 * ( numDivisions + 1 ) + x1;

			faceIdx++;
		}
	}

	Mat3 matOrients[ 6 ];
	for ( int i = 0; i < 6; i++ ) {
		matOrients[ i ].Identity();
	}
	// px
	matOrients[ 0 ].rows[ 0 ] = Vec3( 0.0f, 0.0f, 1.0f );
	matOrients[ 0 ].rows[ 1 ] = Vec3( 1.0f, 0.0f, 0.0f );
	matOrients[ 0 ].rows[ 2 ] = Vec3( 0.0f, 1.0f, 0.0f );
	// nx
	matOrients[ 1 ].rows[ 0 ] = Vec3( 0.0f, 0.0f,-1.0f );
	matOrients[ 1 ].rows[ 1 ] = Vec3(-1.0f, 0.0f, 0.0f );
	matOrients[ 1 ].rows[ 2 ] = Vec3( 0.0f, 1.0f, 0.0f );

	// py
	matOrients[ 2 ].rows[ 0 ] = Vec3( 1.0f, 0.0f, 0.0f );
	matOrients[ 2 ].rows[ 1 ] = Vec3( 0.0f, 0.0f, 1.0f );
	matOrients[ 2 ].rows[ 2 ] = Vec3( 0.0f,-1.0f, 0.0f );
	// ny
	matOrients[ 3 ].rows[ 0 ] = Vec3( 1.0f, 0.0f, 0.0f );
	matOrients[ 3 ].rows[ 1 ] = Vec3( 0.0f, 0.0f,-1.0f );
	matOrients[ 3 ].rows[ 2 ] = Vec3( 0.0f, 1.0f, 0.0f );

	// pz
	matOrients[ 4 ].rows[ 0 ] = Vec3( 1.0f, 0.0f, 0.0f );
	matOrients[ 4 ].rows[ 1 ] = Vec3( 0.0f, 1.0f, 0.0f );
	matOrients[ 4 ].rows[ 2 ] = Vec3( 0.0f, 0.0f, 1.0f );
	// nz
	matOrients[ 5 ].rows[ 0 ] = Vec3(-1.0f, 0.0f, 0.0f );
	matOrients[ 5 ].rows[ 1 ] = Vec3( 0.0f, 1.0f, 0.0f );
	matOrients[ 5 ].rows[ 2 ] = Vec3( 0.0f, 0.0f,-1.0f );

	const int numIdxs = 3 * 2 * 6 * numFaces;
	const int numVerts = 4 * 6 * numFaces;
	vert_t *	cubeVerts = (vert_t *)malloc( numVerts * sizeof( vert_t ) );
	int *		cubeIdxs = (int *)malloc( numIdxs * sizeof( int ) );

	memset( cubeVerts, 0, sizeof( vert_t ) * numVerts );

	for ( int side = 0; side < 6; side++ ) {
		const Mat3 & mat = matOrients[ side ];

		const Vec3 tang = mat * Vec3( 1.0f, 0.0f, 0.0f );
		const Vec3 norm = mat * Vec3( 0.0f, 0.0f, 1.0f );

		for ( int vid = 0; vid < numnum; vid++ ) {
			const Vec3 xyz	= mat * v[ vid ];
			const Vec2 uv	= st[ vid ];

			cubeVerts[ side * numnum + vid ].pos[ 0 ] = xyz[ 0 ];
			cubeVerts[ side * numnum + vid ].pos[ 1 ] = xyz[ 1 ];
			cubeVerts[ side * numnum + vid ].pos[ 2 ] = xyz[ 2 ];

			cubeVerts[ side * numnum + vid ].st[ 0 ] = uv[ 0 ];
			cubeVerts[ side * numnum + vid ].st[ 1 ] = uv[ 1 ];

			cubeVerts[ side * numnum + vid ].norm[ 0 ] = vert_t::FloatToByte_n11( norm[ 0 ] );
			cubeVerts[ side * numnum + vid ].norm[ 1 ] = vert_t::FloatToByte_n11( norm[ 1 ] );
			cubeVerts[ side * numnum + vid ].norm[ 2 ] = vert_t::FloatToByte_n11( norm[ 2 ] );
			cubeVerts[ side * numnum + vid ].norm[ 3 ] = vert_t::FloatToByte_n11( 0.0f );

			cubeVerts[ side * numnum + vid ].tang[ 0 ] = vert_t::FloatToByte_n11( tang[ 0 ] );
			cubeVerts[ side * numnum + vid ].tang[ 1 ] = vert_t::FloatToByte_n11( tang[ 1 ] );
			cubeVerts[ side * numnum + vid ].tang[ 2 ] = vert_t::FloatToByte_n11( tang[ 2 ] );
			cubeVerts[ side * numnum + vid ].tang[ 3 ] = vert_t::FloatToByte_n11( 0.0f );
		}

		for ( int idx = 0; idx < 3 * 2 * numFaces; idx++ ) {
			const int offset = 3 * 2 * numFaces * side;
			cubeIdxs[ idx + offset ] = faceIdxs[ idx ] + numnum * side;
		}
	}

	for ( int i = 0; i < numVerts; i++ ) {
		model.m_vertices.push_back( cubeVerts[ i ] );
	}

	for ( int i = 0; i < numIdxs; i++ ) {
		model.m_indices.push_back( cubeIdxs[ i ] );
	}

	free( v );
	free( st );
	free( faceIdxs );
	free( cubeVerts );
	free( cubeIdxs );
}

/*
====================================================
FillSphere
====================================================
*/
void FillSphere( Model & model, const float radius ) {
	//FillCubeTessellated( model, 100 );
	float t = radius;
	if ( t < 0.0f ) {
		t = 0.0f;
	}
	if ( t > 100.0f ) {
		t = 100.0f;
	}
	t /= 100.0f;
	float min = 5;
	float max = 30;
	float s = min * ( 1.0f - t ) + max * t;
	FillCubeTessellated( model, (int)s );

	// Project the tessellated cube onto a sphere
	for ( int i = 0; i < model.m_vertices.size(); i++ ) {
		Vec3 xyz = model.m_vertices[ i ].pos;
		xyz.Normalize();

		model.m_vertices[ i ].pos[ 0 ] = xyz[ 0 ];
		model.m_vertices[ i ].pos[ 1 ] = xyz[ 1 ];
		model.m_vertices[ i ].pos[ 2 ] = xyz[ 2 ];

		model.m_vertices[ i ].norm[ 0 ] = vert_t::FloatToByte_n11( xyz[ 0 ] );
		model.m_vertices[ i ].norm[ 1 ] = vert_t::FloatToByte_n11( xyz[ 1 ] );
		model.m_vertices[ i ].norm[ 2 ] = vert_t::FloatToByte_n11( xyz[ 2 ] );
		model.m_vertices[ i ].norm[ 3 ] = vert_t::FloatToByte_n11( 0.0f );

		Vec3 tang = vert_t::Byte4ToVec3( model.m_vertices[ i ].norm );
		Vec3 bitang = xyz.Cross( tang );
		bitang.Normalize();
		tang = bitang.Cross( xyz );

		model.m_vertices[ i ].tang[ 0 ] = vert_t::FloatToByte_n11( tang[ 0 ] );
		model.m_vertices[ i ].tang[ 1 ] = vert_t::FloatToByte_n11( tang[ 1 ] );
		model.m_vertices[ i ].tang[ 2 ] = vert_t::FloatToByte_n11( tang[ 2 ] );
		model.m_vertices[ i ].tang[ 3 ] = vert_t::FloatToByte_n11( 0.0f );
	}
}

/*
====================================================
Model::BuildFromShape
====================================================
*/
bool Model::BuildFromShape( const Shape * shape ) {
	if ( NULL == shape ) {
		return false;
	}

	if ( shape->GetType() == Shape::SHAPE_BOX ) {
		const ShapeBox * shapeBox = (const ShapeBox *)shape;

		m_vertices.clear();
		m_indices.clear();

		//FillCube( *this );
		FillCubeTessellated( *this, 0 );
		Vec3 halfdim = ( shapeBox->m_bounds.maxs - shapeBox->m_bounds.mins ) * 0.5f;
		Vec3 center = ( shapeBox->m_bounds.maxs + shapeBox->m_bounds.mins ) * 0.5f;
		for ( int v = 0; v < m_vertices.size(); v++ ) {
			for ( int i = 0; i < 3; i++ ) {
				m_vertices[ v ].pos[ i ] *= halfdim[ i ];
				m_vertices[ v ].pos[ i ] += center[ i ];
			}
		}
	} else if ( shape->GetType() == Shape::SHAPE_SPHERE ) {
		const ShapeSphere * shapeSphere = (const ShapeSphere *)shape;

		m_vertices.clear();
		m_indices.clear();

		FillSphere( *this, shapeSphere->m_radius );
		for ( int v = 0; v < m_vertices.size(); v++ ) {
			for ( int i = 0; i < 3; i++ ) {
				m_vertices[ v ].pos[ i ] *= shapeSphere->m_radius;
			}
		}
	} else if ( shape->GetType() == Shape::SHAPE_CAPSULE ) {
		const ShapeCapsule * shapeCapsule = (const ShapeCapsule *)shape;

		m_vertices.clear();
		m_indices.clear();

		FillSphere( *this, shapeCapsule->m_radius );
		for ( int v = 0; v < m_vertices.size(); v++ ) {
			for ( int i = 0; i < 3; i++ ) {
				m_vertices[ v ].pos[ i ] *= shapeCapsule->m_radius;
			}
		}
		for ( int v = 0; v < m_vertices.size(); v++ ) {
			if ( m_vertices[ v ].pos[ 2 ] > 0.0f ) {
				m_vertices[ v ].pos[ 2 ] += shapeCapsule->m_height * 0.5f;
			}
			if ( m_vertices[ v ].pos[ 2 ] < 0.0f ) {
				m_vertices[ v ].pos[ 2 ] -= shapeCapsule->m_height * 0.5f;
			}
		}
	} else if ( shape->GetType() == Shape::SHAPE_CONVEX ) {
		const ShapeConvex * shapeConvex = (const ShapeConvex *)shape;

		m_vertices.clear();
		m_indices.clear();

		// Build the connected convex hull from the points
		std::vector< Vec3 > hullPts;
		std::vector< tri_t > hullTris;
		//void BuildConvexHull( const std::vector< Vec3 > & verts, std::vector< Vec3 > & hullPts, std::vector< tri_t > & hullTris );
		BuildConvexHull( shapeConvex->m_points, hullPts, hullTris );

		Bounds bounds;
		bounds.Expand( hullPts.data(), hullPts.size() );

#if 0
		// Calculate smoothed normals
		std::vector< Vec3 > normals;
		normals.reserve( hullPts.size() );
		for ( int i = 0; i < hullPts.size(); i++ ) {
			Vec3 norm( 0.0f );

			for ( int t = 0; t < hullTris.size(); t++ ) {
				const tri_t & tri = hullTris[ t ];
				if ( i != tri.a && i != tri.b && i != tri.c ) {
					continue;
				}

				const Vec3 & a = hullPts[ tri.a ];
				const Vec3 & b = hullPts[ tri.b ];
				const Vec3 & c = hullPts[ tri.c ];

				Vec3 ab = b - a;
				Vec3 ac = c - a;
				norm += ab.Cross( ac );
			}

			norm.Normalize();
			normals.push_back( norm );
		}

		m_vertices.reserve( hullPts.size() );
		for ( int i = 0; i < hullPts.size(); i++ ) {
			vert_t vert;
			memset( &vert, 0, sizeof( vert_t ) );
			
			vert.pos[ 0 ] = hullPts[ i ].x;
			vert.pos[ 1 ] = hullPts[ i ].y;
			vert.pos[ 2 ] = hullPts[ i ].z;

			vert.st[ 0 ] = ( vert.pos[ 0 ] - bounds.mins.x ) / bounds.WidthX();
			vert.st[ 1 ] = ( vert.pos[ 1 ] - bounds.mins.y ) / bounds.WidthY();
			if ( 0.0f * vert.st[ 0 ] != 0.0f * vert.st[ 0 ] ) {
				vert.st[ 0 ] = 0.0f;
			}
			if ( 0.0f * vert.st[ 1 ] != 0.0f * vert.st[ 1 ] ) {
				vert.st[ 1 ] = 0.0f;
			}

			Vec3 norm = normals[ i ];
			norm.Normalize();

			vert.norm[ 0 ] = FloatToByte_n11( norm[ 0 ] );
			vert.norm[ 1 ] = FloatToByte_n11( norm[ 1 ] );
			vert.norm[ 2 ] = FloatToByte_n11( norm[ 2 ] );
			vert.norm[ 3 ] = FloatToByte_n11( 0.0f );

			m_vertices.push_back( vert );
		}

		m_indices.reserve( hullTris.size() * 3 );
		for ( int i = 0; i < hullTris.size(); i++ ) {
			m_indices.push_back( hullTris[ i ].a );
			m_indices.push_back( hullTris[ i ].b );
			m_indices.push_back( hullTris[ i ].c );
		}
#else
		//
		//	Make each triangle unique (for texturing and face based normals)
		//
		m_vertices.reserve( hullTris.size() * 3 );
		for ( int t = 0; t < hullTris.size(); t++ ) {
			const tri_t & tri = hullTris[ t ];

			Vec3 pts[ 3 ];
			pts[ 0 ] = hullPts[ tri.a ];
			pts[ 1 ] = hullPts[ tri.b ];
			pts[ 2 ] = hullPts[ tri.c ];

			Vec3 ab = pts[ 1 ] - pts[ 0 ];
			Vec3 ac = pts[ 2 ] - pts[ 0 ];
			Vec3 norm = ab.Cross( ac );
			norm.Normalize();
			if ( !norm.IsValid() ) {
				norm = Vec3( 0, 0, 1 );
			}

			for ( int v = 0; v < 3; v++ ) {
				vert_t vert;
				memset( &vert, 0, sizeof( vert_t ) );
			
				vert.pos[ 0 ] = pts[ v ].x;
				vert.pos[ 1 ] = pts[ v ].y;
				vert.pos[ 2 ] = pts[ v ].z;

				Vec3 n;
				n.x = norm.x * norm.x;
				n.y = norm.y * norm.y;
				n.z = norm.z * norm.z;

				const float maxDist = 8.0f;	// 4 meters for the texture to repeat
				
				if ( n.z > n.x && n.z > n.y ) {
					vert.st[ 0 ] = ( vert.pos[ 0 ] - bounds.mins.x ) / bounds.WidthX();
					vert.st[ 1 ] = ( vert.pos[ 1 ] - bounds.mins.y ) / bounds.WidthY();

					vert.st[ 0 ] = ( vert.pos[ 0 ] - bounds.mins.x ) / maxDist;//bounds.WidthX();
					vert.st[ 1 ] = ( vert.pos[ 1 ] - bounds.mins.y ) / maxDist;//bounds.WidthY();
				} else if ( n.x > n.y && n.x > n.z ) {
					vert.st[ 0 ] = ( vert.pos[ 1 ] - bounds.mins.y ) / bounds.WidthY();
					vert.st[ 1 ] = ( vert.pos[ 2 ] - bounds.mins.z ) / bounds.WidthZ();

					vert.st[ 0 ] = ( vert.pos[ 1 ] - bounds.mins.y ) / maxDist;//bounds.WidthY();
					vert.st[ 1 ] = ( vert.pos[ 2 ] - bounds.mins.z ) / maxDist;//bounds.WidthZ();
				} else {
					vert.st[ 0 ] = ( vert.pos[ 2 ] - bounds.mins.z ) / bounds.WidthZ();
					vert.st[ 1 ] = ( vert.pos[ 0 ] - bounds.mins.x ) / bounds.WidthX();

					vert.st[ 0 ] = ( vert.pos[ 2 ] - bounds.mins.z ) / maxDist;//bounds.WidthZ();
					vert.st[ 1 ] = ( vert.pos[ 0 ] - bounds.mins.x ) / maxDist;//bounds.WidthX();
				}

				if ( 0.0f * vert.st[ 0 ] != 0.0f * vert.st[ 0 ] ) {
					vert.st[ 0 ] = 0.0f;
				}
				if ( 0.0f * vert.st[ 1 ] != 0.0f * vert.st[ 1 ] ) {
					vert.st[ 1 ] = 0.0f;
				}

				vert.norm[ 0 ] = vert_t::FloatToByte_n11( norm[ 0 ] );
				vert.norm[ 1 ] = vert_t::FloatToByte_n11( norm[ 1 ] );
				vert.norm[ 2 ] = vert_t::FloatToByte_n11( norm[ 2 ] );
				vert.norm[ 3 ] = vert_t::FloatToByte_n11( 0.0f );

				m_vertices.push_back( vert );
			}
		}

		m_indices.reserve( hullTris.size() * 3 );
		for ( int i = 0; i < hullTris.size() * 3; i++ ) {
			m_indices.push_back( i );
		}
#endif
	}

	return true;
}

/*
====================================================
Model::BuildFromCloth
====================================================
*/
bool Model::BuildFromCloth( float * verts, const int width, const int height, const int stride ) {
	const int num = width * height;
	m_indices.reserve( num * 3 );
	m_vertices.reserve( num );

	for ( int i = 0; i < num; i++ ) {
		vert_t vert;
		memset( &vert, 0, sizeof( vert ) );

		int idx = i * stride / 4;

		vert.pos[ 0 ] = verts[ idx + 0 ];
		vert.pos[ 1 ] = verts[ idx + 1 ];
		vert.pos[ 2 ] = verts[ idx + 2 ];

		int x = i % width;
		int y = i / height;
		vert.st[ 0 ] = float( x ) / float( width - 1 );
		vert.st[ 1 ] = float( y ) / float( height - 1 );

		vert.norm[ 0 ] = 0;
		vert.norm[ 1 ] = 255;
		vert.norm[ 2 ] = 0;
		vert.norm[ 3 ] = 0;

		vert.tang[ 0 ] = 0;
		vert.tang[ 1 ] = 0;
		vert.tang[ 2 ] = 255;
		vert.tang[ 3 ] = 0;

		m_vertices.push_back( vert );
	}

	for ( int y = 0; y < height - 1; y++ ) {
		for ( int x = 0; x < width - 1; x++ ) {
			int x1 = x + 1;
			int y1 = y + 1;

			int idx00 = x	+ y * width;
			int idx10 = x1	+ y * width;
			int idx01 = x	+ y1 * width;
			int idx11 = x1	+ y1 * width;

			m_indices.push_back( idx00 );
			m_indices.push_back( idx10 );
			m_indices.push_back( idx11 );

			m_indices.push_back( idx00 );
			m_indices.push_back( idx11 );
			m_indices.push_back( idx01 );


			m_indices.push_back( idx00 );
			m_indices.push_back( idx11 );
			m_indices.push_back( idx10 );

			m_indices.push_back( idx00 );
			m_indices.push_back( idx01 );
			m_indices.push_back( idx11 );
		}
	}

	return true;
}

/*
====================================================
Model::UpdateClothVerts
====================================================
*/
bool Model::BuildFromBrush( const brush_t * brush ) {
	for ( int i = 0; i < brush->numPlanes; i++ ) {
		const winding_t & winding = brush->windings[ i ];
		const plane_t & plane = brush->planes[ i ];
		Vec3 up = Vec3( 0, 0, 1 );
		if ( plane.normal.z * plane.normal.z > 0.9f ) {
			up = Vec3( 1, 0, 0 );
		}
		Vec3 tang = plane.normal.Cross( up );

		int idx0 = m_vertices.size();
		for ( int j = 0; j < winding.pts.size(); j++ ) {
			vert_t vert;
			memset( &vert, 0, sizeof( vert ) );

			vert.pos = winding.pts[ j ];
			// TODO: Load the ST values from the file
			// TODO: Use the st values to calculate the tangent vector
			vert_t::Vec3ToByte4( plane.normal, vert.norm );
			vert_t::Vec3ToByte4( tang, vert.tang );

			// Select the vertex color from the normal (this is for global illumination)
			vert.buff[ 0 ] = 0;
			vert.buff[ 1 ] = 0;
			vert.buff[ 2 ] = 0;
			vert.buff[ 3 ] = 255;
			float x2 = plane.normal.x * plane.normal.x;
			float y2 = plane.normal.y * plane.normal.y;
			float z2 = plane.normal.z * plane.normal.z;
			if ( z2 > x2 && z2 > y2 ) {
				vert.buff[ 2 ] = 255;
			} else if ( x2 > y2 && x2 > z2 ) {
				vert.buff[ 0 ] = 255;
			} else {
				vert.buff[ 1 ] = 255;
			}
			
			m_vertices.push_back( vert );

			int idxN = idx0 + j;
			int idxN1 = idx0 + ( ( j + 1 ) % winding.pts.size() );
			m_indices.push_back( idx0 );
			m_indices.push_back( idxN );
			m_indices.push_back( idxN1 );
		}
	}

	return true;
}

/*
====================================================
Model::UpdateClothVerts
====================================================
*/
void Model::UpdateClothVerts( DeviceContext & deviceContext, float * verts, const int width, const int height, const int stride ) {
	int num = width * height;

	VkCommandBuffer vkCommandBuffer = deviceContext.m_vkCommandBuffers[ 0 ];

	int bufferSize;

	for ( int i = 0; i < num; i++ ) {
		vert_t & vert = m_vertices[ i ];
		memset( &vert, 0, sizeof( vert ) );

		int idx = i * stride / 4;

		vert.pos[ 0 ] = verts[ idx + 0 ];
		vert.pos[ 1 ] = verts[ idx + 1 ];
		vert.pos[ 2 ] = verts[ idx + 2 ];
	}

	vert_t * vertsMapped = (vert_t *)m_vertexBuffer.MapBuffer( &deviceContext );
	for ( int i = 0; i < num; i++ ) {
		vertsMapped[ i ] = m_vertices[ i ];
	}
	m_vertexBuffer.UnmapBuffer( &deviceContext );
}
