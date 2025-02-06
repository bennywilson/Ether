/// kbPlane.h
///
/// 2017-2015 blk 1.0

#pragma once

#include "kbVector.h"

enum KBPLANE_Intersect {
	PLANE_BOTH_IN,
	PLANE_BOTH_OUT,
	PLANE_BOTH_ON,
	PLANE_FIRSTVERT_IN,
	PLANE_SECONDVERT_IN,
};

class kbPlane : public kbVec3 {
public:
	kbPlane() : w(1.f) {}
	kbPlane(const float inX, const float inY, const float inZ, const float inW) { x = inX, y = inY, z = inZ, w = inW; }
	kbPlane(const kbVec3& Normal, const float W) { x = Normal.x, y = Normal.y, z = Normal.z, w = W; }
	kbPlane(const kbVec3& Point, const kbVec3& Normal) {
		x = Normal.x;
		y = Normal.y;
		z = Normal.z;
		w = Point.dot(Normal);
	}

	KBPLANE_Intersect Intersect(const kbVec3& startPt, const kbVec3& endPt, float& t, kbVec3& intersectionPt);

	bool PlanesIntersect(kbVec3& KnownPoint, kbVec3& Direction, const kbPlane& op2) const;

	float DotWithVec(const kbVec3& Vec);

	float w;
};
