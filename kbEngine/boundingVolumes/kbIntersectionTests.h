//===================================================================================================
// kbIntersectionTests.h
//
//
// 2016-2018 blk 1.0
//===================================================================================================
#ifndef _KBINTERSECTIONTESTS_H_
#define _KBINTERSECTIONTESTS_H_

class Vec3;
class kbBounds;

bool kbRayOBBIntersection( const Mat4 & orientation, const Vec3 & origin, const Vec3 & start, const Vec3 & end, const Vec3 & min, const Vec3 & max );
bool kbRayAABBIntersection( const Vec3 & origin, const Vec3 & direction, const kbBounds & box );
bool kbRayAABBIntersection( float & outT, const Vec3 & origin, const Vec3 & direction, const kbBounds & box );
bool kbRayTriIntersection( float & outT, const Vec3 & rayOrigin, const Vec3 & rayDirection, const Vec3 & v0, const Vec3 & v1, const Vec3 & v2 );

bool kbRaySphereIntersection( Vec3 & outIntersectionPt, const Vec3 & rayOrigin, const Vec3 & rayDirection, const Vec3 & sphereOrigin, const float sphereRadius );
#endif