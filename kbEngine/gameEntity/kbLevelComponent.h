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

protected:
	virtual void								EditorChange( const std::string & propertyName ) override;

	virtual void								SetEnable_Internal( const bool bEnable ) override;

private:
	ELevelType									m_LevelType;

	float										m_GlobalModelScale;
	float										m_EditorIconScale;
};


extern float g_GlobalModelScale;
extern float g_EditorIconScale;
#endif