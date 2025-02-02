//===================================================================================================
// kbPlane.h
//
//
// 2017 kbEngine 2.0
//===================================================================================================
#ifndef __KBPLANE_H_
#define __KBPLANE_H_

#include "kbVector.h"

enum KBPLANE_Intersect
{
	PLANE_BOTH_IN,
	PLANE_BOTH_OUT,
	PLANE_BOTH_ON,
	PLANE_FIRSTVERT_IN,
	PLANE_SECONDVERT_IN,
};

class kbPlane : public kbVec3 {
public:
	kbPlane(){}
	kbPlane( const float inX, const float inY, const float inZ, const float inW )	{ x = inX, y = inY, z = inZ, w = inW; }
	kbPlane( const kbVec3 & Normal, const float W )				{ x = Normal.x, y = Normal.y, z = Normal.z, w = W; }
	kbPlane( const kbVec3 & Point, const kbVec3 & Normal ){
		x = Normal.x;
		y = Normal.y;
		z = Normal.z;
		w = Point.Dot( Normal );
	}

	KBPLANE_Intersect Intesect( const kbVec3 & startPt, const kbVec3 & endPt, float & t, kbVec3 & intersectionPt );

	bool PlanesIntersect( kbVec3 & KnownPoint, kbVec3 & Direction, const kbPlane & op2 ) const;

	float DotWithVec( const kbVec3 & Vec );

	float w;
};

#endif
