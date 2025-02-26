/// Plane3d.h
///
/// 2017-2015 blk 1.0

#pragma once

#include "matrix.h"

enum Plane3d_Intersect {
	PLANE_BOTH_IN,
	PLANE_BOTH_OUT,
	PLANE_BOTH_ON,
	PLANE_FIRSTVERT_IN,
	PLANE_SECONDVERT_IN,
};

/// Plane3d
class Plane3d : public Vec3 {
public:
	Plane3d() : w(1.f) {}
	Plane3d(const float inX, const float inY, const float inZ, const float inW) { x = inX, y = inY, z = inZ, w = inW; }
	Plane3d(const Vec3& Normal, const float W) { x = Normal.x, y = Normal.y, z = Normal.z, w = W; }
	Plane3d(const Vec3& Point, const Vec3& Normal) {
		x = Normal.x;
		y = Normal.y;
		z = Normal.z;
		w = Point.dot(Normal);
	}

	Plane3d_Intersect Intersect(const Vec3& startPt, const Vec3& endPt, float& t, Vec3& intersectionPt);

	bool PlanesIntersect(Vec3& KnownPoint, Vec3& Direction, const Plane3d& op2) const;

	float DotWithVec(const Vec3& Vec);

	float w;
};
