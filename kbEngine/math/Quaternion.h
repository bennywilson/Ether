/// Quaternion.h
///
/// 2016-2025 blk 1.0

#pragma once

class Mat4;
class Vec3;

/// Quat4
class Quat4 {
public:
	Quat4() :
		x(0.f),
		y(0.f),
		z(0.f),
		w(1.f) {}

	explicit Quat4(const Vec3& axis, const float angle) { from_axis_angle(axis, angle); }
	explicit Quat4(const float x, const float y, const float z, const float w) { set(x, y, z, w); }

	float length() const;

	Quat4& normalize_self() {
		float length = this->length();

		if (length != 0.0f) {
			length = 1.0f / length;
			x *= length;
			y *= length;
			z *= length;
			w *= length;
		}

		return *this;
	}

	Quat4 normalize_safe() const {
		Quat4 returnQuat = *this;
		returnQuat.normalize_self();
		return returnQuat;
	}

	void set(const float inX, const float inY, const float inZ, const float inW) {
		x = inX;
		y = inY;
		z = inZ;
		w = inW;
	}

	Quat4 operator *(const Quat4& op2) const {
		return Quat4(w * op2.x + x * op2.w + y * op2.z - z * op2.y,
						w * op2.y + y * op2.w + z * op2.x - x * op2.z,
						w * op2.z + z * op2.w + x * op2.y - y * op2.x,
						w * op2.w - x * op2.x - y * op2.y - z * op2.z
		);
	}

	float operator |(const Quat4& quat2) const {
		return (x * quat2.x) + (y * quat2.y) + (z * quat2.z) + (w * quat2.w);
	}

	bool operator ==(const Quat4& op2) const;

	Mat4 to_mat4() const;
	void from_axis_angle(const Vec3& axis, float angle);

	static Quat4 slerp(const Quat4& from, const Quat4& to, float t);
	static Quat4 from_mat4(const Mat4& matrix);

	float x, y, z, w;

	static const Quat4 zero;
	static const Quat4 identity;
};
