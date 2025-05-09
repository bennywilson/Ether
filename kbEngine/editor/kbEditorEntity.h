//===================================================================================================
// kbEditorEntity.h
//
//
// 2016-2018 blk 1.0
//===================================================================================================
#ifndef _KBEDITORENTITY_H_
#define _KBEDITORENTITY_H_

// The editor will use this to store extra information about displayed properties
struct varMetaData_t {
	varMetaData_t() :
		bExpanded( false ) { }

	bool bExpanded;
};

/// kbEditorEntity
class kbEditorEntity {

	friend class kbEditor;

//---------------------------------------------------------------------------------------------------
public:

												kbEditorEntity();
												kbEditorEntity( kbGameEntity *const );
												~kbEditorEntity();

	void										Update( const float DT );
	void										RenderSync();

	bool										IsSelected() const { return m_bIsSelected; }
	void										SetIsSelected( bool bIsSelected ) { m_bIsSelected = bIsSelected; }

	const kbBounds								GetWorldBounds() const;

	const Vec3								GetPosition() const;
	void										SetPosition( const Vec3 & newPosition );

	const Quat4								GetOrientation() const;
	void										SetOrientation( const Quat4 & newOrientation );

	const Vec3								GetScale() const;
	void										SetScale( const Vec3 & newScale );

	kbGameEntity *								GetGameEntity() const;
	void										SetGameEntity( kbGameEntity *const gameEntity ) { m_pGameEntity = gameEntity; m_PropertyMetaData.clear(); }

	varMetaData_t *								GetPropertyMetaData( const kbComponent * pComponent, const size_t Offset );

private:

	kbGameEntity *								m_pGameEntity;

	std::map< std::string, varMetaData_t >		m_PropertyMetaData;

	bool										m_bIsSelected : 1;
};

#endif