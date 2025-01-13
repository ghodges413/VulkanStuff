//
//  BuildAtmosphere.cpp
//
#include "Renderer/Common.h"
#include "Renderer/BuildAtmosphere.h"
#include "Models/ModelShape.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Math/Sphere.h"
#include "Math/Math.h"
#include "Graphics/Targa.h"
#include "Miscellaneous/Fileio.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

template <class T>
T hbLerp( const T& a, const T& b, const float& t ) {
    return a * ( 1.0f - t ) + b * t;
}

template <class T>
const T & hbClamp( T & n, const T & mins, const T & maxs ) {
	assert( mins < maxs );

	if ( n < mins ) {
		n = mins;
	} else if ( n > maxs ) {
		n = maxs;
	}
	return n;
}




/*
==========================
ScatterPhaseFunctionRayleigh
// Equation 2 from Bruneton2008
==========================
*/
float ScatterPhaseFunctionRayleigh( const float cosTheta ) {
	const float pi = acosf( -1.0f );

	float phase = ( 3.0f / ( 16.0f * pi ) ) * ( 1.0f + cosTheta * cosTheta );
	return phase;
}

/*
==========================
ScatterPhaseFunctionMie
// Equation 4 from Bruneton2008
==========================
*/
float ScatterPhaseFunctionMie( const float cosTheta, const float mieG ) {
	const float pi = acosf( -1.0f );

	float g = mieG;
	float g2 = g * g;
	
	float phase = ( 3.0f / ( 8.0f * pi ) ) * ( 1.0f - g2 ) * ( 1.0f + cosTheta * cosTheta ) / ( ( 2.0f + g2 ) * powf( ( 1.0f + g2 - 2.0f * g * cosTheta ), 1.5f ) );
	return phase;
}


/*
========================================================================================================

4D Table Lookups

========================================================================================================
*/


/*
 ===============================
 GetRadius
 fragCoord is in the range [0, width], [0, height], [0, depth]
 ===============================
 */
float GetRadius( const int coord, const int dim, const float radiusTop, const float radiusGround ) {
	// ur = rho / H
	// H = sqrt( rt * rt - rg * rg )
	// rho = sqrt( r * r - rg * rg )

	// r = sqrt( rho * rho + rg * rg )
	// rho = ur * H
	// H = sqrt( rt * rt - rg * rg )
	float ur = float( coord ) / float( dim - 1 );
	const float H = sqrtf( radiusTop * radiusTop - radiusGround * radiusGround );
	const float rho = ur * H;
	float radius = sqrtf( rho * rho + radiusGround * radiusGround );
	hbClamp( radius, radiusGround + 0.01f, radiusTop - 0.001f );
	return radius;
}

/*
 ===============================
 GetCosAngleViewSun
 fragCoord is in the range [0, width], [0, height]
 ===============================
 */
float GetCosAngleViewSun( const int coord, const int dim ) {
	const float Unu = float( coord ) / float( dim - 1 );
	// Unu = ( 1 + nu ) / 2
	// nu = 2 * Unu - 1
	float nu = 2.0f * Unu - 1.0f;
	hbClamp( nu, -1.0f, 1.0f );
	return nu;
}
float GetCosAngleSun( const int coord, const int dim ) {
	const float Umus = ( float( coord ) + 0.5f ) / float( dim - 1 );
	// Umus = ( 1 - e( -3mus - 0.6 ) / ( 1 - e( -3.6 ) )
	// Umus * ( 1 - e( -3.6 ) ) = ( 1 - e( -3mus - 0.6 )
	// -3mus - 0.6 = log( 1 - Umus * ( 1 - e( -3.6 ) ) )

	float cosAngleSun = -( 0.6f + logf( 1.0f - Umus * ( 1.0f -  expf( -3.6f ) ) ) ) / 3.0f;
	hbClamp( cosAngleSun, -1.0f, 1.0f );
	return cosAngleSun;
}
float ClampCosAngleViewSun( const float cosAngleViewSun, const float cosAngleView, const float cosAngleSun ) {
	float sinAngleView = sqrtf( 1.0f - cosAngleView * cosAngleView );
	float sinAngleSun = sqrtf( 1.0f - cosAngleSun * cosAngleSun );
		
	// The angle between the sun and view need to range between the min and max angles away from
	// the view vector that the sun can possible be.  The min and max sun vectors will be co-planar with
	// the view vector.
	Vec3 view = Vec3( sinAngleView, 0.0f, cosAngleView );
	Vec3 sun0 = Vec3( sinAngleSun, 0.0f, cosAngleSun );
	Vec3 sun1 = Vec3( -sinAngleSun, 0.0f, cosAngleSun );
		
	// TODO: make sure these are the right limits (ie that sun0 really is the min and sun1 really is the max)
	float min = view.Dot( sun0 );
	float max = view.Dot( sun1 );
		
	// This probably isn't necessary
	if ( max < min ) {
		float tmp = min;
		min = max;
		max = tmp;
	}

	// Clamp the angle between the view and sun
	if ( cosAngleViewSun < min ) {
		return min;
	}
	if ( cosAngleViewSun > max ) {
		return max;
	}
	return cosAngleViewSun;
}
float GetCosAngleView( const int coord, const int dim, const float radius, const float radiusTop, const float radiusGround ) {
	const int dimHalf = dim >> 1;

	const float Umu = float( coord ) / float( dim - 1 );

	const float rho = sqrtf( radius * radius - radiusGround * radiusGround );
	const float H = sqrtf( radiusTop * radiusTop - radiusGround * radiusGround );
	
	float cosAngleView;
	if ( coord < dimHalf ) {
		const float beta = ( 2.0f * Umu - 1.0f ) * rho;
		cosAngleView = ( beta * beta + rho * rho ) / ( 2.0f * beta * radius );
    } else {
		const float beta = ( 2.0f * Umu - 1.0f ) * ( rho + H );
		cosAngleView = ( H * H - rho * rho - beta * beta ) / ( 2.0f * beta * radius );
    }

	hbClamp( cosAngleView, -1.0f, 1.0f );

	return cosAngleView;
}

/*
 ===============================
 GetCoords4D
 Uses the specialized coordinate look ups from the paper
 ===============================
 */
Vec4 GetCoords4D( const atmosData_t & data, const float radius, const float cosAngleView, const float cosAngleSun, const float cosAngleViewSun ) {
	const float radiusTop = data.radius_top;
	const float radiusGround = data.radius_ground;

	// Helper values
	const float r = ( radius > radiusGround ) ? radius : ( radiusGround + 0.01f );
	const float mu = cosAngleView;

	const float rmu = r * mu;
	const float H = sqrtf( radiusTop * radiusTop - radiusGround * radiusGround );
	const float rho = sqrtf( r * r - radiusGround * radiusGround );
	const float delta = rmu * rmu - rho * rho;

	// Coordinate lookups from the paper
	float uMu = 0.5f;
	if ( rmu < 0.0f && delta > 0.0f ) {
		uMu += ( rmu + sqrtf( delta ) ) / ( 2.0f * rho );
	} else {
		uMu -= ( rmu - sqrtf( delta + H * H ) ) / ( 2.0f * rho + 2.0f * H );
	}
	
	// Coordinate lookups from the paper
	const float uR = rho / H;
	const float uMuS = ( 1.0f - expf( -3.0f * cosAngleSun - 0.6f ) ) / ( 1.0f - expf( -3.6f ) );
	const float uNu = ( 1.0f + cosAngleViewSun ) * 0.5f;

	assert( uMuS == uMuS );
	assert( uNu == uNu );
	assert( uMu == uMu );
	assert( uR == uR );
	return Vec4( uMuS, uNu, uMu, uR );
}

Vec4 SampleTexture4DLinear( const Vec4 * texture, float s, float t, float r, float q, const int dimX, const int dimY, const int dimZ, const int dimW );

/*
 ===============================
 SampleScatter
 ===============================
 */
Vec4 SampleScatter( const atmosData_t & data, const float radius, const float cosAngleView, const float cosAngleSun, const float cosAngleViewSun, const Vec4 * table ) {
	const Vec4 coords4D = GetCoords4D( data, radius, cosAngleView, cosAngleSun, cosAngleViewSun );
	Vec4 scatter = SampleTexture4DLinear( table, coords4D.x, coords4D.y, coords4D.z, coords4D.w, data.scatter_dims.x, data.scatter_dims.y, data.scatter_dims.z, data.scatter_dims.w );
	return scatter;
}


