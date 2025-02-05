//===================================================================================================
// kbCamera.cpp
//
//
// 2016 blk 1.0
//===================================================================================================
#include "kbCamera.h"

/**
 *	kbCamera
 */
kbCamera::kbCamera() :
	m_Position( 0.0f, 3.0f, 20.0f ),
	m_Rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_RotationTarget( 0.0f, 0.0f, 0.0f, 1.0f ) {
}

/**
 *	kbCamera::Update
 */
void kbCamera::Update() {
	m_Rotation = kbQuat::Slerp( m_Rotation, m_RotationTarget, 0.33f );
	m_Rotation.Normalize();
}