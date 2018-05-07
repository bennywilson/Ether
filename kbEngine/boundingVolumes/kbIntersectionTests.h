//===================================================================================================
// kbIntersectionTests.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBINTERSECTIONTESTS_H_
#define _KBINTERSECTIONTESTS_H_

class kbVec3;
class kbBounds;

bool kbRayOBBIntersection( const kbMat4 & orientation, const kbVec3 & origin, const kbVec3 & start, const kbVec3 & end, const kbVec3 & min, const kbVec3 & max );
bool kbRayAABBIntersection( const kbVec3 & origin, const kbVec3 & direction, const kbBounds & box );
bool kbRayAABBIntersection( float & outT, const kbVec3 & origin, const kbVec3 & direction, const kbBounds & box );
bool kbRayTriIntersection( float & outT, const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbVec3 & v0, const kbVec3 & v1, const kbVec3 & v2 );

bool kbRaySphereIntersection( kbVec3 & outIntersectionPt, const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbVec3 & sphereOrigin, const float sphereRadius );
#endif