/*
========================================================================================================

Texture Sampling

========================================================================================================
*/

/*
==========================
SampleTexture1D
==========================
*/
Vec4 SampleTexture1D( const Vec4 * texture, float s, const int dimX ) {
	hbClamp( s, 0.0f, 1.0f );

	int x = s * dimX;
	hbClamp( x, 0, dimX - 1 );
	return texture[ x ];
}
Vec4 SampleTexture1DLinear( const Vec4 * texture, float s, const int dimX ) {
	hbClamp( s, 0.0f, 1.0f );

	int x0 = s * dimX;
	hbClamp( x0, 0, dimX - 1 );
	const Vec4 sample0 = texture[ x0 ];

	int x1 = x0 + 1;
	hbClamp( x1, 0, dimX - 1 );
	const Vec4 sample1 = texture[ x1 ];

	const float fx = s * (float)dimX;
	const float fracX = fx - (int)fx;
	Vec4 sample = hbLerp( sample0, sample1, fracX );
	return sample;
}

/*
==========================
SampleTexture2D
==========================
*/
Vec4 SampleTexture2D( const Vec4 * texture, float s, float t, const int dimX, const int dimY ) {
	hbClamp( s, 0.0f, 1.0f );
	hbClamp( t, 0.0f, 1.0f );

	int x = s * dimX;
	int y = t * dimY;

	hbClamp( x, 0, dimX - 1 );
	hbClamp( y, 0, dimY - 1 );

	const int index = x + y * dimX;
	return texture[ index ];
}
Vec4 SampleTexture2DLinear( const Vec4 * texture, float s, float t, const int dimX, const int dimY ) {
	hbClamp( s, 0.0f, 1.0f );
	hbClamp( t, 0.0f, 1.0f );

	int x0 = s * dimX;
	int y0 = t * dimY;

	hbClamp( x0, 0, dimX - 1 );
	hbClamp( y0, 0, dimY - 1 );

	int x1 = x0 + 1;
	int y1 = y0 + 1;

	hbClamp( x1, 0, dimX - 1 );
	hbClamp( y1, 0, dimY - 1 );

	//
	// Get Samples
	//

	int indices[ 4 ];
	indices[ 0 ] = x0 + y0 * dimX;
	indices[ 1 ] = x0 + y1 * dimX;
	indices[ 2 ] = x1 + y0 * dimX;
	indices[ 3 ] = x1 + y1 * dimX;

	Vec4 samples[ 4 ];
	for ( int i = 0; i < 4; ++i ) {
		samples[ i ] = texture[ indices[ i ] ];
	}

	//
	//	Calculate weights
	//

	const float fx = s * (float)dimX;
	const float fy = t * (float)dimY;

	const float fracX = fx - (int)fx;
	const float fracY = fy - (int)fy;

	float weights[ 4 ];
	weights[ 0 ] = ( 1.0f - fracX ) * ( 1.0f - fracY );
	weights[ 1 ] = ( 1.0f - fracX ) * ( fracY );
	weights[ 2 ] = ( fracX ) * ( 1.0f - fracY );
	weights[ 3 ] = ( fracX ) * ( fracY );

	//
	//	Calculate final sample from weights
	//

	Vec4 finalSample = Vec4( 0 );
	for ( int i = 0; i < 4; ++i ) {
		finalSample += samples[ i ] * weights[ i ];
	}
	assert( finalSample.x == finalSample.x );
	assert( finalSample.y == finalSample.y );
	assert( finalSample.z == finalSample.z );
	assert( finalSample.w == finalSample.w );
	return finalSample;
}

/*
==========================
SampleTexture3D
==========================
*/
Vec4 SampleTexture3D( const Vec4 * texture, float s, float t, float r, const int dimX, const int dimY, const int dimZ ) {
	hbClamp( s, 0.0f, 1.0f );
	hbClamp( t, 0.0f, 1.0f );
	hbClamp( r, 0.0f, 1.0f );

	int x = s * dimX;
	int y = t * dimY;
	int z = r * dimZ;

	hbClamp( x, 0, dimX - 1 );
	hbClamp( y, 0, dimY - 1 );
	hbClamp( z, 0, dimZ - 1 );

	const int index = x + y * dimX + z * dimX * dimY;
	Vec4 sample = texture[ index ];
	return sample;
}
Vec4 SampleTexture3DLinear( const Vec4 * texture, float s, float t, float r, const int dimX, const int dimY, const int dimZ ) {
	hbClamp( s, 0.0f, 1.0f );
	hbClamp( t, 0.0f, 1.0f );
	hbClamp( r, 0.0f, 1.0f );

	int x0 = s * dimX;
	int y0 = t * dimY;
	int z0 = r * dimZ;

	hbClamp( x0, 0, dimX - 1 );
	hbClamp( y0, 0, dimY - 1 );
	hbClamp( z0, 0, dimZ - 1 );

	int x1 = x0 + 1;
	int y1 = y0 + 1;
	int z1 = z0 + 1;

	hbClamp( x1, 0, dimX - 1 );
	hbClamp( y1, 0, dimY - 1 );
	hbClamp( z1, 0, dimZ - 1 );

	//
	// Get Samples
	//

	int indices[ 8 ];
	indices[ 0 ] = x0 + y0 * dimX + z0 * dimX * dimY;
	indices[ 1 ] = x0 + y1 * dimX + z0 * dimX * dimY;
	indices[ 2 ] = x1 + y0 * dimX + z0 * dimX * dimY;
	indices[ 3 ] = x1 + y1 * dimX + z0 * dimX * dimY;
	indices[ 4 ] = x0 + y0 * dimX + z1 * dimX * dimY;
	indices[ 5 ] = x0 + y1 * dimX + z1 * dimX * dimY;
	indices[ 6 ] = x1 + y0 * dimX + z1 * dimX * dimY;
	indices[ 7 ] = x1 + y1 * dimX + z1 * dimX * dimY;

	Vec4 samples[ 8 ];
	for ( int i = 0; i < 8; ++i ) {
		samples[ i ] = texture[ indices[ i ] ];
	}

	//
	//	Calculate weights
	//

	const float fx = s * (float)dimX;
	const float fy = t * (float)dimY;
	const float fz = r * (float)dimZ;

	const float fracX = fx - (int)fx;
	const float fracY = fy - (int)fy;
	const float fracZ = fz - (int)fz;

	float weights[ 8 ];
	weights[ 0 ] = ( 1.0f - fracX ) * ( 1.0f - fracY ) * ( 1.0f - fracZ );
	weights[ 1 ] = ( 1.0f - fracX ) * ( fracY ) * ( 1.0f - fracZ );
	weights[ 2 ] = ( fracX ) * ( 1.0f - fracY ) * ( 1.0f - fracZ );
	weights[ 3 ] = ( fracX ) * ( fracY ) * ( 1.0f - fracZ );
	weights[ 4 ] = ( 1.0f - fracX ) * ( 1.0f - fracY ) * ( fracZ );
	weights[ 5 ] = ( 1.0f - fracX ) * ( fracY ) * ( fracZ );
	weights[ 6 ] = ( fracX ) * ( 1.0f - fracY ) * ( fracZ );
	weights[ 7 ] = ( fracX ) * ( fracY ) * ( fracZ );

	//
	//	Calculate final sample from weights
	//

	Vec4 finalSample = Vec4( 0 );
	for ( int i = 0; i < 8; ++i ) {
		finalSample += samples[ i ] * weights[ i ];
	}
	return finalSample;
}

