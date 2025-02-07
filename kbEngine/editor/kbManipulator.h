//===================================================================================================
// kbManipulator.h
//
//
// 2016-2018 blk 1.0
//===================================================================================================
#ifndef _KBEDITORMANIPULATOR_H_
#define _KBEDITORMANIPULATOR_H_

 /**
  *	kbManipulator
  */
 class kbManipulator {

//---------------------------------------------------------------------------------------------------
 public:
	 enum manipulatorMode_t {
		 Translate,
		 Rotate,
		 Scale,
		 NumManipulators
	 };

												kbManipulator();
												~kbManipulator();

	void										Update();
	void										RenderSync();

	void										ProcessInput( const bool leftMouseDown );

	bool										AttemptMouseGrab( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Quat4 & cameraOrientation );
	void										UpdateMouseDrag( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Quat4 & cameraOrientation );
	void										ReleaseFromMouseGrab() { m_SelectedGroup = -1; m_LastOrientation = m_Orientation; }
	bool										IsGrabbed() const { return m_SelectedGroup != -1; }

	void										SetPosition( const Vec3 & newPosition ) { m_Position = newPosition; }
	const Vec3 &								GetPosition() const { return m_Position; }

	void										SetOrientation( const Quat4 & newOrientation ) { m_Orientation = m_LastOrientation = newOrientation; }
	const Quat4 &								GetOrientation() const { return m_Orientation; }

	void										SetScale( const Vec3 & newScale ) { m_Scale = newScale; }
	const Vec3 &								GetScale() const { return m_Scale; }

	void										SetMode( const manipulatorMode_t newMode ) { m_ManipulatorMode = newMode; }
	manipulatorMode_t							GetMode() const { return m_ManipulatorMode; }

private:

	kbModel *									m_pModels[NumManipulators];

	manipulatorMode_t							m_ManipulatorMode;
	
	Vec3										m_Position;
	Quat4										m_Orientation;
	Vec3										m_Scale;

	Vec3										m_MouseWorldGrabPoint;
	Vec3										m_MouseLocalGrabPoint;
	Quat4										m_LastOrientation;
	Vec3										m_LastScale;

	int											m_SelectedGroup;

	std::vector<kbShaderParamOverrides_t>		m_ManipulatorMaterials;

	Vec4										m_NextTransformFromInput;

	// draw stuff
	Vec3 vecToGrabPoint;
	Vec3 vecToNewPoint;
};

#endif