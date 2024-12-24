//
//	Frustum.cpp
//
#include "Math/Frustum.h"
#include "Math/MatrixOps.h"
#include "Math/Quat.h"
#include "Math/Math.h"

/*
 ================================
 Frustum::Frustum
 ================================
 */
Frustum::Frustum() :
m_isInitialized( false ) {
}

/*
 ================================
 Frustum::Frustum
 ================================
 */
Frustum::Frustum( const Frustum & rhs ) :
m_isInitialized( rhs.m_isInitialized ) {
    for ( int i = 0; i < 6; ++i ) {
        m_planes[ i ] = rhs.m_planes[ i ];
    }
	for ( int i = 0; i < 8; ++i ) {
        m_corners[ i ] = rhs.m_corners[ i ];
    }
	m_bounds = rhs.m_bounds;
}

/*
 ================================
 Frustum::Frustum
 ================================
 */
Frustum::Frustum( const float & nearz,
                             const float & farz,
                             const float & fovy_degrees,
                             const float & screenWidth,
                             const float & screenHeight,
                             const Vec3 & camPos,
                             const Vec3 & camUp,
                             const Vec3 & camLook ) {
    Build( nearz, farz, fovy_degrees, screenWidth, screenHeight, camPos, camUp, camLook );
    m_isInitialized = true;
}

/*
 ================================
 Frustum::operator=
 ================================
 */
Frustum& Frustum::operator = ( const Frustum & rhs ) {
    for ( int i = 0; i < 6; ++i ) {
        m_planes[ i ] = rhs.m_planes[ i ];
    }
	m_bounds = rhs.m_bounds;
	m_isInitialized = rhs.m_isInitialized;
	return *this;
}

/*
 ================================
 Frustum::~Frustum
 ================================
 */
Frustum::~Frustum() {
}


/*
 ================================
 Frustum::Build
 ================================
 */