/*
==========================
SampleTexture4D
==========================
*/
Vec4 SampleTexture4D( const Vec4 * texture, float s, float t, float r, float q, const int dimX, const int dimY, const int dimZ, const int dimW ) {
	hbClamp( s, 0.0f, 1.0f );
	hbClamp( t, 0.0f, 1.0f );
	hbClamp( r, 0.0f, 1.0f );
	hbClamp( q, 0.0f, 1.0f );

	int x = s * dimX;
	int y = t * dimY;
	int z = r * dimZ;
	int w = r * dimW;

	hbClamp( x, 0, dimX - 1 );
	hbClamp( y, 0, dimY - 1 );
	hbClamp( z, 0, dimZ - 1 );
	hbClamp( w, 0, dimW - 1 );

	const int index = x + y * dimX + z * dimX * dimY + w * dimX * dimY * dimZ;
	Vec4 sample = texture[ index ];
	return sample;
}
Vec4 SampleTexture4DLinear( const Vec4 * texture, float s, float t, float r, float q, const int dimX, const int dimY, const int dimZ, const int dimW ) {
	hbClamp( s, 0.0f, 1.0f );
	hbClamp( t, 0.0f, 1.0f );
	hbClamp( r, 0.0f, 1.0f );
	hbClamp( q, 0.0f, 1.0f );

	int x0 = s * dimX;
	int y0 = t * dimY;
	int z0 = r * dimZ;
	int w0 = q * dimW;

	hbClamp( x0, 0, dimX - 1 );
	hbClamp( y0, 0, dimY - 1 );
	hbClamp( z0, 0, dimZ - 1 );
	hbClamp( w0, 0, dimW - 1 );

	int x1 = x0 + 1;
	int y1 = y0 + 1;
	int z1 = z0 + 1;
	int w1 = w0 + 1;

	hbClamp( x1, 0, dimX - 1 );
	hbClamp( y1, 0, dimY - 1 );
	hbClamp( z1, 0, dimZ - 1 );
	hbClamp( w1, 0, dimW - 1 );

	//
	// Get Samples
	//

	int indices[ 16 ];
	indices[ 0 ] = x0 + y0 * dimX + z0 * dimX * dimY + w0 * dimX * dimY * dimZ;
	indices[ 1 ] = x0 + y1 * dimX + z0 * dimX * dimY + w0 * dimX * dimY * dimZ;
	indices[ 2 ] = x1 + y0 * dimX + z0 * dimX * dimY + w0 * dimX * dimY * dimZ;
	indices[ 3 ] = x1 + y1 * dimX + z0 * dimX * dimY + w0 * dimX * dimY * dimZ;
	indices[ 4 ] = x0 + y0 * dimX + z1 * dimX * dimY + w0 * dimX * dimY * dimZ;
	indices[ 5 ] = x0 + y1 * dimX + z1 * dimX * dimY + w0 * dimX * dimY * dimZ;
	indices[ 6 ] = x1 + y0 * dimX + z1 * dimX * dimY + w0 * dimX * dimY * dimZ;
	indices[ 7 ] = x1 + y1 * dimX + z1 * dimX * dimY + w0 * dimX * dimY * dimZ;

	indices[ 8 ] = x0 + y0 * dimX + z0 * dimX * dimY + w1 * dimX * dimY * dimZ;
	indices[ 9 ] = x0 + y1 * dimX + z0 * dimX * dimY + w1 * dimX * dimY * dimZ;
	indices[10 ] = x1 + y0 * dimX + z0 * dimX * dimY + w1 * dimX * dimY * dimZ;
	indices[11 ] = x1 + y1 * dimX + z0 * dimX * dimY + w1 * dimX * dimY * dimZ;
	indices[12 ] = x0 + y0 * dimX + z1 * dimX * dimY + w1 * dimX * dimY * dimZ;
	indices[13 ] = x0 + y1 * dimX + z1 * dimX * dimY + w1 * dimX * dimY * dimZ;
	indices[14 ] = x1 + y0 * dimX + z1 * dimX * dimY + w1 * dimX * dimY * dimZ;
	indices[15 ] = x1 + y1 * dimX + z1 * dimX * dimY + w1 * dimX * dimY * dimZ;

	Vec4 samples[ 16 ];
	for ( int i = 0; i < 16; ++i ) {
		samples[ i ] = texture[ indices[ i ] ];
	}

	//
	//	Calculate weights
	//

	const float fx = s * (float)dimX;
	const float fy = t * (float)dimY;
	const float fz = r * (float)dimZ;
	const float fw = q * (float)dimW;

	const float fracX = fx - (int)fx;
	const float fracY = fy - (int)fy;
	const float fracZ = fz - (int)fz;
	const float fracW = fw - (int)fw;

	float weights[ 16 ];
	weights[ 0 ] = ( 1.0f - fracX ) * ( 1.0f - fracY ) * ( 1.0f - fracZ ) * ( 1.0f - fracW );
	weights[ 1 ] = ( 1.0f - fracX ) * ( fracY ) * ( 1.0f - fracZ ) * ( 1.0f - fracW );
	weights[ 2 ] = ( fracX ) * ( 1.0f - fracY ) * ( 1.0f - fracZ ) * ( 1.0f - fracW );
	weights[ 3 ] = ( fracX ) * ( fracY ) * ( 1.0f - fracZ ) * ( 1.0f - fracW );
	weights[ 4 ] = ( 1.0f - fracX ) * ( 1.0f - fracY ) * ( fracZ ) * ( 1.0f - fracW );
	weights[ 5 ] = ( 1.0f - fracX ) * ( fracY ) * ( fracZ ) * ( 1.0f - fracW );
	weights[ 6 ] = ( fracX ) * ( 1.0f - fracY ) * ( fracZ ) * ( 1.0f - fracW );
	weights[ 7 ] = ( fracX ) * ( fracY ) * ( fracZ ) * ( 1.0f - fracW );

	weights[ 8 ] = ( 1.0f - fracX ) * ( 1.0f - fracY ) * ( 1.0f - fracZ ) * ( fracW );
	weights[ 9 ] = ( 1.0f - fracX ) * ( fracY ) * ( 1.0f - fracZ ) * ( fracW );
	weights[10 ] = ( fracX ) * ( 1.0f - fracY ) * ( 1.0f - fracZ ) * ( fracW );
	weights[11 ] = ( fracX ) * ( fracY ) * ( 1.0f - fracZ ) * ( fracW );
	weights[12 ] = ( 1.0f - fracX ) * ( 1.0f - fracY ) * ( fracZ ) * ( fracW );
	weights[13 ] = ( 1.0f - fracX ) * ( fracY ) * ( fracZ ) * ( fracW );
	weights[14 ] = ( fracX ) * ( 1.0f - fracY ) * ( fracZ ) * ( fracW );
	weights[15 ] = ( fracX ) * ( fracY ) * ( fracZ ) * ( fracW );

	//
	//	Calculate final sample from weights
	//

	Vec4 finalSample = Vec4( 0 );
	for ( int i = 0; i < 16; ++i ) {
		finalSample += samples[ i ] * weights[ i ];
	}
	return finalSample;
}



/*
========================================================================================================

Transmittance Table

========================================================================================================
*/

/*
 ===============================
 GetRadiusExtinction
 ===============================
 */
float GetRadiusExtinction( const int coord, const int dim, const float radiusTop, const float radiusGround ) {
	float t = float( coord ) / float( dim - 1 );
	float radius = radiusGround + t * ( radiusTop - radiusGround );
	return radius;
}

/*
 ===============================
 GetAngleExtinction
 ===============================
 */
float GetAngleExtinction( const int coord, const int dim ) {
	float t = float( coord ) / float( dim - 1 );
	float cosAngle = 2.0f * t - 1.0f;
	return cosAngle;
}

/*
 ===============================
 Transmittance
 // Returns the calculated transmittance for the height/angle.  Ignores ground intersections.
 ===============================
 */
Vec4 Transmittance( const atmosData_t & data, const float radius, const float cosAngle, const Vec4 * sampler ) {
	const float radiusGround = data.radius_ground;
	const float radiusTop = data.radius_top;

	Vec2 st;
	st.x = ( radius - radiusGround ) / ( radiusTop - radiusGround );
	st.y = 0.5f * ( cosAngle + 1.0f );

	Vec4 sample = SampleTexture2DLinear( sampler, st.x, st.y, data.transmittance_width, data.transmittance_height );
	assert( sample.x == sample.x );
	assert( sample.y == sample.y );
	assert( sample.z == sample.z );
	assert( sample.w == sample.w );
	return sample;
}
Vec4 Transmittance( const atmosData_t & data, Vec3 pos, Vec3 view, const Vec4 * sampler ) {
	const float radius = pos.GetMagnitude();
	pos.Normalize();
	view.Normalize();
	const float cosAngle = pos.Dot( view );
	return Transmittance( data, radius, cosAngle, sampler );
}


/*
 ===============================
 DoesCollideGround
 ===============================
 */
bool DoesCollideGround( const const Vec3 & pt, const Vec3 & ray, const float ground ) {
	const Sphere sphere = Sphere( Vec3( 0.0f ), ground );
	float t0 = -1;
	float t1 = -1;
	hbIntersectRaySphere( pt, ray, sphere, t0, t1 );
	if ( t0 > 0 || t1 > 0 ) {
		return true;
	}
	return false;
}

/*
 ===============================
 IntersectGroundTop
 // Intersect the ground or the top, whichever is nearest, and return the distance
 ===============================
 */
