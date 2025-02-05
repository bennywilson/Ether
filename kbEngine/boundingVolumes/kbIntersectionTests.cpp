//===================================================================================================
// kbIntersectionTests.cpp
//
//
// 2016-2018 blk 1.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbIntersectionTests.h"
#include "kbBounds.h"

/**
 *	kbRayOBBIntersection
 */
bool kbRayOBBIntersection( const kbMat4 & orientation, const kbVec3 & origin, const kbVec3 & start, const kbVec3 & end, const kbVec3 & min, const kbVec3 & max ) {
	kbMat4 transpose = orientation;
	transpose.TransposeUpper();
	//kbVec3 origin = ( max + min ) * 0.5f;

	const kbVec3 p1 = ( start - origin ) * transpose;
	const kbVec3 p2 = ( end - origin ) * transpose;

	const  kbVec3 d = ( p2 - p1 ) * 0.5f;
	const  kbVec3 e = ( max - min ) * 0.5f;
    const kbVec3 c = p1 + d - ( min + max ) * 0.5f;

    //Vector3 ad = d.Absolute(); // Returns same vector with all components positive
	const kbVec3 ad( fabsf( d.x ), fabsf( d.y ), fabsf( d.z ) );

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

/**
 *	kbRayAABBIntersection
 */
bool kbRayAABBIntersection( float & outT, const kbVec3 & origin, const kbVec3 & direction, const kbBounds & box ) {
	const kbVec3 tMin = ( box.Min() - origin ) / direction;
	const kbVec3 tMax = ( box.Max() - origin ) / direction;

	kbVec3 realMin, realMax;
	for ( int i = 0; i < 3; i++ ) {
		realMin[i] = min( tMin[i], tMax[i] );
		realMax[i] = max( tMin[i], tMax[i] );
	}

	float minMax = min( min( realMax.x, realMax.y ), realMax.z );
	float maxMin = max( max( realMin.x, realMin.y ), realMin.z );

	outT = maxMin;
	return minMax >= maxMin;
}

/**
 *	kbRayAABBIntersection
 */
bool kbRayAABBIntersection( const kbVec3 & origin, const kbVec3 & direction, const kbBounds & box ) {
	float t;
	return kbRayAABBIntersection( t, origin, direction, box );
}

/**
 *	kbRayTriIntersection - From Real-Time Rendering by Tomas Akenine-Moller and Eric Haines
 */
bool kbRayTriIntersection( float & outT, const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbVec3 & v0, const kbVec3 & v1, const kbVec3 & v2 ) {

	const kbVec3 e1 = v1 - v0;
	const kbVec3 e2 = v2 - v0;
	const kbVec3 p  = rayDirection.Cross( e2 );
	const float a = e1.Dot( p );
	
	if ( a > -kbEpsilon && a < kbEpsilon ) {
		return false;
	}

	const float f = 1.0f / a;

	const kbVec3 s = rayOrigin - v0;
	const float u = f * ( s.Dot( p ) );

	if ( u < 0.0f || u > 1.0f ) {
		return false;
	}

	const kbVec3 q = s.Cross( e1 );
	const float v = f * ( rayDirection.Dot( q ) );
	if ( v < 0.0f || u + v > 1.0f ) {
		return false;
	}

	outT = f * ( e2.Dot( q ) );

	return true;
}

/**
 *	kbRaySphereIntersection
 */
bool kbRaySphereIntersection( kbVec3 & outIntersectionPt, const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbVec3 & sphereOrigin, const float sphereRadius ) {
	const float sphereRadiusSqr = sphereRadius * sphereRadius;
	const kbVec3 rayToSphereVec = sphereOrigin - rayOrigin;

	const float rayLen = rayToSphereVec.LengthSqr();
	if ( rayLen < sphereRadiusSqr ) {
		outIntersectionPt = rayOrigin;
		return true;
	}
	const float rayDirDDotRayToSphere = rayDirection.Dot( rayToSphereVec );

	if ( rayDirDDotRayToSphere <= 0.0f ) {
		return false;
	}

	const kbVec3 closestPtToSphere = rayOrigin + rayDirection * rayDirDDotRayToSphere;

	const kbVec3 closestPtToSphereOrigin = closestPtToSphere - sphereOrigin;
	const float sqrDistToCenter = closestPtToSphereOrigin.Dot( closestPtToSphereOrigin );
	if ( sqrDistToCenter > sphereRadiusSqr ) {
		return false;
	}

	const float closestPtToSphereIntersectDist = sqrt( sphereRadiusSqr - sqrDistToCenter );
	const float finalDist = rayDirDDotRayToSphere - closestPtToSphereIntersectDist;

	outIntersectionPt = rayOrigin + rayDirection * finalDist;

	return true;
}
