/// Plane3d.cpp
///
/// 2017-2025 blk 1.0

#include "blk_core.h"
#include "plane3d.h"

/// Plane3d::Intesect
Plane3d_Intersect Plane3d::Intersect(const Vec3& startPt, const Vec3& endPt, float& t, Vec3& intersectionPt) {
	Vec3 planeNormal(x, y, z);
	Vec3 knownPt = planeNormal * w;
	Vec3 vToStartPt = startPt - knownPt;
	Vec3 vToEndPt = endPt - knownPt;

	int side = 0;

	float startDot = vToStartPt.dot(planeNormal);
	float endDot = vToEndPt.dot(planeNormal);

	if (startDot > 0 && endDot > 0) {
		return PLANE_BOTH_IN;
	}

	if (startDot <= 0 && endDot <= 0) {
		return PLANE_BOTH_OUT;
	}

	if (startDot == 0 && endDot == 0) {
		return PLANE_BOTH_OUT;	// for now
	}

	Vec3 vecTo = endPt - startPt;
	vecTo.normalize_self();

	float denominator = planeNormal.dot(vecTo);

	//	if (denominator == 0)	
	//		return 0;

	float numerator = knownPt.dot(planeNormal) - startPt.dot(planeNormal);
	t = numerator / denominator;
	intersectionPt = startPt + vecTo * t;

	if (startDot > 0) {
		return PLANE_FIRSTVERT_IN;
	}
	else {
		return PLANE_SECONDVERT_IN;
	}
}

bool Plane3d::PlanesIntersect(Vec3& KnownPoint, Vec3& Direction, const Plane3d& op2) const {
	// Compute line direction, perpendicular to both plane normals.
	const Plane3d& op1 = *this;
	Direction = op1.cross(op2);
	const float DirSqr = Direction.length_sqr();

	const float EPSILON = 0.000001f;
	if (DirSqr < EPSILON)
	{
		return false;
	}
	else
	{
		// Compute intersection.
		KnownPoint = ((op2.cross(Direction)) * op1.w + (Direction.cross(op1)) * op2.w) / DirSqr;
		Direction.normalize_self();
		return true;
	}
}

float Plane3d::DotWithVec(const Vec3& Vec)
{
	return (x * Vec.x) + (y * Vec.y) + (z * Vec.z) - w;
}