float IntersectGroundTop( const Vec3 & pt, const Vec3 & ray, const atmosData_t & data ) {
	const float radiusTop = data.radius_top;
	const float radiusGround = data.radius_ground;

	const Sphere sphereGround = Sphere( Vec3( 0.0f ), radiusGround );
	const Sphere sphereTop = Sphere( Vec3( 0.0f ), radiusTop + 1.0f );

	float tout;
	float t[ 4 ] = { -1 };
	hbIntersectRaySphere( pt, ray, sphereGround, t[ 0 ], t[ 1 ] );
	hbIntersectRaySphere( pt, ray, sphereTop, t[ 2 ], t[ 3 ] );
	if ( t[ 0 ] >= 0 && t[ 1 ] >= 0 ) {
		tout = Min( t[ 0 ], t[ 1 ] );
	} else {
		tout = t[ 3 ];
		// This shouldn't be necessary since we should always be inside the atmosphere during pre-compute
		if ( t[ 2 ] >= 0.0f ) {
			tout = Min( tout, t[ 2 ] );
		}
	}
	
    return tout;
}


/*
 ===============================
 ExtinctionOverPath
 ===============================
 */
Vec3 ExtinctionOverPath( const float scaleHeight, const Vec3 & pos, const Vec3 & view, const Vec3 & betaExtinction, const atmosData_t & data ) {
	// If we hit the planet, return an opaque value
	if ( DoesCollideGround( pos, view, data.radius_ground ) ) {
		float bigValue = 1e18;
		return Vec3( bigValue );
	}
	
	// Get the distance for this ray
	const float pathLength = IntersectGroundTop( pos, view, data );
	const float ds = pathLength / float( data.num_samples_extinction );

	const float radius = pos.GetMagnitude();

	// Calculate the initial sample
    float prevDensityRatio = expf( -( radius - data.radius_ground ) / scaleHeight );

	// Integrate over the path for the extinction
	float result = 0.0f;
    for ( int i = 1; i <= data.num_samples_extinction; i++ ) {
		// Get the path length
        float s = float( i ) * ds;
		
		// Accurately calculate the height at this location
		const Vec3 pt = pos + view * s;
		const float r = pt.GetMagnitude();

		// Get the density ratio
        float densityRatio = expf( -( r - data.radius_ground ) / scaleHeight );

		// Trapezoidal integration
        result += ( prevDensityRatio + densityRatio ) * 0.5f * ds;

		// Save off the density ratio for trapezoidal integration
        prevDensityRatio = densityRatio;
    }

	assert( result == result );
    return betaExtinction * result;
}


/*
====================================================
BuildTransmission
====================================================
*/
void BuildTransmission( const atmosData_t & parms, Vec4 * image ) {
	for ( int y = 0; y < parms.transmittance_height; y++ ) {
		float cosAngle = GetAngleExtinction( y, parms.transmittance_height );
		float sinAngle = sqrtf( 1.0f - cosAngle * cosAngle );	// get the sine of the angle
		const Vec3 view = Vec3( sinAngle, 0.0f, cosAngle );

		for ( int x = 0; x < parms.transmittance_width; x++ ) {
			float radius = GetRadiusExtinction( x, parms.transmittance_width, parms.radius_top, parms.radius_ground );
			const Vec3 pos = Vec3( 0, 0, radius );

			// Calculate the optical depth for rayleigh and mie
			Vec3 extinctionRayleigh = ExtinctionOverPath( parms.scale_height_rayleigh, pos, view, parms.beta_rayleigh_extinction, parms );
			Vec3 extinctionMie = ExtinctionOverPath( parms.scale_height_mie, pos, view, parms.beta_mie_extinction, parms );

			// Calculate the final extinction
			Vec4 finalExtinction( 0 );
			for ( int i = 0; i < 3; i++ ) {
				finalExtinction[ i ] = expf( -extinctionRayleigh[ i ] - extinctionMie[ i ] );
				assert( finalExtinction[ i ] == finalExtinction[ i ] );
			}
	
			// Store the final extinction into the transmittance table
			const int idx = parms.transmittance_width * y + x;
			image[ idx ] = finalExtinction;
		}
	}
}



/*
========================================================================================================

Irradiance Table

========================================================================================================
*/

/*
 ===============================
 GetRadiusIrradiance
 ===============================
 */
float GetRadiusIrradiance( const int coord, const int dim, const float radiusTop, const float radiusGround ) {
	float t = float( coord ) / float( dim - 1 );
	float radius = radiusGround + t * ( radiusTop - radiusGround );
	return radius;
}

/*
 ===============================
 GetAngleIrradiance
 ===============================
 */
float GetAngleIrradiance( const int coord, const int dim ) {
	float t = float( coord ) / float( dim - 1 );
	float cosAngle = 2.0f * t - 1.0f;
	return cosAngle;
}

/*
 ===============================
 Irradiance
 ===============================
 */
Vec4 Irradiance( const atmosData_t & data, const float radius, const float cosAngle, const Vec4 * sampler ) {
	const float radiusGround = data.radius_ground;
	const float radiusTop = data.radius_top;

	Vec2 st;
	st.x = ( radius - radiusGround ) / ( radiusTop - radiusGround );
	st.y = 0.5f * ( cosAngle + 1.0f );
    
	return SampleTexture2DLinear( sampler, st.x, st.y, data.irradiance_width, data.irradiance_height );
}




/*
 ===============================
 InitIrradiance
 ===============================
 */
void InitIrradiance( const atmosData_t & data, const Vec4 * transmittance, Vec4 * image ) {
	const float pi = acosf( -1.0f );
	const float invPi = 1.0f / pi;

	for ( int y = 0; y < data.irradiance_height; ++y ) {
		float cosAngle = GetAngleIrradiance( y, data.irradiance_height );

		for ( int x = 0; x < data.irradiance_width; ++x ) {
			float radius = GetRadiusIrradiance( x, data.irradiance_width, data.radius_top, data.radius_ground );
	
			// Lookup the transmittance for this height and solar direction
			Vec4 transmission = Transmittance( data, radius, cosAngle, transmittance );

			// Perform a dot product with the normal of the spherical ground
			Vec4 finalColor = transmission * Max( cosAngle, 0.0f );

			// Store the final color
			const int idx = data.irradiance_width * y + x;
			image[ idx ] = finalColor * invPi;
		}
	}
}

/*
 ===============================
 DeltaGroundIrradiance
 ===============================
 */
void DeltaGroundIrradiance( const atmosData_t & data, const Vec4 * deltaScatter, Vec4 * image ) {
	const float pi = acosf( -1.0f );
	const float invPi = 1.0f / pi;

	for ( int y = 0; y < data.irradiance_height; ++y ) {
		printf( "DeltaGroundIrradiance:  %i of [%i]\n", y, (int)data.irradiance_height );
		float cosAngleSun = GetAngleIrradiance( y, data.irradiance_height );

		for ( int x = 0; x < data.irradiance_width; ++x ) {
			float radius = GetRadiusIrradiance( x, data.irradiance_width, data.radius_top, data.radius_ground );
	
			// Calculate the ray to the sun
			float sinAngleSun = sqrtf( 1.0f - cosAngleSun * cosAngleSun );
			Vec3 sunRay = Vec3( sinAngleSun, 0.0f, cosAngleSun );

			float deltaTheta = 0.5f * pi / float( data.num_samples_irradiance );		// zenith angle (only integrate over half the sphere.. we only need the upper half for calculating ground irradiance)
			float deltaPhi = 2.0f * pi / float( data.num_samples_irradiance );		// azimuth angle

			//
			// Integrate over the entire view sphere for this height and solar angle ( equation 15 in Bruneton2008 )
			//
			Vec4 irradiance = Vec4( 0.0f );
			for ( int i = 0; i <= data.num_samples_irradiance; ++i ) {
				// Current azimuth angle for the view
				float phi = float( i ) * deltaPhi;
		
				for ( int j = 0; j <= data.num_samples_irradiance; ++j ) {
					// Current zenith angle for the view
					float theta = float( j ) * deltaTheta;
			
					// The solid angle for this part of the integral
					float dOmega = sinf( theta ) * deltaTheta * deltaPhi;

					// Calculate the view ray direction
					Vec3 raySample = Vec3( cosf( phi ) * sinf( theta ), sinf( phi ) * sinf( theta ), cosf( theta ) );
			
					// Calculate the angle between the ray to the sun and view direction ray
					float cosAngleViewSun = sunRay.Dot( raySample );

					// Lookup the change in the scattering
					Vec4 scatter = SampleScatter( data, radius, raySample.z, cosAngleSun, cosAngleViewSun, deltaScatter );
			
					// Sum the integral
					irradiance += scatter * raySample.z * dOmega;
				}
			}

			// Store the final color
			const int idx = data.irradiance_width * y + x;
			image[ idx ] = irradiance * invPi;
		}
	}
}



