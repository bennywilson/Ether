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
	ELevelType									GetLevelType() const { return m_LevelType; }

	static float								GetGlobalModelScale() { return s_GlobalModelScale; }
	static float								GetEditorIconScale() { return s_EditorIconScale; }

protected:
	virtual void								EditorChange( const std::string & propertyName ) override;

	virtual void								SetEnable_Internal( const bool bEnable ) override;

private:

	// Editor
	ELevelType									m_LevelType;
	float										m_GlobalModelScale;
	float										m_EditorIconScale;

	// Run time
	static float								s_GlobalModelScale;
	static float								s_EditorIconScale;
};

#endif