void Frustum::Build(  const float & nearz,
                            const float & farz,
                            const float & fovy_degrees,
                            const float & screenWidth,
                            const float & screenHeight,
                            const Vec3 & camPos,
                            const Vec3 & camUp,
                            const Vec3 & camLook ) {
    assert( fovy_degrees > 0 );
    assert( screenWidth > 0 );
    assert( screenHeight > 0 );
    Plane & nearPlane = m_planes[ 0 ];
    Plane & farPlane  = m_planes[ 1 ];
    Plane & top       = m_planes[ 2 ];
    Plane & bottom    = m_planes[ 3 ];
    Plane & right     = m_planes[ 4 ];
    Plane & left      = m_planes[ 5 ];
    const float aspect  = screenWidth / screenHeight;

	m_camPos = camPos;
	m_camLookDir = camLook;
	m_camUp = camUp;
	m_fovRad = fovy_degrees * Math::PI / 180.0f;
	m_fovDeg = fovy_degrees;
    
    nearPlane   = Plane( camPos + camLook * nearz, camLook * -1.0f );
    farPlane    = Plane( camPos + camLook * farz, camLook );

	const Vec3 camRight	= ( camLook.Cross( camUp ) ).Normalize();
	const Vec3 camLeft	= camRight * -1.0f;

	const Vec3 camUp2 = camRight.Cross( camLook ).Normalize();
	const Vec3 camDown	= camUp2 * -1.0f;

	Vec3 fwdUp		= camLook;
	Vec3 fwdDown	= camLook;
	Vec3 fwdRight	= camLook;
	Vec3 fwdLeft	= camLook;

	Quat testQuat = Quat( Vec3( 0, 0, 1 ), Math::PI * 0.5f );
	Mat3 testMat = testQuat.ToMat3();
	Vec3 testVec = Vec3( 1, 0, 0 );
	Vec3 testA = testQuat.RotatePoint( testVec );
	Vec3 testB = testMat * testVec;

    //
    // calculate the top and bottom planes
    //
    {
		Quat quat;
		Mat3 matRot;
        const float halfAngleY = fovy_degrees * 0.5f;
		const float halfAngleYRadians = halfAngleY * Math::PI / 180.0f;

		quat = Quat( camRight, halfAngleYRadians );
		matRot = quat.ToMat3();
		fwdUp = matRot * camLook;
        const Vec3 up = matRot * camUp2;
		const Vec3 up2 = quat.RotatePoint( camUp2 );
        top = Plane( camPos, up2 );
        
		quat = Quat( camRight, -halfAngleYRadians );
		matRot = quat.ToMat3();
		fwdDown = matRot * camLook;
        const Vec3 down = matRot * camDown;
		const Vec3 down2 = quat.RotatePoint( camDown );
        bottom = Plane( camPos, down2 );
    }
    
    //
    // calculate the right and left planes
    //
    {
		Quat quat;
		Mat3 matRot;
        const float halfAngle = fovy_degrees * 0.5f * aspect;
		const float halfAngleRadians = halfAngle * Math::PI / 180.0f;
        
		quat = Quat( camUp2, halfAngleRadians );
		matRot = quat.ToMat3();
		fwdLeft = matRot * camLook;
        const Vec3 leftNorm = matRot * camLeft;
		const Vec3 leftNorm2 = quat.RotatePoint( camLeft );
        left = Plane( camPos, leftNorm2 );
        
		quat = Quat( camUp2, -halfAngleRadians );
		matRot = quat.ToMat3();
		fwdRight = matRot * camLook;
        const Vec3 rightNorm = matRot * camRight;
		const Vec3 rightNorm2 = quat.RotatePoint( camRight );
        right = Plane( camPos, rightNorm2 );
    }

	//
	//	Calculate corner points
	//	Use the fwd rays, intersecting with near/far planes to calculate
	//
	float t = 0.0f;
	nearPlane.IntersectRay(	camPos, fwdLeft + fwdUp, t, m_corners[ 0 ] );
	farPlane.IntersectRay(	camPos, fwdLeft + fwdUp, t, m_corners[ 1 ] );

	nearPlane.IntersectRay(	camPos, fwdRight + fwdUp, t, m_corners[ 2 ] );
	farPlane.IntersectRay(	camPos, fwdRight + fwdUp, t, m_corners[ 3 ] );

	nearPlane.IntersectRay(	camPos, fwdRight + fwdDown, t, m_corners[ 4 ] );
	farPlane.IntersectRay(	camPos, fwdRight + fwdDown, t, m_corners[ 5 ] );

	nearPlane.IntersectRay(	camPos, fwdLeft + fwdDown, t, m_corners[ 6 ] );
	farPlane.IntersectRay(	camPos, fwdLeft + fwdDown, t, m_corners[ 7 ] );

	// Expand the bounding box
	Bounds bounds;
	for ( int i = 0; i < 8; ++i ) {
		bounds.Expand( m_corners[ i ] );
	}
	m_bounds = bounds;
}

/*
 ================================
 Frustum::DoBoundsIntersectFrustum
 ================================
 */
bool Frustum::DoBoundsIntersectFrustum( const Bounds & box ) const {
	for ( int i = 0; i < 6; i++ ) {
		const Plane & plane = m_planes[ i ];

		// Use the normal of the plane to get the "closer" support point on the bounds
		Vec3 pt;
		pt.x = ( plane.mNormal.x > 0.0f ) ? box.mins.x : box.maxs.x;
		pt.y = ( plane.mNormal.y > 0.0f ) ? box.mins.y : box.maxs.y;
		pt.z = ( plane.mNormal.z > 0.0f ) ? box.mins.z : box.maxs.z;

		// Check if the "closer" point is on the positive or negative side of the plane
		// If the "closer" point is on the positive side, then the bounds do not intersect
		float dist = plane.DistanceFromPlane( pt );
		if ( dist > 0.0f ) {
			return false;
		}
	}

	return true;
}

/*
 ================================
 Frustum::IsPointInFrustum
 ================================
 */
bool Frustum::IsPointInFrustum( const Vec3 & pt ) const {
	for ( int i = 0; i < 6; i++ ) {
		const Plane & plane = m_planes[ i ];

		float dist = plane.DistanceFromPlane( pt );
		if ( dist > 0.0f ) {
			return false;
		}
	}

	return true;
}

/*
 ================================
 Frustum::IsSphericalPointInFrustum
 ================================
 */
bool Frustum::IsSphericalPointInFrustum( const Vec3 & pt, const float radius ) const {
	for ( int i = 0; i < 6; i++ ) {
		const Plane & plane = m_planes[ i ];

		float dist = plane.DistanceFromPlane( pt );
		if ( dist > radius ) {
			return false;
		}
	}

	return true;
}