/*
========================================================================================================

Single Scattering

========================================================================================================
*/



/*
 ===============================
 GetViewAndSunRaysFromAngles
 ===============================
 */
void GetViewAndSunRaysFromAngles( const float cosAngleView, const float cosAngleSun, const float cosAngleViewSun, Vec3 & view, Vec3 & sun ) {
	float sinAngleView = sqrtf( 1.0f - cosAngleView * cosAngleView );	// get the sine of the angle
	float sinAngleSun = sqrtf( 1.0f - cosAngleSun * cosAngleSun );	// get the sine of the angle

	// Build a ray for the view
    view = Vec3( sinAngleView, 0.0f, cosAngleView );
	
	// Build a ray for the sun
	sun = Vec3( sinAngleSun, 0.0f, cosAngleSun );
	
	// If the view is directly up, then it doesn't matter if the sun is in the same plane... it's all symmetric.
	// In the event that the view isn't directly up, we need to calculate the 3D direction to the sun.
	if ( sinAngleView > 0.0f ) {
		// cosAngleViewSun is the dot product between the sun and view
		// cosAngleViewSun = sx * vx + sy * vy + sz + vz
		// = sx * vx + sz + vz
		// => sx * vx = cosAngleViewSun - sz * vz
		sun.x = ( cosAngleViewSun - sun.z * view.z ) / view.x;
		
		// x2 + y2 + z2 = 1.0
		// => y = sqrt( 1.0 - x2 - z2 )
		float ySqr = 1.0f - sun.x * sun.x - sun.z * sun.z;
		if ( ySqr > 0.0f ) {
			sun.y = sqrtf( ySqr );
		}
	}
}

/*
 ===============================
 TransmittanceBetweenPoints
 ===============================
 */
Vec4 TransmittanceBetweenPoints( const atmosData_t & data, Vec3 pt0, Vec3 pt1, const Vec4 * sampler ) {
	float radius0 = pt0.GetMagnitude();
	float radius1 = pt1.GetMagnitude();
	
	Vec3 ray = ( pt1 - pt0 ).Normalize();

	Vec3 zenith0 = pt0.Normalize();
	Vec3 zenith1 = pt1.Normalize();
	
	float cosAngle0 = zenith0.Dot( ray );
	float cosAngle1 = zenith1.Dot( ray );
	
	Vec4 transmission0 = Transmittance( data, radius0, cosAngle0, sampler );
	Vec4 transmission1 = Transmittance( data, radius1, cosAngle1, sampler );
	
	Vec4 result;
	for ( int i = 0; i < 4; ++i ) {
		result[ i ] = Min( transmission0[ i ] / transmission1[ i ], 1.0f );
		assert( result[ i ] == result[ i ] );
	}
    return result;
}

/*
 ===============================
 ScatterAtPointFromSunIntoView
 ===============================
 */
Vec3 ScatterAtPointFromSunIntoView( const atmosData_t & data, Vec3 pos, Vec3 pt, Vec3 sun, const float scaleHeight, const Vec4 * transmittance ) {
	const float radiusGround = data.radius_ground;
	
	// Get the extinction for the view and for the sun
	Vec4 extinctionView = TransmittanceBetweenPoints( data, pos, pt, transmittance );
	Vec4 extinctionSun = Transmittance( data, pt, sun, transmittance );

	// Multiply the extinction from the camera to this point and the extinction from this point to the sun
	Vec3 totalExtinction;
	for ( int i = 0; i < 3; ++i ) {
		totalExtinction[ i ] = extinctionView[ i ] * extinctionSun[ i ];
		assert( totalExtinction[ i ] == totalExtinction[ i ] );
	}

	const float radiusPoint = pt.GetMagnitude();
	
	// Calculate the scattering
	float heightPoint = radiusPoint - radiusGround;
	Vec3 scatter = totalExtinction * expf( -heightPoint / scaleHeight );
	return scatter;
}

/*
 ===============================
 CalcluateScattering
 ===============================
 */
Vec3 CalcluateScattering( const atmosData_t & data, Vec3 pos, Vec3 view, Vec3 sun, const Vec3 & betaScatter, const float scaleHeight, const Vec4 * transmittance ) {
	// Get the total path length and the delta path length
	const float pathLength = IntersectGroundTop( pos, view, data );
	const float ds = pathLength / float( data.num_samples_scatter );
	
	// Integrate over the path
	Vec3 scatter = Vec3( 0.0f );
	Vec3 prevScatter = Vec3( 0.0f );
    for ( int i = 0; i < data.num_samples_scatter; ++i ) {
		// Get the path length to this sample
        const float s = float( i ) * ds;
		
		// Accurately calculate the height at this location
		const Vec3 pt = pos + view * s;
		
		// Get the scattering over the path for this height and solar angle
		const Vec3 samplePointScatter = ScatterAtPointFromSunIntoView( data, pos, pt, sun, scaleHeight, transmittance );
		
		// Trapezoidal integration
        scatter += ( prevScatter + samplePointScatter ) * 0.5f * ds;
		
		// Store this iteration to the previous for trapezoidal integration
        prevScatter = samplePointScatter;
    }
	
	// Finalize the scattering
	for ( int i = 0; i < 3; ++i ) {
		scatter[ i ] *= betaScatter[ i ];
		assert( scatter[ i ] == scatter[ i ] );
	}
	return scatter;
}

/*
 ===============================
 SingleScattering
 ===============================
 */
void SingleScattering( const atmosData_t & data, const Vec4 * transmittance, Vec4 * image, Vec4 * deltaScatter ) {
	for ( int w = 0; w < data.scatter_dims.w; ++w ) {
		float radius = GetRadius( w, data.scatter_dims.w, data.radius_top, data.radius_ground );
		printf( "Single Scattering:  %i of [%i]\n", w, (int)data.scatter_dims.w );

		for ( int z = 0; z < data.scatter_dims.z; ++z ) {
			float cosAngleView = GetCosAngleView( z, data.scatter_dims.z, radius, data.radius_top, data.radius_ground );

			for ( int y = 0; y < data.scatter_dims.y; ++y ) {
				float cosAngleViewSun = GetCosAngleViewSun( y, data.scatter_dims.y );

				for ( int x = 0; x < data.scatter_dims.x; ++x ) {
					float cosAngleSun = GetCosAngleSun( x, data.scatter_dims.x );

					// Clamp the angle between the view and the sun.  Otherwise we get invalid values.
					// Obviously it's not physically possible for the angle between the view and sun to
					// be outside a range... dependent on the current view and sun vectors.
					// Really, these are wasted texels in the 4D texture anyway.  We could skip them,
					// but then we might have a problem with sampling border texels... as they'll look
					// up a linear interpolation with a valid value and a bad value.
					cosAngleViewSun = ClampCosAngleViewSun( cosAngleViewSun, cosAngleView, cosAngleSun );

					Vec3 view;
					Vec3 sun;
					GetViewAndSunRaysFromAngles( cosAngleView, cosAngleSun, cosAngleViewSun, view, sun );
					Vec3 pos = Vec3( 0, 0, radius );

					Vec3 scatterRayleigh = CalcluateScattering( data, pos, view, sun, data.beta_rayleigh_scatter, data.scale_height_rayleigh, transmittance );
					Vec3 scatterMie = CalcluateScattering( data, pos, view, sun, data.beta_mie_scatter, data.scale_height_mie, transmittance );
	
					// This is simple single scattering.  Likewise from Schafhitzel 2007, we are to defer the phase function to later.
					Vec4 finalScatter;
					finalScatter.x = scatterRayleigh.x;
					finalScatter.y = scatterRayleigh.y;
					finalScatter.z = scatterRayleigh.z;
					finalScatter.w = scatterMie.x;

					assert( finalScatter.x == finalScatter.x );
					assert( finalScatter.y == finalScatter.y );
					assert( finalScatter.z == finalScatter.z );
					assert( finalScatter.w == finalScatter.w );
	
					// Store off the scattering
					int idx = x;
					idx += data.scatter_dims.x * y;
					idx += data.scatter_dims.x * data.scatter_dims.y * z;
					idx += data.scatter_dims.x * data.scatter_dims.y * data.scatter_dims.z * w;
					image[ idx ] = finalScatter;

					// We need to also initialize the deltaScatter table... so that we can properly estimate the change in scattering between multi-scattering calculations.
					// Since we use the deltaScatter table to calculate the multi-scattering, and we need the phase function information then... we go ahead and include it
					// in our delta scatter calculation here.  This must be why Bruneton 2008 used a 4D texture to store the scattering information... multiple scattering requires it.
					for ( int n = 0; n < 3; ++n ) {
						const float deltaScatterRayleigh = finalScatter[ n ] * ScatterPhaseFunctionRayleigh( cosAngleViewSun );
						const float deltaScatterMie = finalScatter.w * ScatterPhaseFunctionMie( cosAngleViewSun, data.mie_g );
				
						finalScatter[ n ] = deltaScatterRayleigh + deltaScatterMie;
					}
					deltaScatter[ idx ] = finalScatter;
				}
			}
		}
	}
}



