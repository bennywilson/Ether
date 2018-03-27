//===================================================================================================
// kbMath.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "kbMath.h"

float CosInterpolation( float a, float b, float x ) {
	float ft = x * 3.1415927f;
	float f = (float)( 1 - cos((double)ft) ) * 0.5f;
	float returnVal =  a * ( 1.0f - f ) + b * f;

	return returnVal;
}

int seed = 555;
float SeededNoise( const float x, const float y ) {

	int n = seed + (int)x + (int)y * 57;
	n = ( n << 13 ) ^ n;
	int nn = (n*(n*n*15731 + 789221 )+ 1376312589)&0x7fffffff;
	return 1.0f - ( nn / 1073741824.0f);
}

float SmoothNoise( const float x, const float y ) {
	float corners = ( SeededNoise( x - 1, y - 1 ) + SeededNoise( x + 1, y - 1 ) + SeededNoise( x - 1, y + 1 ), SeededNoise( x + 1, y + 1 ) ) / 16.0f;
	float sides = ( SeededNoise( x - 1, y ) + SeededNoise( x + 1, y ) + SeededNoise( x, y - 1 ) + SeededNoise( x,  y + 1 ) ) / 8.0f;
	float center = SeededNoise( x, y ) / 4.0f;
	return corners + sides + center;
}

float InterpolatedNoise( const float x, const float y ) {

	float iX = ( float )( ( int ) (x + 0.5f) );
	float fractionalX = x - iX;

	float iY = ( float ) ( ( int )( y + 0.5f) );
	float fractionalY = y - iY;


	float v1 = SmoothNoise( iX, iY );
	float v2 = SmoothNoise( iX + 1, iY );
	float v3 = SmoothNoise( iX, iY + 1 );
	float v4 = SmoothNoise( iX + 1, iY + 1 );

	float i1 = CosInterpolation( v1, v2, fractionalX );
	float i2 = CosInterpolation( v3, v4, fractionalX );
	float finalVal =  CosInterpolation( i1, i2, fractionalY );
	return finalVal;
}

float NormalizedNoise( float x, float y ) {
	float p = 0.88f;
	int numOctaves = 4;
	float total = 0.0f;

	for ( int iOctave = 0; iOctave < numOctaves; iOctave++ ) {
		float freq = (float)pow( 2.0f, (float)iOctave );
		float amp = (float)pow( p, (float)iOctave );

		total += InterpolatedNoise( x * freq, y * freq ) * amp;
	}

	return ( total * 0.5f ) + 0.5f;
}
