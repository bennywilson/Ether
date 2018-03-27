//===================================================================================================
// kbMath.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef __KBMATH_H_
#define __KBMATH_H_

#include <math.h>

float Interpolate( float a, float b, float x );
float SeededNoise( const float x, const float y );
float SmoothNoise( const float x, const float y );
float InterpolatedNoise( const float x, const float y );
float NormalizedNoise( const float x, const float y );

#endif