/*
 =============================================================================================

 DeltaLightIrradiance

 =============================================================================================
 */

/*
 ===============================
 CalculateInscatter
 ===============================
 */
Vec3 CalculateInscatter( const atmosData_t & data, Vec3 pos, Vec3 rayView, Vec3 raySun, const Vec4 * transmittance, const Vec4 * deltaIrradiance, const Vec4 * deltaScatterTable ) {
	const float radiusGround = data.radius_ground;
	const float radiusTop = data.radius_top;

	const float pi = acosf( -1.0f );
	float deltaTheta = pi / float( data.num_samples_scatter_spherical );	// zenith angle
	float deltatPhi = 2.0f * pi / float( data.num_samples_scatter_spherical );		// azimuth angle

    //
	//	Integrate over the sphere
	//
	Vec3 scatter = Vec3( 0.0f );
    for ( int i = 0; i <= data.num_samples_scatter_spherical; ++i ) {
		// Current zenith angle for the sample
		float theta = float( i ) * deltaTheta;
        float cosTheta = cosf( theta );
		float sinTheta = sinf( theta );

		// Integrate over the azimuth
        for ( int j = 0; j <= data.num_samples_scatter_spherical; ++j ) {
			// Current azimuth angle for the sample
			float phi = float( j ) * deltatPhi;
			
			// The solid angle for the sample
            float dOmega = deltaTheta * deltatPhi * sinTheta * 0.25f;
			
			// The ray pointing at the sample
            Vec3 raySample = Vec3( cosf( phi ) * sinTheta, sinf( phi ) * sinTheta, cosTheta );

			//
			// Calculate the light scattering into the sample from the ground
			//
			Vec3 groundScatter = Vec3( 0.0f );
			if ( DoesCollideGround( pos, raySample, radiusGround ) ) {
				// Get the distance to the ground
				float distanceToGround = IntersectGroundTop( pos, raySample, data );
				
				// Get the vector to the location of the ground that was hit
				Vec3 groundPos = pos + raySample * distanceToGround;
				
				// Get the extinction between here and the ground
				Vec4 groundTransmittance = TransmittanceBetweenPoints( data, pos, groundPos, transmittance );

				// Get the normal of the ground (spherical surface, so just normalize)
				Vec3 groundNormal = groundPos;
				groundNormal.Normalize();

				// Calculate the angle between the ground and the sun
				float cosAngleGroundSun = groundNormal.Dot( raySun );
				
				// Get the light coming off of the ground
				Vec4 groundIrradiance = Irradiance( data, radiusGround, cosAngleGroundSun, deltaIrradiance );
				
				// Mulitply the ground irradiance by the extinction from the ground to the view position
				for ( int n = 0; n < 3; ++n ) {
					groundScatter[ n ] = data.average_ground_reflectance * groundIrradiance[ n ] * groundTransmittance[ n ];
				}
			}
			
			//
			//	Calculate the deltaS term
			//
			
			const float cosAngleSunSample = raySun.Dot( raySample );
			const float cosAngleViewSample = rayView.Dot( raySample );

			// Lookup the scattering entering this point from this direction.
			const Vec4 scatterElementIn = SampleScatter( data, pos.GetMagnitude(), raySample.z, raySun.z, cosAngleSunSample, deltaScatterTable );

			//
			//	Calculate the final light entering our sample
			//
			
			// Get the height
			const float height = pos.GetMagnitude() - radiusGround;
			
			// Get the density ratios
			const float densityRatioRayleigh = expf( -height / data.scale_height_rayleigh );
			const float densityRatioMie = expf( -height / data.scale_height_mie );
			
			for ( int n = 0; n < 3; ++n ) {
				// Get the scattering
				const float scatterRayleigh = data.beta_rayleigh_scatter[ n ] * densityRatioRayleigh * ScatterPhaseFunctionRayleigh( cosAngleViewSample );
				const float scatterMie = data.beta_mie_scatter[ n ] * densityRatioMie * ScatterPhaseFunctionMie( cosAngleViewSample, data.mie_g );
			
				// Integrate
				scatter[ n ] += ( groundScatter[ n ] + scatterElementIn[ n ] ) * ( scatterRayleigh + scatterMie ) * dOmega;
				assert( scatter[ n ] == scatter[ n ] );
			}
        }
    }

	return scatter;
}

#define WINDOWS
#ifdef WINDOWS
#include <Windows.h>
#define ThreadReturnType_t DWORD WINAPI
//typedef DWORD WINAPI ThreadReturnType_t;
typedef LPVOID ThreadInputType_t;
#else
#include <pthread.h>
typedef void* ThreadReturnType_t;
typedef void* ThreadInputType_t;
#endif

#define NUM_THREADS 8

struct LightIrradianceData_t {
	atmosData_t data;
	const Vec4 * transmittance;
	const Vec4 * deltaIrradiance;
	const Vec4 * deltaScatter;
	Vec4 * image;
	int w;
};
ThreadReturnType_t InnerLightLoops( ThreadInputType_t threadData ) {
	LightIrradianceData_t * lightData = (LightIrradianceData_t *)threadData;
	const atmosData_t & data = lightData->data;
	const Vec4 * transmittance = lightData->transmittance;
	const Vec4 * deltaIrradiance = lightData->deltaIrradiance;
	const Vec4 * deltaScatter = lightData->deltaScatter;
	Vec4 * image = lightData->image;
	int w = lightData->w;

	float radius = GetRadius( w, data.scatter_dims.w, data.radius_top, data.radius_ground );
		
	for ( int z = 0; z < data.scatter_dims.z; ++z ) {
		float cosAngleView = GetCosAngleView( z, data.scatter_dims.z, radius, data.radius_top, data.radius_ground );

		for ( int y = 0; y < data.scatter_dims.y; ++y ) {
			float cosAngleViewSun = GetCosAngleViewSun( y, data.scatter_dims.y );

			for ( int x = 0; x < data.scatter_dims.x; ++x ) {
				float cosAngleSun = GetCosAngleSun( x, data.scatter_dims.x );

				// Clamp the angle between the view and the sun.  Otherwise we get invalid values.
				// Obviously it's not physically possible for the angle between the view and sun to
				// be outside a range... dependent on the current view and sun vectors.
				// Really, these are wasted texels in the 4D texture anyway.  We could skip them,
				// but then we might have a problem with sampling border texels... as they'll look
				// up a linear interpolation with a valid value and a bad value.
				cosAngleViewSun = ClampCosAngleViewSun( cosAngleViewSun, cosAngleView, cosAngleSun );

				//
				//	Build the rays of the View and to the Sun
				//
				Vec3 rayView;
				Vec3 raySun;
				GetViewAndSunRaysFromAngles( cosAngleView, cosAngleSun, cosAngleViewSun, rayView, raySun );
				Vec3 pos = Vec3( 0, 0, radius );
				
				Vec3 inscatter = CalculateInscatter( data, pos, rayView, raySun, transmittance, deltaIrradiance, deltaScatter );

				Vec4 finalScatter;
				finalScatter.x = inscatter.x;
				finalScatter.y = inscatter.y;
				finalScatter.z = inscatter.z;
				finalScatter.w = 0.0f;

				assert( finalScatter.x == finalScatter.x );
				assert( finalScatter.y == finalScatter.y );
				assert( finalScatter.z == finalScatter.z );

				// Store off the scattering
				int idx = x;
				idx += data.scatter_dims.x * y;
				idx += data.scatter_dims.x * data.scatter_dims.y * z;
				idx += data.scatter_dims.x * data.scatter_dims.y * data.scatter_dims.z * w;
				image[ idx ] = finalScatter;
			}
		}
	}
	return 0;
}

