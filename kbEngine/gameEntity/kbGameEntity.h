//===================================================================================================
// kbGameEntity.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBGAMEENTITY_H_
#define _KBGAMEENTITY_H_

class kbComponent;
class kbTransformComponent;

#define INVALID_ENTITYID UINT_MAX

bool operator<( const kbGUID & a, const kbGUID & b );

/**
 *	kbGameEntityPtr - All entities have a unique m_EntityIndex per game instance.  Entities loaded from disk (ex. from a .kblevel or .kbPkg file) will have m_GUID set
 */
class kbGameEntityPtr {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbGameEntityPtr() : m_EntityIndex( INVALID_ENTITYID ) { }

	bool										operator==( const kbGameEntityPtr op2 ) const { return m_EntityIndex == op2.m_EntityIndex; }

	void										SetEntity( const kbGUID & guid );
	void										SetEntity( kbGameEntity *const pGameEntity );
	kbGameEntity *								GetEntity();
	const kbGameEntity *						GetEntity() const;

	kbGUID										GetGUID() const;

	int											GetEntityIndex() const { return m_EntityIndex; }

private:

	kbGUID										m_GUID;
	uint										m_EntityIndex;
};

/**
 *	kbGameEntity
 */
class kbGameEntity {

	friend class kbGameEntityPtr;
	friend class kbGame;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

												kbGameEntity( const kbGUID *const guid = nullptr, const bool bIsPrefab = false );
												kbGameEntity( const kbGameEntity *, const bool bIsPrefab, const kbGUID *const guid = nullptr );

	virtual										~kbGameEntity();

	void										PostLoad();

	// Children
	void										AddComponent( kbComponent *const pComponent, int indexToInsertAt = -1 );
	void										RemoveComponent( kbComponent *const pComponent );
	void										AddEntity( kbGameEntity *const pEntity );

	// Updates
	void										Update( const float DeltaTime );
	void										DisableAllComponents();

	void										RenderSync();

	// Accessors
	const std::string &							GetName() const { return m_pTransformComponent->GetName(); }

	const kbVec3								GetPosition() const;
	void										SetPosition( const kbVec3 & newPosition ) { MarkAsDirty(); m_pTransformComponent->SetPosition( newPosition ); }

	const kbQuat								GetOrientation() const;
	void										SetOrientation( const kbQuat & newOrientation ) { m_bIsDirty = true; m_pTransformComponent->SetOrientation( newOrientation ); }

	const kbVec3								GetScale() const { return m_pTransformComponent->GetScale(); }
	void										SetScale( const kbVec3 & newScale ) { m_bIsDirty = true; m_pTransformComponent->SetScale( newScale ); }

	void										CalculateWorldMatrix( kbMat4 & worldMatrix );

	const kbBounds &							GetBounds() const { return m_Bounds; }
	kbBounds									GetWorldBounds() const;
	
	void										MarkAsDirty() { m_bIsDirty = true; for ( int i = 0; i < m_Components.size(); i++ ) { m_Components[i]->m_bIsDirty = true; } }
	bool										IsDirty() const { return m_bIsDirty; }
	bool										IsPrefab() const { return m_bIsPrefab; }

	void										DeleteWhenComponentsAreInactive( const bool bDelete ) { m_bDeleteWhenComponentsAreInactive = bDelete; }

	kbGameEntity *								GetParent() const { return m_pParentEntity; }

	kbActorComponent *							GetActorComponent() const { return m_pActorComponent; }
	kbComponent *								GetComponent( const size_t index ) const { return m_Components[index]; }
	size_t										NumComponents() const { return m_Components.size(); }
	kbComponent *								GetComponentByType( const void *const pTypeInfoClass ) const;

	const std::vector<kbGameEntity*> &			GetChildEntities() const { return m_ChildEntities; }

	const kbGUID &								GetGUID() const { return m_GUID; }
	const uint									GetEntityId() const { return m_EntityId; }

private:

	kbBounds									m_Bounds;

	std::vector<kbComponent *>					m_Components;
	kbTransformComponent *						m_pTransformComponent;		// For convenience.  This is always the first entry in the m_Components list
	kbActorComponent *							m_pActorComponent;
	std::vector<kbGameEntity*>					m_ChildEntities;
	kbGameEntity *								m_pParentEntity;

	// All entities will have a m_EntityId.  They're temporary values that may differ between game instances
	uint										m_EntityId;

	// Entities that came from file (level, package, etc) have a GUID
	kbGUID										m_GUID;

	bool										m_bIsDirty							: 1;
	bool										m_bIsPrefab							: 1;
	bool										m_bDeleteWhenComponentsAreInactive	: 1;
};

/**
 *	kbPrefab
 */
class kbPrefab {
	friend class kbEditor;
	friend class kbResourceManager;
	friend class kbFile;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

												kbPrefab( const kbPrefab * prefabToCopy );
												~kbPrefab() { }

	const std::string &							GetPrefabName() const { return m_PrefabName; }
	const size_t								NumGameEntities() const { return m_GameEntities.size(); }
	const kbGameEntity *						GetGameEntity( const int idx ) const { return m_GameEntities[idx]; }

private:
												kbPrefab() { }

	UUID										m_GUID;

	std::string									m_PrefabName;
	std::vector<kbGameEntity *>					m_GameEntities;
};
#endif