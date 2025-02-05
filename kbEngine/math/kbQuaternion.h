/// kbQuaternion.h
///
/// 2016-2025 blk 1.0

#pragma once

/// kbQuat
class kbQuat {
public:
	kbQuat() { }
	explicit kbQuat(const kbVec3& axis, const float angle) { FromAxisAngle(axis, angle); }
	explicit kbQuat(const float x, const float y, const float z, const float w) { Set(x, y, z, w); }

	float Length();
	kbQuat& Normalize();
	kbQuat Normalized() const { kbQuat returnQuat = *this; returnQuat.Normalize(); return returnQuat; }

	void Set(const float inX, const float inY, const float inZ, const float inW) { x = inX, y = inY, z = inZ, w = inW; }

	kbQuat operator*(const kbQuat&) const;
	float operator|(const kbQuat&) const;
	inline bool	operator==(const kbQuat& a) const;

	void FromAxisAngle(const kbVec3& axis, float angle);
	kbMat4 ToMat4() const;

	static kbQuat Slerp(const kbQuat& from, const kbQuat& to, float t);

	float x, y, z, w;

	static const kbQuat zero;
	static const kbQuat identity;
};

/// kbQuatFromMatrix
inline kbQuat kbQuatFromMatrix(const kbMat4& matrix) {
	float trace = matrix[0][0] + matrix[1][1] + matrix[2][2] + 1.0f;

	if (trace > 0.00001f) {
		const float s = sqrt(trace) * 2.0f;
		return kbQuat((matrix[2][1] - matrix[1][2]) / s,
						(matrix[0][2] - matrix[2][0]) / s,
						(matrix[1][0] - matrix[0][1]) / s,
						s / 4);
	}
	else if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2]) {
		const float s = sqrtf(1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]) * 2;
		return kbQuat(s / 4.0f,
						(matrix[1][0] + matrix[0][1]) / s,
						(matrix[0][2] + matrix[2][0]) / s,
						(matrix[2][1] - matrix[1][2]) / s);
	}
	else if (matrix[1][1] > matrix[2][2]) {
		const float s = sqrtf(1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2]) * 2.0f;
		return kbQuat((matrix[1][0] + matrix[0][1]) / s,
						s / 4,
						(matrix[2][1] + matrix[1][2]) / 2,
						(matrix[0][2] - matrix[2][0]) / s);
	}
	else {
		const float s = sqrtf(1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1]) * 2.0f;
		return kbQuat((matrix[0][2] + matrix[2][0]) / s,
						(matrix[2][1] + matrix[1][2]) / s,
						s / 4,
						(matrix[1][0] - matrix[0][1]) / s);
	}
}

inline float kbQuat::operator|(const kbQuat& quat2) const {
	return (x * quat2.x) + (y * quat2.y) + (z * quat2.z) + (w * quat2.w);
}

/// Length
inline float kbQuat::Length() {
	return sqrtf((x * x) + (y * y) + (z * z) + (w * w));
}

/// Normalize
inline kbQuat& kbQuat::Normalize() {
	float length = Length();

	if (length != 0.0f) {
		length = 1.0f / length;
		x *= length;
		y *= length;
		z *= length;
		w *= length;
	}

	return *this;
}

/*
 * operator*()
 */
inline kbQuat kbQuat::operator*(const kbQuat& op2) const {
	return kbQuat(w * op2.x + x * op2.w + y * op2.z - z * op2.y,
					w * op2.y + y * op2.w + z * op2.x - x * op2.z,
					w * op2.z + z * op2.w + x * op2.y - y * op2.x,
					w * op2.w - x * op2.x - y * op2.y - z * op2.z);
}

/*
 * operator==()
 */
inline bool	kbQuat::operator==(const kbQuat& op2) const {
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

/*
 *	kbQuat::FromAxisAngle
 */
inline void kbQuat::FromAxisAngle(const kbVec3& axis, float angle) {
	const float sin_a = sin(angle / 2.0f);
	const float cos_a = cos(angle / 2.0f);

	x = axis.x * sin_a;
	y = axis.y * sin_a;
	z = axis.z * sin_a;
	w = cos_a;
}

/*
 *	kbQuat::ToMat4
 */
inline kbMat4 kbQuat::ToMat4() const {
	kbMat4 mat;
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