void DeltaLightIrradianceMT( const atmosData_t & data, const Vec4 * transmittance, const Vec4 * deltaIrradiance, const Vec4 * deltaScatter, Vec4 * image ) {
#ifdef WINDOWS
	LightIrradianceData_t lightData[ NUM_THREADS ];
	HANDLE hThreadArray[ NUM_THREADS ];
	DWORD dwThreadIdArray[ NUM_THREADS ];
#else
	pthread_t threads[ NUM_THREADS ];
    LightIrradianceData_t data[ NUM_THREADS ];
    int rc = 0;
#endif
	for ( int w = 0; w < data.scatter_dims.w; w += NUM_THREADS ) {
		printf( "DeltaLightIrradiance:  %i of [%i]\n", w, (int)data.scatter_dims.w );

		for ( int i = 0; i < NUM_THREADS; ++i ) {
				lightData[ i ].data = data;
				lightData[ i ].transmittance = transmittance;
				lightData[ i ].deltaIrradiance = deltaIrradiance;
				lightData[ i ].deltaScatter = deltaScatter;
				lightData[ i ].image = image;
				lightData[ i ].w = w + i;

#ifdef WINDOWS
				hThreadArray[ i ] = CreateThread( 
										NULL,						// default security attributes
										0,							// use default stack size  
										InnerLightLoops,			// thread function name
										lightData + i,					// argument to thread function 
										0,							// use default creation flags 
										&dwThreadIdArray[ i ] );	// returns the thread identifier
				assert( hThreadArray[ i ] != NULL );
#else
				rc = pthread_create( &threads[ i ], NULL, ThreadFunctionWide, (void *) &lightData[ i ] );
				assert( 0 == rc );
#endif
		}

#ifdef WINDOWS
		// Wait until all threads have terminated.
		WaitForMultipleObjects( NUM_THREADS, hThreadArray, TRUE, INFINITE );

		for ( int i = 0; i < NUM_THREADS; ++i ) {
			CloseHandle( hThreadArray[ i ] );
		}
#else
		// wait for each thread to complete
		for ( i = 0; i < NUM_THREADS; ++i ) {
			// block until thread i completes
			rc = pthread_join( threads[ i ], NULL );
			assert( 0 == rc );
		}
#endif
	}
}

/*
 ===============================
 DeltaLightIrradiance
 ===============================
 */
void DeltaLightIrradiance( const atmosData_t & data, const Vec4 * transmittance, const Vec4 * deltaIrradiance, const Vec4 * deltaScatter, Vec4 * image ) {
	for ( int w = 0; w < data.scatter_dims.w; ++w ) {
		float radius = GetRadius( w, data.scatter_dims.w, data.radius_top, data.radius_ground );
		printf( "DeltaLightIrradiance:  %i of [%i]\n", w, (int)data.scatter_dims.w );

		for ( int z = 0; z < data.scatter_dims.z; ++z ) {
			float cosAngleView = GetCosAngleView( z, data.scatter_dims.z, radius, data.radius_top, data.radius_ground );

			for ( int y = 0; y < data.scatter_dims.y; ++y ) {
				float cosAngleViewSun = GetCosAngleViewSun( y, data.scatter_dims.y );

				for ( int x = 0; x < data.scatter_dims.x; ++x ) {
					float cosAngleSun = GetCosAngleSun( x, data.scatter_dims.x );

					// Clamp the angle between the view and the sun.  Otherwise we get invalid values.
					// Obviously it's not physically possible for the angle between the view and sun to
					// be outside a range... dependent on the current view and sun vectors.
					// Really, these are wasted texels in the 4D texture anyway.  We could skip them,
					// but then we might have a problem with sampling border texels... as they'll look
					// up a linear interpolation with a valid value and a bad value.
					cosAngleViewSun = ClampCosAngleViewSun( cosAngleViewSun, cosAngleView, cosAngleSun );

					//
					//	Build the rays of the View and to the Sun
					//
					Vec3 rayView;
					Vec3 raySun;
					GetViewAndSunRaysFromAngles( cosAngleView, cosAngleSun, cosAngleViewSun, rayView, raySun );
					Vec3 pos = Vec3( 0, 0, radius );
				
					Vec3 inscatter = CalculateInscatter( data, pos, rayView, raySun, transmittance, deltaIrradiance, deltaScatter );

					Vec4 finalScatter;
					finalScatter.x = inscatter.x;
					finalScatter.y = inscatter.y;
					finalScatter.z = inscatter.z;
					finalScatter.w = 0.0f;

					assert( finalScatter.x == finalScatter.x );
					assert( finalScatter.y == finalScatter.y );
					assert( finalScatter.z == finalScatter.z );

					// Store off the scattering
					int idx = x;
					idx += data.scatter_dims.x * y;
					idx += data.scatter_dims.x * data.scatter_dims.y * z;
					idx += data.scatter_dims.x * data.scatter_dims.y * data.scatter_dims.z * w;
					image[ idx ] = finalScatter;
				}
			}
		}
	}
}

/*
 =============================================================================================

 CalculateInscatter

 =============================================================================================
 */

/*
 ===============================
 CalculateScatterAtPoint
 ===============================
 */
Vec4 CalculateScatterAtPoint( const atmosData_t & data, Vec3 pos, Vec3 view, Vec3 sun, float s, const Vec4 * transmittance, const Vec4 * deltaLightScatterTable ) {
	// Get the next position
	Vec3 pt = pos + view * s;
	float radiusScatterPoint = pt.GetMagnitude();

	// Get the view angle at the next position
	float cosAngleViewScatterPoint = pt.Dot( view );

	// Get the sun angle at the next position
	float cosAngleSunScatterPoint = pt.Dot( sun );

	// Get the angle between the view and sun
	float cosAngleSunView = view.Dot( sun );
	
	// Lookup the change in the light scattering
	Vec4 deltaLightScatter = SampleScatter( data, radiusScatterPoint, cosAngleViewScatterPoint, cosAngleSunScatterPoint, cosAngleSunView, deltaLightScatterTable );

	// Look up the extinction between these two points
	Vec4 transmission = TransmittanceBetweenPoints( data, pos, pt, transmittance );
	
	// Return the amount of light scattered into the position
	Vec4 scatter;
	for ( int i = 0; i < 4; ++i ) {
		scatter[ i ] = transmission[ i ] * deltaLightScatter[ i ];
		assert( scatter[ i ] == scatter[ i ] );
	}
    return scatter;
}

/*
 ===============================
 DeltaScatter
 ===============================
 */
void DeltaScatter( const atmosData_t & data, const Vec4 * transmittance, const Vec4 * deltaLightScatterTable, Vec4 * image ) {
	for ( int w = 0; w < data.scatter_dims.w; ++w ) {
		float radius = GetRadius( w, data.scatter_dims.w, data.radius_top, data.radius_ground );
		printf( "DeltaScatter:  %i of [%i]\n", w, (int)data.scatter_dims.w );

		for ( int z = 0; z < data.scatter_dims.z; ++z ) {
			float cosAngleView = GetCosAngleView( z, data.scatter_dims.z, radius, data.radius_top, data.radius_ground );

			for ( int y = 0; y < data.scatter_dims.y; ++y ) {
				float cosAngleViewSun = GetCosAngleViewSun( y, data.scatter_dims.y );

				for ( int x = 0; x < data.scatter_dims.x; ++x ) {
					float cosAngleSun = GetCosAngleSun( x, data.scatter_dims.x );

					// Clamp the angle between the view and the sun.  Otherwise we get invalid values.
					// Obviously it's not physically possible for the angle between the view and sun to
					// be outside a range... dependent on the current view and sun vectors.
					// Really, these are wasted texels in the 4D texture anyway.  We could skip them,
					// but then we might have a problem with sampling border texels... as they'll look
					// up a linear interpolation with a valid value and a bad value.
					cosAngleViewSun = ClampCosAngleViewSun( cosAngleViewSun, cosAngleView, cosAngleSun );

					
					Vec3 view;
					Vec3 sun;
					GetViewAndSunRaysFromAngles( cosAngleView, cosAngleSun, cosAngleViewSun, view, sun );
					Vec3 pos = Vec3( 0, 0, radius );

					// Get the total path length and the delta path length
					const float pathLength = IntersectGroundTop( pos, view, data );
					const float ds = pathLength / float( data.num_samples_scatter );
	
					// Get the initial scatering for trapezoidal integration
					Vec4 prevScatter = CalculateScatterAtPoint( data, pos, view, sun, 0.0f, transmittance, deltaLightScatterTable );
	
					// Integrate over the path to find all the new light scattering into this position
					Vec4 scatter = Vec4( 0.0f );
					for ( int i = 1; i <= data.num_samples_scatter; ++i ) {
						// Get the path length for this sample
						float s = float( i ) * ds;
		
						// Get the change in the inscattering at this point
						Vec4 samplePointScatter = CalculateScatterAtPoint( data, pos, view, sun, s, transmittance, deltaLightScatterTable );
		
						// Trapezoidal integration
						scatter += ( prevScatter + samplePointScatter ) * 0.5f * ds;

						// Store off this sample for trapezoidal integration
						prevScatter = samplePointScatter;
					}

					Vec4 finalScatter;
					finalScatter.x = scatter.x;
					finalScatter.y = scatter.y;
					finalScatter.z = scatter.z;
					finalScatter.w = 0.0f;

					assert( finalScatter.x == finalScatter.x );
					assert( finalScatter.y == finalScatter.y );
					assert( finalScatter.z == finalScatter.z );

					// Store off the scattering
					int idx = x;
					idx += data.scatter_dims.x * y;
					idx += data.scatter_dims.x * data.scatter_dims.y * z;
					idx += data.scatter_dims.x * data.scatter_dims.y * data.scatter_dims.z * w;
					image[ idx ] = finalScatter;
				}
			}
		}
	}
}







