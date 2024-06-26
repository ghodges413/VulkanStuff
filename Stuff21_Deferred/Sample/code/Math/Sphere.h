//
//  Sphere.h
//
#pragma once
#include "Math/Vector.h"

// Forward declarator
template < typename T > class Array;

/*
 ===============================
 Sphere
 ===============================
 */
class Sphere {
public:
    Sphere();
    Sphere( const Vec3 & center, const float radius );
    Sphere( const float x, const float y, const float z, const float r );
    Sphere( const Sphere & rhs );
    Sphere( const Vec3 * pts, const int num_pts );
    const Sphere & operator = ( const Sphere & rhs );
    ~Sphere() {}
    
    void Inflate( const Vec3 & pt );
    void Inflate( const Vec3 * pts, const int& num_pts );
    
    void TightestFit( const Vec3 * pts, const int& num_pts );
    void GeometricFit( const Vec3 * pts, const int& num_pts );
    
	static void BuildHalfSphereGeo( Array< Vec3 > & out, const int & numSteps );
    static void BuildGeometry( Array< Vec3 > & out, const int & numSteps );
    static Sphere Expand( const Vec3 & poleA, const Vec3 & poleB, const Vec3 & ext );

	bool IsPointInSphere( const Vec3 & point ) const;
	bool IntersectSphere( const Sphere & rhs ) const;
    
public:
	Vec3 mCenter;
	float   mRadius;
	bool    mInitialized;
};

/*
 ===============================
 hbIntersectRaySphere
 always t1 <= t2
 if t1 < 0 && t2 > 0 ray is inside
 if t1 < 0 && t2 < 0 sphere is behind ray origin
 recover the 3D position with p = rayStart + t * rayDir
 ===============================
 */
inline bool hbIntersectRaySphere( const Vec3 & rayStart, const Vec3 & rayDir, const Sphere & sphere, float & t1, float & t2 ) {
    // Note:    If we force the rayDir to be normalized,
    //          then we can get an optimization where
    //          a = 1, b = m.
    //          Which would decrease the number of operations
    const Vec3 m = sphere.mCenter - rayStart;
    const float a   = rayDir.Dot( rayDir );
    const float b   = m.Dot( rayDir );
    const float c   = m.Dot( m ) - sphere.mRadius * sphere.mRadius;
    
    const float delta = b * b - a * c;
    const float invA = 1.0f / a;
    
    if ( delta < 0 ) {
        // no real solutions exist
        return false;
    }
    
    const float deltaRoot = sqrtf( delta );
    t1 = invA * ( b - deltaRoot );
    t2 = invA * ( b + deltaRoot );

    return true;
}