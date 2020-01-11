//===================================================================================================
// kbLevelComponent.h
//
// 2019 kbEngine 2.0
//===================================================================================================
#ifndef _KBLEVELCOMPONENT_H_
#define _KBLEVELCOMPONENT_H_

#include "kbGameEntity.h"

/**
 *	ELevelType
 */
enum ELevelType {
	LevelType_Menu,
	LevelType_2D
};

/**
 *	kbLevelComponent
 */
class kbLevelComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbLevelComponent, kbGameComponent )

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

												~kbLevelComponent();

	ELevelType									GetLevelType() const { return m_LevelType; }

	static float								GetGlobalModelScale();
	static float								GetEditorIconScale();
	static float								GetGlobalVolumeScale();

protected:

	virtual void								EditorChange( const std::string & propertyName ) override;

	virtual void								SetEnable_Internal( const bool bEnable ) override;

private:

	virtual void								UpdateDebugAndCheats() { }

	// Editor
	ELevelType									m_LevelType;
	float										m_GlobalModelScale;
	float										m_EditorIconScale;
	float										m_GlobalVolumeScale;
};

/**
 *	eCinematicActionType
 */
enum eCinematicActionType {
	CineAction_Override,
	CineAction_Animate,
	CineAction_MoveTo
};

/**
 *	kbCinematicAction
 */
class kbCinematicAction : public kbGameComponent {
public:
	friend class kbCinematicComponent;

	KB_DECLARE_COMPONENT( kbCinematicAction, kbGameComponent );

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
private:

	eCinematicActionType						m_CineActionType;
	kbString									m_sCineParam;
	float										m_fCineParam;
	kbGameEntityPtr								m_pCineParam;
	kbVec3										m_vCineParam;

	float										m_ActionStartTime;
	float										m_ActionDuration;
};

/**
 *	kbCinematicComponent
 */
class kbCinematicComponent : public kbGameComponent {

//---------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( kbCinematicComponent, kbGameComponent );

	virtual										~kbCinematicComponent();

protected:

	void										SetEnable_Internal( const bool bEnable ) override;
	void										Update_Internal( const float dt ) override;

private:

	std::vector<kbCinematicAction>				m_Actions;
};

#endif