/*
====================================================
BuildAtmosphereTables
====================================================
*/
void BuildAtmosphereTables( const atmosData_t & data ) {
	const int scatterTableSize = data.scatter_r_size * data.scatter_mu_size * data.scatter_mu_s_size * data.scatter_nu_size;

	// Create the tables for holding the final data
	Vec4 * transmissionTable = new Vec4[ data.transmittance_width * data.transmittance_height ];	// Transmittance of atmosphere
	Vec4 * irradianceTable = new Vec4[ data.irradiance_width * data.irradiance_height ];			// Ground irradiance
	Vec4 * scatterTable = new Vec4[ scatterTableSize ];												// In scattering of light

	// Temporary tables for calculating the multi-scattering
	Vec4 * deltaIrradiance = new Vec4[ data.irradiance_width * data.irradiance_height ];
	Vec4 * deltaScatter = new Vec4[ scatterTableSize ];
	Vec4 * deltaLightTable = new Vec4[ scatterTableSize ];

	printf( "Building atmosphere\n" );
	printf( "Building atmosphere - transmision\n" );

	// Build the transmission table... same as single scattering
	BuildTransmission( data, transmissionTable );

	// Calculate the initial ground ambience from the extinction of sunlight through the atmosphere
	// (only do this to the temporary irradiance table... we'll do a direct lighting
	// calculation at render-time, and we don't want it in the pre-calculated table)
	printf( "Building atmosphere - irradiance\n" );
	InitIrradiance( data, transmissionTable, deltaIrradiance );

	// Calculate the single scattering from the sun
	printf( "Building atmosphere - single scattering\n" );
	SingleScattering( data, transmissionTable, scatterTable, deltaScatter );
	
	// Perform the multi-scattering
	for ( int i = 0; i <= 6; ++i ) {
		printf( "Building atmosphere - multi-scattering %i of 6\n", i );

		DeltaLightIrradiance( data, transmissionTable, deltaIrradiance, deltaScatter, deltaLightTable );
		//DeltaLightIrradianceMT( data, transmissionTable, deltaIrradiance, deltaScatter, deltaLightTable );

		DeltaGroundIrradiance( data, deltaScatter, deltaIrradiance );

		DeltaScatter( data, transmissionTable, deltaLightTable, deltaScatter );

		// Add the change in ground irradiance into the irradiance table
		for ( int j = 0; j < data.irradiance_width * data.irradiance_height; ++j ) {
			irradianceTable[ j ] += deltaIrradiance[ j ];
		}

		// Add the change in scattering into the scattering table
		for ( int j = 0; j < scatterTableSize; ++j ) {
			scatterTable[ j ] += deltaLightTable[ j ];
		}
	}

	printf( "Saving atmosphere\n" );



	//
	//	Save the transmission table
	//
	int numPixels;
	char strTmp[ 64 ];
	char strName[ 256 ];
	std::string fileStr;
		
	sprintf_s( strName, 256, "generated/atmos_transmittance.ppm" );
	sprintf_s( strTmp, 64, "%i %i\n", data.transmittance_width, data.transmittance_height );

	fileStr = "P3\n";
	fileStr += strTmp;
	fileStr += "255\n";

	numPixels = data.transmittance_width * data.transmittance_height;
	for ( int i = 0; i < numPixels; i++ ) {
		Vec4 pt = transmissionTable[ i ] * 255.0f;

		char tmpStr[ 64 ];
		unsigned int r = (unsigned int)pt.x;
		unsigned int g = (unsigned int)pt.y;
		unsigned int b = (unsigned int)pt.z;
		sprintf_s( tmpStr, 64, "%u %u %u\n", r, g, b );
		fileStr += tmpStr;
	}
	SaveFileData( strName, fileStr.c_str(), (unsigned int)fileStr.length() );

	//
	//	Save the irradiance table
	//
	sprintf_s( strName, 256, "generated/atmos_irradiance.ppm" );
	sprintf_s( strTmp, 64, "%i %i\n", data.irradiance_width, data.irradiance_height );

	fileStr = "P3\n";
	fileStr += strTmp;
	fileStr += "255\n";

	numPixels = data.irradiance_width * data.irradiance_height;
	for ( int i = 0; i < numPixels; i++ ) {
		Vec4 pt = transmissionTable[ i ] * 255.0f;

		char tmpStr[ 64 ];
		unsigned int r = (unsigned int)pt.x;
		unsigned int g = (unsigned int)pt.y;
		unsigned int b = (unsigned int)pt.z;
		sprintf_s( tmpStr, 64, "%u %u %u\n", r, g, b );
		fileStr += tmpStr;
	}
	SaveFileData( strName, fileStr.c_str(), (unsigned int)fileStr.length() );

	//
	//	Save the scatter tables
	//
	for ( int z = 0; z < data.scatter_r_size; z++ ) {
		int width = data.scatter_mu_s_size * data.scatter_nu_size;
		int height = data.scatter_mu_size;
		sprintf_s( strName, 256, "generated/atmos_scatter_%i.ppm", z );
		sprintf_s( strTmp, 64, "%i %i\n", width, height );

		fileStr = "P3\n";
		fileStr += strTmp;
		fileStr += "255\n";

		numPixels = width * height;
		for ( int i = 0; i < numPixels; i++ ) {
			Vec4 * data4d_ptr = scatterTable + width * height * z;
			Vec4 pt = data4d_ptr[ i ] * 255.0f;

			char tmpStr[ 64 ];
			unsigned int r = (unsigned int)pt.x;
			unsigned int g = (unsigned int)pt.y;
			unsigned int b = (unsigned int)pt.z;
			sprintf_s( tmpStr, 64, "%u %u %u\n", r, g, b );
			fileStr += tmpStr;
		}
		SaveFileData( strName, fileStr.c_str(), (unsigned int)fileStr.length() );
	}
	
	//
	//	Save the atmosphere tables into one big file
	//
	atmosHeader_t header;
	header.magic = ATMOS_MAGIC;
	header.version = ATMOS_VERSION;
	header.data = data;

	OpenFileWriteStream( "generated/atmosphere.atmos" );
	WriteFileStream( &header, sizeof( atmosHeader_t ) );
	WriteFileStream( transmissionTable, sizeof( Vec4 ) * data.transmittance_width * data.transmittance_height );
	WriteFileStream( irradianceTable, sizeof( Vec4 ) * data.irradiance_width * data.irradiance_height );
	WriteFileStream( scatterTable, sizeof( Vec4 ) * data.scatter_r_size * data.scatter_mu_size * data.scatter_mu_s_size * data.scatter_nu_size );
	CloseFileWriteStream();

	// Delete the tables for holding the final data
	delete[] transmissionTable;
	delete[] irradianceTable;
	delete[] scatterTable;

	// Delete temporary tables for calculating the multi-scattering
	delete[] deltaIrradiance;
	delete[] deltaScatter;
	delete[] deltaLightTable;
}
