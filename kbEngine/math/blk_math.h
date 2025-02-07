/// blk_math
///
/// 2016-2025 blk 1.0

#pragma once

#include <cmath>

// todo - make uint8_t
typedef unsigned char byte;
typedef float f32;

float SeededNoise(const float x, const float y);

float SmoothNoise(const float x, const float y);

float InterpolatedNoise(const float x, const float y);

float NormalizedNoise(const float x, const float y);

// helper functions
const float kbPI = 3.14159265359f;
const float kbEpsilon = 0.00001f;
inline float kbToRadians(const float degrees) { return degrees * kbPI / 180.0f; }
inline float kbToDegrees(const float radians) { return radians * 180.0f / kbPI; }

inline bool kbCompareByte4(const byte lhs[4], const byte rhs[4]) { return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2] && lhs[3] == rhs[3]; }

template<typename T> T kbClamp(const T& value, const T& min, const T& max) { return value < min ? min : (value > max ? max : value); }
template<typename T> T kbSaturate(const T& value) { return value < 0 ? 0 : (value > 1 ? 1 : value); }

template<typename T> inline T kbLerp(const T a, const T b, const float t) { return ((b - a) * t) + a; }

inline int kbirand(const int min, const int max);

float kbfrand(const float min = 0.f, const float max = 1.f);

class Vec2;
class Vec3;
class Vec4;

Vec2 Vec2Rand(const Vec2& min, const Vec2& max);
Vec3 Vec3Rand(const Vec3& min, const Vec3& max);
Vec4 Vec4Rand(const Vec4& min, const Vec4& max);