//===================================================================================================
// kbManipulator.h
//
//
// 2016-2018 kbEngine 2.0
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

	bool										AttemptMouseGrab( const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbQuat & cameraOrientation );
	void										UpdateMouseDrag( const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbQuat & cameraOrientation );
	void										ReleaseFromMouseGrab() { m_SelectedGroup = -1; m_LastOrientation = m_Orientation; }
	bool										IsGrabbed() const { return m_SelectedGroup != -1; }

	void										SetPosition( const kbVec3 & newPosition ) { m_Position = newPosition; }
	const kbVec3 &								GetPosition() const { return m_Position; }

	void										SetOrientation( const kbQuat & newOrientation ) { m_Orientation = m_LastOrientation = newOrientation; }
	const kbQuat &								GetOrientation() const { return m_Orientation; }

	void										SetScale( const kbVec3 & newScale ) { m_Scale = newScale; }
	const kbVec3 &								GetScale() const { return m_Scale; }

	void										SetMode( const manipulatorMode_t newMode ) { m_ManipulatorMode = newMode; }

private:

	kbModel *									m_pModels[NumManipulators];

	manipulatorMode_t							m_ManipulatorMode;
	
	kbVec3										m_Position;
	kbQuat										m_Orientation;
	kbVec3										m_Scale;

	kbVec3										m_MouseWorldGrabPoint;
	kbVec3										m_MouseLocalGrabPoint;
	kbQuat										m_LastOrientation;
	kbVec3										m_LastScale;

	int											m_SelectedGroup;


	// draw stuff
	kbVec3 vecToGrabPoint;
	kbVec3 vecToNewPoint;
};

#endif