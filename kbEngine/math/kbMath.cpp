/// kbMath.cpp
///
/// 2016-2025 blk 1.0

#include <math.h>
#include <stdlib.h>
#include "kbMath.h"
#include "Matrix.h"

float CosInterpolation(float a, float b, float x) {
	float ft = x * 3.1415927f;
	float f = (float)(1 - cos((double)ft)) * 0.5f;
	float returnVal = a * (1.0f - f) + b * f;

	return returnVal;
}

int seed = 555;
float SeededNoise(const float x, const float y) {

	int n = seed + (int)x + (int)y * 57;
	n = (n << 13) ^ n;
	int nn = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
	return 1.0f - (nn / 1073741824.0f);
}

float SmoothNoise(const float x, const float y) {
	float corners = (SeededNoise(x - 1, y - 1) + SeededNoise(x + 1, y - 1) + SeededNoise(x - 1, y + 1), SeededNoise(x + 1, y + 1)) / 16.0f;
	float sides = (SeededNoise(x - 1, y) + SeededNoise(x + 1, y) + SeededNoise(x, y - 1) + SeededNoise(x, y + 1)) / 8.0f;
	float center = SeededNoise(x, y) / 4.0f;
	return corners + sides + center;
}

float InterpolatedNoise(const float x, const float y) {

	float iX = (float)((int)(x + 0.5f));
	float fractionalX = x - iX;

	float iY = (float)((int)(y + 0.5f));
	float fractionalY = y - iY;


	float v1 = SmoothNoise(iX, iY);
	float v2 = SmoothNoise(iX + 1, iY);
	float v3 = SmoothNoise(iX, iY + 1);
	float v4 = SmoothNoise(iX + 1, iY + 1);

	float i1 = CosInterpolation(v1, v2, fractionalX);
	float i2 = CosInterpolation(v3, v4, fractionalX);
	float finalVal = CosInterpolation(i1, i2, fractionalY);
	return finalVal;
}

float NormalizedNoise(float x, float y) {
	float p = 0.88f;
	int numOctaves = 4;
	float total = 0.0f;

	for (int iOctave = 0; iOctave < numOctaves; iOctave++) {
		float freq = (float)pow(2.0f, (float)iOctave);
		float amp = (float)pow(p, (float)iOctave);

		total += InterpolatedNoise(x * freq, y * freq) * amp;
	}

	return (total * 0.5f) + 0.5f;
}


int kbirand(const int min, const int max) {
	return min + rand() % (max - min);
}

// kbrand
f32 kbfrand(const f32 min, const f32 max) {
	const f32 rand_val = rand() / (f32)RAND_MAX;
	return min + rand_val * (max - min);
}

Vec2 Vec2Rand(const Vec2& min, const Vec2& max) {
	Vec2 randVec;
	randVec.x = min.x + (kbfrand() * (max.x - min.x));
	randVec.y = min.y + (kbfrand() * (max.y - min.y));

	return randVec;
}

Vec3 Vec3Rand(const Vec3& min, const Vec3& max) {
	Vec3 randVec;
	randVec.x = min.x + (kbfrand() * (max.x - min.x));
	randVec.y = min.y + (kbfrand() * (max.y - min.y));
	randVec.z = min.z + (kbfrand() * (max.z - min.z));

	return randVec;
}

Vec4 Vec4Rand(const Vec4& min, const Vec4& max) {
	Vec4 randVec;
	randVec.x = min.x + (kbfrand() * (max.x - min.x));
	randVec.y = min.y + (kbfrand() * (max.y - min.y));
	randVec.z = min.z + (kbfrand() * (max.z - min.z));
	randVec.w = min.w + (kbfrand() * (max.w - min.w));
	return randVec;

}
