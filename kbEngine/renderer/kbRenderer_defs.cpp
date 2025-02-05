//===================================================================================================
// kbRenderer_defs.cpp
//
// 2016-2019 blk 1.0
//===================================================================================================
#include "kbCore.h"
#include "kbRenderer_defs.h"

/**
 *	kbBoneMatrix_t::Invert
 */
void kbBoneMatrix_t::Invert() {
	kbVec3 Trans(-m_Axis[3] );
	m_Axis[3].Set( 0.0f, 0.0f, 0.0f );

	TransposeUpper();

	kbVec3 finalTrans;
	finalTrans.x = Trans.x*m_Axis[0].x + Trans.y*m_Axis[1].x + Trans.z*m_Axis[2].x + m_Axis[3].x;
	finalTrans.y = Trans.x*m_Axis[0].y + Trans.y*m_Axis[1].y + Trans.z*m_Axis[2].y + m_Axis[3].y;
	finalTrans.z = Trans.x*m_Axis[0].z + Trans.y*m_Axis[1].z + Trans.z*m_Axis[2].z + m_Axis[3].z;

	m_Axis[3].x = finalTrans.x;
	m_Axis[3].y = finalTrans.y;
	m_Axis[3].z = finalTrans.z;
}

/**
 *	kbBoneMatrix_t::SetFromQuat
 */
void kbBoneMatrix_t::SetFromQuat( const kbQuat & srcQuat ) {

	kbMat4 mat;
	const float xx = srcQuat.x * srcQuat.x;
	const float xy = srcQuat.x * srcQuat.y;
	const float xz = srcQuat.x * srcQuat.z;
	const float xw = srcQuat.x * srcQuat.w;

	const float yy = srcQuat.y * srcQuat.y;
	const float yz = srcQuat.y * srcQuat.z;
	const float yw = srcQuat.y * srcQuat.w;

	const float zz = srcQuat.z * srcQuat.z;
	const float zw = srcQuat.z * srcQuat.w;

	m_Axis[0].x = 1 - 2 * ( yy + zz );
	m_Axis[0].y = 2 * ( xy - zw );
	m_Axis[0].z =  2 * ( xz + yw );

	m_Axis[1].x = 2 * ( xy + zw );
	m_Axis[1].y = 1 - 2 * ( xx + zz );
	m_Axis[1].z = 2 * ( yz - xw );

	m_Axis[2].x = 2 * ( xz - yw );
	m_Axis[2].y = 2 * ( yz + xw );
	m_Axis[2].z = 1 - 2 * ( xx + yy );

	m_Axis[3] = kbVec3::zero;
}

/**
 *	kbBoneMatrix_t::TransposeUpper()
 */
void kbBoneMatrix_t::TransposeUpper() {
	kbBoneMatrix_t transposedMat;
	transposedMat.m_Axis[0].Set( m_Axis[0].x,  m_Axis[1].x,  m_Axis[2].x );
	transposedMat.m_Axis[1].Set( m_Axis[0].y,  m_Axis[1].y,  m_Axis[2].y );
	transposedMat.m_Axis[2].Set( m_Axis[0].z,  m_Axis[1].z,  m_Axis[2].z );
	transposedMat.m_Axis[3] = kbVec3::zero;
	*this = transposedMat;
}

/**
 *	kbBoneMatrix_t::operator*
 */
kbVec3 operator*( const kbVec3 & lhs, const kbBoneMatrix_t & rhs ) {
	kbVec3 returnValue;

	returnValue.x = ( lhs.x * rhs.m_Axis[0].x ) + ( lhs.y * rhs.m_Axis[1].x ) + ( lhs.z * rhs.m_Axis[2].x ) + rhs.m_Axis[3].x;
	returnValue.y = ( lhs.x * rhs.m_Axis[0].y ) + ( lhs.y * rhs.m_Axis[1].y ) + ( lhs.z * rhs.m_Axis[2].y ) + rhs.m_Axis[3].y;
	returnValue.z = ( lhs.x * rhs.m_Axis[0].z ) + ( lhs.y * rhs.m_Axis[1].z ) + ( lhs.z * rhs.m_Axis[2].z ) + rhs.m_Axis[3].z;

	return returnValue;
}

/**
 *	kbBoneMatrix_t::operator*=
 */
