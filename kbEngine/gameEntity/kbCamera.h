//===================================================================================================
// kbCamera.h
//
//
// 2016 blk 1.0
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
	Vec3	m_Position;
	kbQuat	m_Rotation;

	Mat4	m_EyeMats[2];

	kbQuat	m_RotationTarget;
};

#endif