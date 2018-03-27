//===================================================================================================
// kbCamera.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBCAMERA_H_
#define _KBCAMERA_H_

#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"

/**
 *	kbCamera
 */
class kbCamera {
public:
	friend class kbEditor;

	kbCamera();

	void Update();

//private:
	kbVec3	m_Position;
	kbQuat	m_Rotation;

	kbMat4	m_EyeMats[2];

	kbQuat	m_RotationTarget;
};

#endif