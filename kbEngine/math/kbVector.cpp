//===================================================================================================
// kbVector.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbPlane.h"
#include "kbQuaternion.h"

const kbVec3 kbVec3::right( 1.0f, 0.0f, 0.0f );
const kbVec3 kbVec3::up( 0.0f, 1.0f, 0.0f );
const kbVec3 kbVec3::down( 0.0f, -1.0f, 0.0f );
const kbVec3 kbVec3::forward( 0.0f, 0.0f, 1.0f );
const kbVec3 kbVec3::zero( 0.0f, 0.0f, 0.0f );
const kbVec3 kbVec3::one( 1.0f, 1.0f, 1.0f );

const kbVec4 kbVec4::right( 1.0f, 0.0f, 0.0f, 0.0f );
const kbVec4 kbVec4::up( 0.0f, 1.0f, 0.0f, 0.0f );
const kbVec4 kbVec4::forward( 0.0f, 0.0f, 1.0f, 0.0f );

const kbColor kbColor::red( 1.0f, 0.0f, 0.0f, 1.0f );
const kbColor kbColor::green( 0.0f, 1.0f, 0.0f, 1.0f );
const kbColor kbColor::blue( 0.0f, 0.0f, 1.0f, 1.0f );
const kbColor kbColor::yellow( 1.0f, 1.0f, 0.0f, 1.0f );
const kbColor kbColor::white( 1.0f, 1.0f, 1.0f, 1.0f );

const kbMat4 kbMat4::identity( kbVec4::right, kbVec4::up, kbVec4::forward, kbVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );

kbVec2 operator *( const kbVec2 & op1, const float op2 ) {
	return kbVec2( op2 * op1.x, op2 * op1.y );
}

kbVec3 kbVec3::operator *( const kbMat4 & rhs ) const {
	kbVec3 returnVec;
	
	returnVec.x = (x * rhs[0][0]) + (y * rhs[1][0]) + (z * rhs[2][0]);
	returnVec.y = (x * rhs[0][1]) + (y * rhs[1][1]) + (z * rhs[2][1]);
	returnVec.z = (x * rhs[0][2]) + (y * rhs[1][2]) + (z * rhs[2][2]);

	return returnVec;
}

kbVec3 operator *( const float op1, const kbVec3 & op2 ) {
	return kbVec3( op1 * op2.x, op1 * op2.y, op1 * op2.z ); 
}

kbVec4 operator *( const float op1, const kbVec4 & op2 ) {
	return kbVec4( op1 * op2.x, op1 * op2.y, op1 * op2.z, op1 * op2.w ); 
}

kbVec4	kbVec4::TransformPoint(const kbMat4 & op2, bool bDivideByW) const
{
	kbVec4 returnVec;
	
	returnVec.x = (x * op2[0][0]) + (y * op2[1][0]) + (z * op2[2][0]) + (w * op2[3][0]);
	returnVec.y = (x * op2[0][1]) + (y * op2[1][1]) + (z * op2[2][1]) + (w * op2[3][1]);
	returnVec.z = (x * op2[0][2]) + (y * op2[1][2]) + (z * op2[2][2]) + (w * op2[3][2]);
	returnVec.w = (x * op2[0][3]) + (y * op2[1][3]) + (z * op2[2][3]) + (w * op2[3][3]);

	if ( bDivideByW )
	{
		returnVec /= returnVec.w;
	}
	return returnVec;
}


kbMat4::kbMat4( const kbVec4 & xAxis, const kbVec4 & yAxis, const kbVec4 & zAxis, const kbVec4 & wAxis ) {
    Set( xAxis, yAxis, zAxis, wAxis );
}

kbMat4::kbMat4( const kbQuat & rotation, const kbVec3 & position ) {
    *this = rotation.ToMat4();
    (*this)[3] = position;
}

kbVec3 kbMat4::TransformPoint(const kbVec3 Point) const {
	kbVec3 returnVec;
	
	returnVec.x = (Point.x * mat[0][0]) + (Point.y * mat[1][0]) + (Point.z * mat[2][0]) + mat[3][0];
	returnVec.y = (Point.x * mat[0][1]) + (Point.y * mat[1][1]) + (Point.z * mat[2][1]) + mat[3][1];
	returnVec.z = (Point.x * mat[0][2]) + (Point.y * mat[1][2]) + (Point.z * mat[2][2]) + mat[3][2];

	return returnVec;
}

void kbMat4::GetLeftClipPlane( kbPlane & ClipPlane ) {	
	ClipPlane.x = mat[0][3] + mat[0][0];
	ClipPlane.y = mat[1][3] + mat[1][0];
	ClipPlane.z = mat[2][3] + mat[2][0];
	ClipPlane.w = mat[3][3] + mat[3][0];

	float InvSqrtLen = 1.f / sqrt( ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z );

	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

void kbMat4::GetRightClipPlane(kbPlane &ClipPlane)
{
	ClipPlane.x = mat[0][3] - mat[0][0];
	ClipPlane.y = mat[1][3] - mat[1][0];
	ClipPlane.z = mat[2][3] - mat[2][0];
	ClipPlane.w = mat[3][3] - mat[3][0];

	float InvSqrtLen = 1.f / sqrt( ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z );

	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

void kbMat4::GetTopClipPlane(kbPlane &ClipPlane)
{
	ClipPlane.x = mat[0][3] - mat[0][1];
	ClipPlane.y = mat[1][3] - mat[1][1];
	ClipPlane.z = mat[2][3] - mat[2][1];
	ClipPlane.w = mat[3][3] - mat[3][1];

	float InvSqrtLen = 1.f / sqrt( ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z );

	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

void kbMat4::GetBottomClipPlane(kbPlane &ClipPlane)
{
	ClipPlane.x = mat[0][3] + mat[0][1];
	ClipPlane.y = mat[1][3] + mat[1][1];
	ClipPlane.z = mat[2][3] + mat[2][1];
	ClipPlane.w = mat[3][3] + mat[3][1];

	float InvSqrtLen = 1.f / sqrt( ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z );

	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

void kbMat4::GetNearClipPlane(kbPlane &ClipPlane)
{
	ClipPlane.x = mat[0][2];
	ClipPlane.y = mat[1][2];
	ClipPlane.z = mat[2][2];
	ClipPlane.w = mat[3][2];

	float InvSqrtLen = 1.f / sqrt( ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z );

	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}

void kbMat4::GetFarClipPlane(kbPlane &ClipPlane)
{
	ClipPlane.x = mat[0][3] - mat[0][2];
	ClipPlane.y = mat[1][3] - mat[1][2];
	ClipPlane.z = mat[2][3] - mat[2][2];
	ClipPlane.w = mat[3][3] - mat[3][2];

	float InvSqrtLen = 1.f / sqrt( ClipPlane.x * ClipPlane.x + ClipPlane.y * ClipPlane.y + ClipPlane.z * ClipPlane.z );

	ClipPlane.x *= -InvSqrtLen;
	ClipPlane.y *= -InvSqrtLen;
	ClipPlane.z *= -InvSqrtLen;
	ClipPlane.w *= InvSqrtLen;
}