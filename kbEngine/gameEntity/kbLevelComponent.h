//===================================================================================================
// kbLevelComponent.h
//
// 2019 kbEngine 2.0
//===================================================================================================
#ifndef _KBLEVELCOMPONENT_H_
#define _KBLEVELCOMPONENT_H_

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

	// Editor
	ELevelType									m_LevelType;
	float										m_GlobalModelScale;
	float										m_EditorIconScale;
	float										m_GlobalVolumeScale;
};

#endif