void kbBoneMatrix_t::operator*=( const kbBoneMatrix_t & op2 ) {
	kbBoneMatrix_t temp = *this;
	m_Axis[0].x = temp.m_Axis[0].x*op2.m_Axis[0].x + temp.m_Axis[0].y*op2.m_Axis[1].x + temp.m_Axis[0].z*op2.m_Axis[2].x;
	m_Axis[1].x = temp.m_Axis[1].x*op2.m_Axis[0].x + temp.m_Axis[1].y*op2.m_Axis[1].x + temp.m_Axis[1].z*op2.m_Axis[2].x;
	m_Axis[2].x = temp.m_Axis[2].x*op2.m_Axis[0].x + temp.m_Axis[2].y*op2.m_Axis[1].x + temp.m_Axis[2].z*op2.m_Axis[2].x;
	m_Axis[3].x = temp.m_Axis[3].x*op2.m_Axis[0].x + temp.m_Axis[3].y*op2.m_Axis[1].x + temp.m_Axis[3].z*op2.m_Axis[2].x + op2.m_Axis[3].x;

	m_Axis[0].y = temp.m_Axis[0].x*op2.m_Axis[0].y + temp.m_Axis[0].y*op2.m_Axis[1].y + temp.m_Axis[0].z*op2.m_Axis[2].y;
	m_Axis[1].y = temp.m_Axis[1].x*op2.m_Axis[0].y + temp.m_Axis[1].y*op2.m_Axis[1].y + temp.m_Axis[1].z*op2.m_Axis[2].y;
	m_Axis[2].y = temp.m_Axis[2].x*op2.m_Axis[0].y + temp.m_Axis[2].y*op2.m_Axis[1].y + temp.m_Axis[2].z*op2.m_Axis[2].y;
	m_Axis[3].y = temp.m_Axis[3].x*op2.m_Axis[0].y + temp.m_Axis[3].y*op2.m_Axis[1].y + temp.m_Axis[3].z*op2.m_Axis[2].y + op2.m_Axis[3].y;

	m_Axis[0].z = temp.m_Axis[0].x*op2.m_Axis[0].z + temp.m_Axis[0].y*op2.m_Axis[1].z + temp.m_Axis[0].z*op2.m_Axis[2].z;
	m_Axis[1].z = temp.m_Axis[1].x*op2.m_Axis[0].z + temp.m_Axis[1].y*op2.m_Axis[1].z + temp.m_Axis[1].z*op2.m_Axis[2].z;
	m_Axis[2].z = temp.m_Axis[2].x*op2.m_Axis[0].z + temp.m_Axis[2].y*op2.m_Axis[1].z + temp.m_Axis[2].z*op2.m_Axis[2].z;
	m_Axis[3].z = temp.m_Axis[3].x*op2.m_Axis[0].z + temp.m_Axis[3].y*op2.m_Axis[1].z + temp.m_Axis[3].z*op2.m_Axis[2].z + op2.m_Axis[3].z;
}

/**
 *	kbBoneMatrix_t::operator*=
 */
void kbBoneMatrix_t::operator*=( const kbMat4 & op2 ) {
	kbBoneMatrix_t temp = *this;
	m_Axis[0].x = temp.m_Axis[0].x*op2[0].x + temp.m_Axis[0].y*op2[1].x + temp.m_Axis[0].z*op2[2].x;
	m_Axis[1].x = temp.m_Axis[1].x*op2[0].x + temp.m_Axis[1].y*op2[1].x + temp.m_Axis[1].z*op2[2].x;
	m_Axis[2].x = temp.m_Axis[2].x*op2[0].x + temp.m_Axis[2].y*op2[1].x + temp.m_Axis[2].z*op2[2].x;
	m_Axis[3].x = temp.m_Axis[3].x*op2[0].x + temp.m_Axis[3].y*op2[1].x + temp.m_Axis[3].z*op2[2].x + op2[3].x;

	m_Axis[0].y = temp.m_Axis[0].x*op2[0].y + temp.m_Axis[0].y*op2[1].y + temp.m_Axis[0].z*op2[2].y;
	m_Axis[1].y = temp.m_Axis[1].x*op2[0].y + temp.m_Axis[1].y*op2[1].y + temp.m_Axis[1].z*op2[2].y;
	m_Axis[2].y = temp.m_Axis[2].x*op2[0].y + temp.m_Axis[2].y*op2[1].y + temp.m_Axis[2].z*op2[2].y;
	m_Axis[3].y = temp.m_Axis[3].x*op2[0].y + temp.m_Axis[3].y*op2[1].y + temp.m_Axis[3].z*op2[2].y + op2[3].y;

	m_Axis[0].z = temp.m_Axis[0].x*op2[0].z + temp.m_Axis[0].y*op2[1].z + temp.m_Axis[0].z*op2[2].z;
	m_Axis[1].z = temp.m_Axis[1].x*op2[0].z + temp.m_Axis[1].y*op2[1].z + temp.m_Axis[1].z*op2[2].z;
	m_Axis[2].z = temp.m_Axis[2].x*op2[0].z + temp.m_Axis[2].y*op2[1].z + temp.m_Axis[2].z*op2[2].z;
	m_Axis[3].z = temp.m_Axis[3].x*op2[0].z + temp.m_Axis[3].y*op2[1].z + temp.m_Axis[3].z*op2[2].z + op2[3].z;
}
