//
//  BuildSubsurface.cpp
//
#include "Renderer/Common.h"
#include "Renderer/BuildSubsurface.h"
#include "Models/ModelManager.h"
#include "Models/ModelStatic.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Math/Math.h"
#include "Math/Sphere.h"
#include "Miscellaneous/Fileio.h"

#include <assert.h>
#include <stdio.h>
#include <vector>


Vec3 Transmittance( Vec3 beta, float ds ) {
	Vec3 t;
	t.x = exp( -beta.x * ds );
	t.y = exp( -beta.y * ds );
	t.z = exp( -beta.z * ds );
	return t;
}


// Perform single scattering on a sphere
void Scattering( Vec3 * data, int count, float radius ) {
	const float pi = acosf( -1.0f );
	const int numSamples = 100;

	const Sphere sphere = Sphere( Vec3( 0.0f ), radius );
	const Vec3 rayDir = Vec3( 1, 0, 0 );

	for ( int a = 0; a < count; a++ ) {
		float z = ( float( a ) / float( count - 1 ) ) * 2.0f - 1.0f;
//		float angle = pi * float( a ) / float( count - 1 );
		float angle = acosf( z );
		Vec3 accumulator = Vec3( 0.0f );

		for ( int i = 0; i < numSamples; i++ ) {
			float t = float( i ) / float( numSamples );
			float r = t * -radius + ( 1.0f - t ) * radius;

			float t1 = 0;
			float ds = 0;
			Vec3 rayStart = Vec3( cosf( angle ), sinf( angle ), 0.0f ) * r;
			hbIntersectRaySphere( rayStart, rayDir, sphere, t1, ds );	// This is overkill, TODO: optimize

			// Beta is the absorption parameter for different materials
			// TODO: make this is a parameter and calculate for various materials
			//Vec3 beta = Vec3( 10.127f, 50.27f, 200.5f );	// This seems to make a pretty good fleshy material
			Vec3 beta = Vec3( 100.0f, 10.0f, 50.0f );	// A nice Jade material
			//Vec3 beta = Vec3( 10.0f, 20.0f, 30.0f );	// Wax?
			//Vec3 beta = Vec3( 10.0f, 10.0f, 10.0f );	// Marble?
			Vec3 trans = Transmittance( beta, ds );

			float ds2 = radius - r;
			Vec3 trans2 = Transmittance( beta, ds2 );

			trans.x *= trans2.x;
			trans.y *= trans2.y;
			trans.z *= trans2.z;

			trans *= 1.0f / numSamples;

			accumulator += trans;
		}

		data[ a ] = accumulator;
	}
}

float CalculateT( float nearf, float farf, float invR ) {
	float invNear = 1.0f / nearf;
	float invFar = 1.0f / farf;
	float t = ( invR - invFar ) / ( invNear - invFar );
	return t;
}

float CalculateRadius( float nearf, float farf, float t ) {
	float invR = t * ( 1.0f / nearf ) + ( 1.0f - t ) * ( 1.0f / farf );
	float r = 1.0f / invR;
	float t2 = CalculateT( nearf, farf, invR );
	return r;
}

/*
====================================================
BuildSubsurfaceTable
====================================================
*/
void BuildSubsurfaceTable() {
	// 1mm to 10 meters on a log scale
// 	int count = 0;
// 	float radius = 0.001f;
// 	while ( radius < 100.0f ) {
// 		printf( "Radius: %i   %f\n", count, radius );
// 		radius *= 1.1f;
// 		count++;
// 	}
	const int countRadii = 256;
	const int countAngle = 256;

	const float minRadius = 0.005f;
	const float maxRadius = 100.0f;

	float r = 1;
	float z = ( maxRadius - r ) / ( maxRadius - minRadius );

	//Vec3 circle[ 128 ];
	Vec3 * table = (Vec3 *)malloc( sizeof( Vec3) * countRadii * countAngle );
	if ( NULL == table ) {
		return;
	}

	char strName[ 256 ];
	int idx = 0;
	sprintf_s( strName, 256, "generated/subsurface%i.ppm", idx );

	char strTmp[ 64 ];
	sprintf_s( strTmp, 64, "%i %i\n", countAngle, countRadii );

	std::string fileStr;
	fileStr = "P3\n";
	fileStr += strTmp;
	fileStr += "255\n";

	float radius = 0.001f;
	for ( int i = 0; i < countRadii; i++ ) {
		// i = 0, radius = maxRadius
		// i = 1, radius = minRadius;
		float t = float( i ) / float( countRadii - 1 );
		float radius = CalculateRadius( minRadius, maxRadius, t );

		//radius *= 1.1f;
		Vec3 * circle = &table[ i * countAngle ];
		Scattering( circle, countAngle, radius );

		for ( int j = 0; j < countAngle; j++ ) {
			Vec3 pt = circle[ j ] * 255.0f;

			char tmpStr[ 64 ];
			unsigned int r = (unsigned int)pt.x;
			unsigned int g = (unsigned int)pt.y;
			unsigned int b = (unsigned int)pt.z;
			sprintf_s( tmpStr, 64, "%u %u %u\n", r, g, b );
			fileStr += tmpStr;
		}
	}
	SaveFileData( strName, fileStr.c_str(), (unsigned int)fileStr.length() );

	subsurfaceHeader_t header;
	header.magic = SUBSURFACE_MAGIC;
	header.version = SUBSURFACE_VERSION;
	header.numRadii = countRadii;
	header.numAngles = countAngle;
	header.minRadius = 0.001f;
	header.maxRadius = radius;

	OpenFileWriteStream( "generated/subsurfaceJade.subsurface" );
	WriteFileStream( &header, sizeof( subsurfaceHeader_t ) );
	WriteFileStream( table, sizeof( Vec3 ) * countRadii * countAngle );
	CloseFileWriteStream();

	free( table );
}