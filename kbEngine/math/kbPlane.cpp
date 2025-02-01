/// kbPlane.cpp
///
// 2017-2025 kbEngine 2.0

#include "kbCore.h"
#include "kbPlane.h"

/**
 *	kbPlane::Intesect
 */
KBPLANE_Intersect kbPlane::Intersect(const kbVec3& startPt, const kbVec3& endPt, float& t, kbVec3& intersectionPt) {
	kbVec3 planeNormal(x, y, z);
	kbVec3 knownPt = planeNormal * w;
	kbVec3 vToStartPt = startPt - knownPt;
	kbVec3 vToEndPt = endPt - knownPt;

	int side = 0;

	float startDot = vToStartPt.Dot(planeNormal);
	float endDot = vToEndPt.Dot(planeNormal);

	if (startDot > 0 && endDot > 0) {
		return PLANE_BOTH_IN;
	}

	if (startDot <= 0 && endDot <= 0) {
		return PLANE_BOTH_OUT;
	}

	if (startDot == 0 && endDot == 0) {
		return PLANE_BOTH_OUT;	// for now
	}

	kbVec3 vecTo = endPt - startPt;
	vecTo.Normalize();

	float denominator = planeNormal.Dot(vecTo);

	//	if (denominator == 0)	
	//		return 0;

	float numerator = knownPt.Dot(planeNormal) - startPt.Dot(planeNormal);
	t = numerator / denominator;
	intersectionPt = startPt + vecTo * t;

	if (startDot > 0) {
		return PLANE_FIRSTVERT_IN;
	}
	else {
		return PLANE_SECONDVERT_IN;
	}
}

bool kbPlane::PlanesIntersect(kbVec3& KnownPoint, kbVec3& Direction, const kbPlane& op2) const {
	// Compute line direction, perpendicular to both plane normals.
	const kbPlane& op1 = *this;
	Direction = op1.Cross(op2);
	const float DirSqr = Direction.LengthSqr();

	const float EPSILON = 0.000001f;
	if (DirSqr < EPSILON)
	{
		return false;
	}
	else
	{
		// Compute intersection.
		KnownPoint = ((op2.Cross(Direction)) * op1.w + (Direction.Cross(op1)) * op2.w) / DirSqr;
		Direction.Normalize();
		return true;
	}
}

float kbPlane::DotWithVec(const kbVec3& Vec)
{
	return (x * Vec.x) + (y * Vec.y) + (z * Vec.z) - w;
}