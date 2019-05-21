//===================================================================================================
// kbQuaternion.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"

const kbQuat kbQuat::zero( 0.0f, 0.0f, 0.0f, 0.0f );
const kbQuat kbQuat::identity( 0.0f, 0.0f, 0.0f, 1.0f );

/**
 *  kbQuat::kbQuat
 */
kbQuat kbQuat::Slerp( const kbQuat & from, const kbQuat & to, const float t ) {
	float cosom, absCosom, sinom, omega, scale0, scale1;

	if ( t <= 0.0f ) {
		return from;
	}

	if ( t >= 1.0f ) {
		return to;
	}

	if ( from == to ) {
		return to;
	}

	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	absCosom = fabsf( cosom );

	if ( ( 1.0f - absCosom ) > 1e-6f ) {
#if 0
		omega = acos( absCosom );
		sinom = 1.0f / sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) * sinom;
		scale1 = sin( t * omega ) * sinom;
#else
		scale0 = 1.0f - absCosom * absCosom;
		sinom = 1.0f / sqrtf( scale0 );
		omega = atan2f( scale0 * sinom, absCosom );
		scale0 = sinf( ( 1.0f - t ) * omega ) * sinom;
		scale1 = sinf( t * omega ) * sinom;
#endif
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	scale1 = ( cosom >= 0.0f ) ? scale1 : -scale1;

	kbQuat returnQuat;
	returnQuat.x = scale0 * from.x + scale1 * to.x;
	returnQuat.y = scale0 * from.y + scale1 * to.y;
	returnQuat.z = scale0 * from.z + scale1 * to.z;
	returnQuat.w = scale0 * from.w + scale1 * to.w;

	return returnQuat;
}
