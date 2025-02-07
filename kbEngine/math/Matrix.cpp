/// kbVector.cpp
///
/// 2016-2025 blk 1.0

#include "kbCore.h"
#include "Matrix.h"
#include "kbPlane.h"
#include "Quaternion.h"

const Vec2 Vec2::zero(0.0f, 0.0f);
const Vec2 Vec2::one(1.0f, 1.0f);

const Vec3 Vec3::right(1.0f, 0.0f, 0.0f);
const Vec3 Vec3::up(0.0f, 1.0f, 0.0f);
const Vec3 Vec3::down(0.0f, -1.0f, 0.0f);
const Vec3 Vec3::forward(0.0f, 0.0f, 1.0f);
const Vec3 Vec3::zero(0.0f, 0.0f, 0.0f);
const Vec3 Vec3::one(1.0f, 1.0f, 1.0f);

const Vec4 Vec4::right(1.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::up(0.0f, 1.0f, 0.0f, 0.0f);
const Vec4 Vec4::forward(0.0f, 0.0f, 1.0f, 0.0f);
const Vec4 Vec4::zero(0.0f, 0.0f, 0.0f, 0.0f);

const kbColor kbColor::red(1.0f, 0.0f, 0.0f, 1.0f);
const kbColor kbColor::green(0.0f, 1.0f, 0.0f, 1.0f);
const kbColor kbColor::blue(0.0f, 0.0f, 1.0f, 1.0f);
const kbColor kbColor::yellow(1.0f, 1.0f, 0.0f, 1.0f);
const kbColor kbColor::white(1.0f, 1.0f, 1.0f, 1.0f);
const kbColor kbColor::black(0.0f, 0.0f, 0.0f, 0.0f);

const Mat4 Mat4::identity(Vec4::right, Vec4::up, Vec4::forward, Vec4(0.0f, 0.0f, 0.0f, 1.0f));

Vec2 operator +(const Vec2& op1, const float op2) {
	return Vec2(op1.x + op2, op1.y + op2);
}

Vec2 operator *(const Vec2& op1, const float op2) {
	return Vec2(op1.x * op2, op1.y * op2);
}

Vec2 operator /(const Vec2& op1, const float op2) {
	return Vec2(op1.x / op2, op1.y / op2);
}


Vec3 Vec3::operator *(const Mat4& rhs) const {
	Vec3 returnVec;

	returnVec.x = (x * rhs[0][0]) + (y * rhs[1][0]) + (z * rhs[2][0]);
	returnVec.y = (x * rhs[0][1]) + (y * rhs[1][1]) + (z * rhs[2][1]);
	returnVec.z = (x * rhs[0][2]) + (y * rhs[1][2]) + (z * rhs[2][2]);

	return returnVec;
}

Vec3 operator *(const float op1, const Vec3& op2) {
	return Vec3(op1 * op2.x, op1 * op2.y, op1 * op2.z);
}

Vec4 operator *(const float op1, const Vec4& op2) {
	return Vec4(op1 * op2.x, op1 * op2.y, op1 * op2.z, op1 * op2.w);
}

/// Vec4::transform_poin
Vec4 Vec4::transform_point(const Mat4& op2, bool bDivideByW) const {
	Vec4 returnVec;
	returnVec.x = (x * op2[0][0]) + (y * op2[1][0]) + (z * op2[2][0]) + (w * op2[3][0]);
	returnVec.y = (x * op2[0][1]) + (y * op2[1][1]) + (z * op2[2][1]) + (w * op2[3][1]);
	returnVec.z = (x * op2[0][2]) + (y * op2[1][2]) + (z * op2[2][2]) + (w * op2[3][2]);
	returnVec.w = (x * op2[0][3]) + (y * op2[1][3]) + (z * op2[2][3]) + (w * op2[3][3]);

	if (bDivideByW) {
		returnVec /= returnVec.w;
	}
	return returnVec;
}

/// Mat4::Mat4
Mat4::Mat4(const Vec4& xAxis, const Vec4& yAxis, const Vec4& zAxis, const Vec4& wAxis) {
	set(xAxis, yAxis, zAxis, wAxis);
}

/// Mat4::Mat4
Mat4::Mat4(const Quat4& rotation, const Vec3& position) {
	*this = rotation.to_mat4();
	(*this)[3] = position;
}

/// Mat4::transform_point
Vec3 Mat4::transform_point(const Vec3& point) const {
	Vec3 returnVec;
	returnVec.x = (point.x * mat[0][0]) + (point.y * mat[1][0]) + (point.z * mat[2][0]) + mat[3][0];
	returnVec.y = (point.x * mat[0][1]) + (point.y * mat[1][1]) + (point.z * mat[2][1]) + mat[3][1];
	returnVec.z = (point.x * mat[0][2]) + (point.y * mat[1][2]) + (point.z * mat[2][2]) + mat[3][2];

	return returnVec;
}

/// Mat4::left_clip_plane
void Mat4::left_clip_plane(kbPlane& ClipPlane) {
	ClipPlane.x = mat[0][3] + mat[0][0];
	ClipPlane.y = mat[1][3] + mat[1][0];
	ClipPlane.z = mat[2][3] + mat[2][0];
	ClipPlane.w = mat[3][3] + mat[3][0];

	float InvSqrtLen = 1.f / sqrt(ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z);
	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

/// Mat4::right_clip_plane(
void Mat4::right_clip_plane(kbPlane& ClipPlane) {
	ClipPlane.x = mat[0][3] - mat[0][0];
	ClipPlane.y = mat[1][3] - mat[1][0];
	ClipPlane.z = mat[2][3] - mat[2][0];
	ClipPlane.w = mat[3][3] - mat[3][0];

	float InvSqrtLen = 1.f / sqrt(ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z);
	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

/// Mat4::top_clip_plane
void Mat4::top_clip_plane(kbPlane& ClipPlane) {
	ClipPlane.x = mat[0][3] - mat[0][1];
	ClipPlane.y = mat[1][3] - mat[1][1];
	ClipPlane.z = mat[2][3] - mat[2][1];
	ClipPlane.w = mat[3][3] - mat[3][1];

	float InvSqrtLen = 1.f / sqrt(ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z);
	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

/// Mat4::bottom_clip_plane
void Mat4::bottom_clip_plane(kbPlane& ClipPlane) {
	ClipPlane.x = mat[0][3] + mat[0][1];
	ClipPlane.y = mat[1][3] + mat[1][1];
	ClipPlane.z = mat[2][3] + mat[2][1];
	ClipPlane.w = mat[3][3] + mat[3][1];

	float InvSqrtLen = 1.f / sqrt(ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z);
	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

/// Mat4::near_clip_plane
void Mat4::near_clip_plane(kbPlane& ClipPlane) {
	ClipPlane.x = mat[0][2];
	ClipPlane.y = mat[1][2];
	ClipPlane.z = mat[2][2];
	ClipPlane.w = mat[3][2];

	const float InvSqrtLen = 1.f / sqrt(ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z);
	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

/// Mat4::far_clip_plane
void Mat4::far_clip_plane(kbPlane& ClipPlane) {
	ClipPlane.x = mat[0][3] - mat[0][2];
	ClipPlane.y = mat[1][3] - mat[1][2];
	ClipPlane.z = mat[2][3] - mat[2][2];
	ClipPlane.w = mat[3][3] - mat[3][2];

	const float InvSqrtLen = 1.f / sqrt(ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z);
	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}
