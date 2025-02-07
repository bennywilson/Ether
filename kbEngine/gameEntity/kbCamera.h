/// kbCamera.h
///
///
/// 2016-2025 blk 1.0

#pragma once

#include "Matrix.h"
#include "Quaternion.h"

/// kbCamera
class kbCamera {
public:
	friend class kbEditor;

	kbCamera();

	void Update();

//private:
	Vec3	m_Position;
	Quat4	m_Rotation;

	Mat4	m_EyeMats[2];

	Quat4	m_RotationTarget;
};
