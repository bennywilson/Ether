/// kbIntersectionTests.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "Matrix.h"
#include "kbIntersectionTests.h"
#include "kbBounds.h"

/// kbRayOBBIntersection
bool kbRayOBBIntersection( const Mat4 & orientation, const Vec3 & origin, const Vec3 & start, const Vec3 & end, const Vec3 & min, const Vec3 & max ) {
	Mat4 transpose = orientation;
	transpose.transpose_upper();
	//Vec3 origin = ( max + min ) * 0.5f;

	const Vec3 p1 = ( start - origin ) * transpose;
	const Vec3 p2 = ( end - origin ) * transpose;

	const  Vec3 d = ( p2 - p1 ) * 0.5f;
	const  Vec3 e = ( max - min ) * 0.5f;
    const Vec3 c = p1 + d - ( min + max ) * 0.5f;

    //Vector3 ad = d.Absolute(); // Returns same vector with all components positive
	const Vec3 ad( fabsf( d.x ), fabsf( d.y ), fabsf( d.z ) );

	if ( fabsf( c[0] ) > e[0] + ad[0] ) {
        return false;
	}

	if ( fabsf( c[1] ) > e[1] + ad[1] ) {
        return false;
	}

	if ( fabsf( c[2] ) > e[2] + ad[2] ) {
        return false;
	}

	const float EPSILON = 0.00001f;

	if ( fabsf( d[1] * c[2] - d[2] * c[1] ) > e[1] * ad[2] + e[2] * ad[1] + EPSILON ) {
        return false;
	}

	if ( fabsf( d[2] * c[0] - d[0] * c[2] ) > e[2] * ad[0] + e[0] * ad[2] + EPSILON ) {
        return false;
	}

	if ( fabsf( d[0] * c[1] - d[1] * c[0] ) > e[0] * ad[1] + e[1] * ad[0] + EPSILON ) {
        return false;
	}
            
    return true;
}

/// kbRayAABBIntersection
bool kbRayAABBIntersection( float & outT, const Vec3 & origin, const Vec3 & direction, const kbBounds & box ) {
	const Vec3 tMin = ( box.Min() - origin ) / direction;
	const Vec3 tMax = ( box.Max() - origin ) / direction;

	Vec3 realMin, realMax;
	for ( int i = 0; i < 3; i++ ) {
		realMin[i] = min( tMin[i], tMax[i] );
		realMax[i] = max( tMin[i], tMax[i] );
	}

	float minMax = min( min( realMax.x, realMax.y ), realMax.z );
	float maxMin = max( max( realMin.x, realMin.y ), realMin.z );

	outT = maxMin;
	return minMax >= maxMin;
}

/// kbRayAABBIntersection
bool kbRayAABBIntersection( const Vec3 & origin, const Vec3 & direction, const kbBounds & box ) {
	float t;
	return kbRayAABBIntersection( t, origin, direction, box );
}

/// kbRayTriIntersection - From Real-Time Rendering by Tomas Akenine-Moller and Eric Haines
bool kbRayTriIntersection( float & outT, const Vec3 & rayOrigin, const Vec3 & rayDirection, const Vec3 & v0, const Vec3 & v1, const Vec3 & v2 ) {

	const Vec3 e1 = v1 - v0;
	const Vec3 e2 = v2 - v0;
	const Vec3 p  = rayDirection.cross( e2 );
	const float a = e1.dot( p );
	
	if ( a > -kbEpsilon && a < kbEpsilon ) {
		return false;
	}

	const float f = 1.0f / a;

	const Vec3 s = rayOrigin - v0;
	const float u = f * ( s.dot( p ) );

	if ( u < 0.0f || u > 1.0f ) {
		return false;
	}

	const Vec3 q = s.cross( e1 );
	const float v = f * ( rayDirection.dot( q ) );
	if ( v < 0.0f || u + v > 1.0f ) {
		return false;
	}

	outT = f * ( e2.dot( q ) );

	return true;
}

/// kbRaySphereIntersection
bool kbRaySphereIntersection( Vec3 & outIntersectionPt, const Vec3 & rayOrigin, const Vec3 & rayDirection, const Vec3 & sphereOrigin, const float sphereRadius ) {
	const float sphereRadiusSqr = sphereRadius * sphereRadius;
	const Vec3 rayToSphereVec = sphereOrigin - rayOrigin;

	const float rayLen = rayToSphereVec.length_sqr();
	if ( rayLen < sphereRadiusSqr ) {
		outIntersectionPt = rayOrigin;
		return true;
	}
	const float rayDirDDotRayToSphere = rayDirection.dot( rayToSphereVec );

	if ( rayDirDDotRayToSphere <= 0.0f ) {
		return false;
	}

	const Vec3 closestPtToSphere = rayOrigin + rayDirection * rayDirDDotRayToSphere;

	const Vec3 closestPtToSphereOrigin = closestPtToSphere - sphereOrigin;
	const float sqrDistToCenter = closestPtToSphereOrigin.dot( closestPtToSphereOrigin );
	if ( sqrDistToCenter > sphereRadiusSqr ) {
		return false;
	}

	const float closestPtToSphereIntersectDist = sqrt( sphereRadiusSqr - sqrDistToCenter );
	const float finalDist = rayDirDDotRayToSphere - closestPtToSphereIntersectDist;

	outIntersectionPt = rayOrigin + rayDirection * finalDist;

	return true;
}
