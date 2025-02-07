/// kbPlane.h
///
/// 2017-2015 blk 1.0

#pragma once

#include "Matrix.h"

enum KBPLANE_Intersect {
	PLANE_BOTH_IN,
	PLANE_BOTH_OUT,
	PLANE_BOTH_ON,
	PLANE_FIRSTVERT_IN,
	PLANE_SECONDVERT_IN,
};

class kbPlane : public Vec3 {
public:
	kbPlane() : w(1.f) {}
	kbPlane(const float inX, const float inY, const float inZ, const float inW) { x = inX, y = inY, z = inZ, w = inW; }
	kbPlane(const Vec3& Normal, const float W) { x = Normal.x, y = Normal.y, z = Normal.z, w = W; }
	kbPlane(const Vec3& Point, const Vec3& Normal) {
		x = Normal.x;
		y = Normal.y;
		z = Normal.z;
		w = Point.dot(Normal);
	}

	KBPLANE_Intersect Intersect(const Vec3& startPt, const Vec3& endPt, float& t, Vec3& intersectionPt);

	bool PlanesIntersect(Vec3& KnownPoint, Vec3& Direction, const kbPlane& op2) const;

	float DotWithVec(const Vec3& Vec);

	float w;
};
