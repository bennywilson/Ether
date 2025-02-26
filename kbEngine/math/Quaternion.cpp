/// Quat4ernion.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "Quaternion.h"
#include "Matrix.h"

const Quat4 Quat4::zero(0.0f, 0.0f, 0.0f, 0.0f);
const Quat4 Quat4::identity(0.0f, 0.0f, 0.0f, 1.0f);

/// Quat4::length
float Quat4::length() const {
	return sqrtf((x * x) + (y * y) + (z * z) + (w * w));
}

/// Quat4::operator==
bool Quat4::operator==(const Quat4& op2) const {
	const float epsilon = 0.00001f;

	if (fabsf(x - op2.x) > epsilon) {
		return false;
	}

	if (fabsf(y - op2.y) > epsilon) {
		return false;
	}

	if (fabsf(z - op2.z) > epsilon) {
		return false;
	}

	if (fabsf(w - op2.w) > epsilon) {
		return false;
	}

	return true;
}

/// Quat4::slerp
Quat4 Quat4::slerp(const Quat4& from, const Quat4& to, const float t) {
	float cosom, absCosom, sinom, omega, scale0, scale1;

	if (t <= 0.0f) {
		return from;
	}

	if (t >= 1.0f) {
		return to;
	}

	if (from == to) {
		return to;
	}

	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	absCosom = fabsf(cosom);

	if ((1.0f - absCosom) > 1e-6f) {
#if 0
		omega = acos(absCosom);
		sinom = 1.0f / sin(omega);
		scale0 = sin((1.0f - t) * omega) * sinom;
		scale1 = sin(t * omega) * sinom;
#else
		scale0 = 1.0f - absCosom * absCosom;
		sinom = 1.0f / sqrtf(scale0);
		omega = atan2f(scale0 * sinom, absCosom);
		scale0 = sinf((1.0f - t) * omega) * sinom;
		scale1 = sinf(t * omega) * sinom;
#endif
	}
	else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	scale1 = (cosom >= 0.0f) ? scale1 : -scale1;

	Quat4 returnQuat;
	returnQuat.x = scale0 * from.x + scale1 * to.x;
	returnQuat.y = scale0 * from.y + scale1 * to.y;
	returnQuat.z = scale0 * from.z + scale1 * to.z;
	returnQuat.w = scale0 * from.w + scale1 * to.w;

	return returnQuat;
}

void Quat4::from_axis_angle(const Vec3& axis, float angle) {
	const float sin_a = sin(angle / 2.0f);
	const float cos_a = cos(angle / 2.0f);

	x = axis.x * sin_a;
	y = axis.y * sin_a;
	z = axis.z * sin_a;
	w = cos_a;
}

Mat4 Quat4::to_mat4() const {
	Mat4 mat;
	const float xx = x * x;
	const float xy = x * y;
	const float xz = x * z;
	const float xw = x * w;

	const float yy = y * y;
	const float yz = y * z;
	const float yw = y * w;

	const float zz = z * z;
	const float zw = z * w;

	mat[0][0] = 1 - 2 * (yy + zz);
	mat[0][1] = 2 * (xy - zw);
	mat[0][2] = 2 * (xz + yw);

	mat[1][0] = 2 * (xy + zw);
	mat[1][1] = 1 - 2 * (xx + zz);
	mat[1][2] = 2 * (yz - xw);

	mat[2][0] = 2 * (xz - yw);
	mat[2][1] = 2 * (yz + xw);
	mat[2][2] = 1 - 2 * (xx + yy);

	mat[0][3] = mat[1][3] = mat[2][3] = mat[3][0] = mat[3][1] = mat[3][2] = 0;
	mat[3][3] = 1;

	return mat;
}


/// Quat4::from_mat4
Quat4 Quat4::from_mat4(const Mat4& matrix) {
	float trace = matrix[0][0] + matrix[1][1] + matrix[2][2] + 1.0f;

	if (trace > 0.00001f) {
		const float s = sqrt(trace) * 2.0f;
		return Quat4((matrix[2][1] - matrix[1][2]) / s,
						(matrix[0][2] - matrix[2][0]) / s,
						(matrix[1][0] - matrix[0][1]) / s,
						s / 4);
	} else if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2]) {
		const float s = sqrtf(1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]) * 2;
		return Quat4(s / 4.0f,
						(matrix[1][0] + matrix[0][1]) / s,
						(matrix[0][2] + matrix[2][0]) / s,
						(matrix[2][1] - matrix[1][2]) / s);
	} else if (matrix[1][1] > matrix[2][2]) {
		const float s = sqrtf(1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2]) * 2.0f;
		return Quat4((matrix[1][0] + matrix[0][1]) / s,
						s / 4,
						(matrix[2][1] + matrix[1][2]) / 2,
						(matrix[0][2] - matrix[2][0]) / s);
	} else {
		const float s = sqrtf(1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1]) * 2.0f;
		return Quat4((matrix[0][2] + matrix[2][0]) / s,
						(matrix[2][1] + matrix[1][2]) / s,
						s / 4,
						(matrix[1][0] - matrix[0][1]) / s);
	}
}
