//===================================================================================================
// kbGameEntity.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGame.h"
#include "kbGameEntityHeader.h"

/**
 *	kbEntity::kbEntity
 */
kbEntity::kbEntity() {
}

/**
 *	kbEntity::AddComponent
 */
void kbEntity::AddComponent( kbComponent *const pComponent, int indexToInsertAt ) {
	pComponent->SetParent( this );

	const int lastComponentIdx = (int)m_Components.size();
	if ( indexToInsertAt == -1 ) {
		indexToInsertAt = lastComponentIdx;
	}

	if ( pComponent->IsA( kbGameLogicComponent::GetType() ) ) {
		indexToInsertAt = lastComponentIdx;
	} else if ( m_Components.size() > 0 && indexToInsertAt == lastComponentIdx ) {
		while( indexToInsertAt > 0 && m_Components[indexToInsertAt-1]->IsA( kbGameLogicComponent::GetType() ) ) {
			indexToInsertAt--;
		}
	}

	if ( indexToInsertAt < 0 || indexToInsertAt >= m_Components.size() ) {
		m_Components.push_back( pComponent );
	} else {
		m_Components.insert( m_Components.begin() + indexToInsertAt, pComponent );
	}
}


/**
 *	kbEntity::RemoveComponent
 */
void kbEntity::RemoveComponent( kbComponent *const pComponent ) {

	// TODO: This might move the game logic component from the back of the list!
	FastRemoveFromVector( m_Components, pComponent );
}

//===================================================================================================
//	kbGameEntityPtr
//===================================================================================================
bool operator<( const kbGUID & a, const kbGUID & b ) {
	return memcmp( &a, &b, sizeof(b) ) < 0;
}

struct EntPtrHash {
	size_t operator()( const kbGUID & op ) const {
		return std::hash<int>()( op.m_iGuid[0] ) ^ std::hash<int>()( op.m_iGuid[1] ) ^ std::hash<int>()( op.m_iGuid[2] ) ^ std::hash<int>()( op.m_iGuid[3] );
	}
};

std::unordered_map<kbGUID, kbGameEntity *, EntPtrHash> g_GUIDToEntityMap;
std::unordered_map<int, kbGameEntity *> g_IndexToEntityMap;

/**
 *	kbGameEntityPtr::SetEntity
 */
void kbGameEntityPtr::SetEntity( const kbGUID & guid ) {

	m_GUID = guid;
	m_EntityIndex = INVALID_ENTITYID;

	std::unordered_map<kbGUID, kbGameEntity *, EntPtrHash>::const_iterator GUIDToEntityIt = g_GUIDToEntityMap.find( m_GUID );
	if ( GUIDToEntityIt == g_GUIDToEntityMap.end() ) {
		g_GUIDToEntityMap[guid] = nullptr;
	} else {
		if ( GUIDToEntityIt->second != nullptr ) {
			m_EntityIndex = GUIDToEntityIt->second->GetEntityId();
		}
	}
}

/**
 *	kbGameEntityPtr::SetEntity
 */
void kbGameEntityPtr::SetEntity( kbGameEntity *const pGameEntity ) {

	if ( pGameEntity == nullptr ) {
		m_EntityIndex = INVALID_ENTITYID;
		ZeroMemory( &m_GUID, sizeof( m_GUID ) );
		return;
	}

	// Enter into guid map
	m_GUID = pGameEntity->GetGUID();

	if ( m_GUID.IsValid() ) {
		std::unordered_map<kbGUID, kbGameEntity *, EntPtrHash>::const_iterator GUIDToEntityIt = g_GUIDToEntityMap.find( m_GUID );

		if ( GUIDToEntityIt != g_GUIDToEntityMap.cend() && GUIDToEntityIt->second != pGameEntity && GUIDToEntityIt->second != nullptr ) {

			kbError( "kbGameEntityPtr::SetEntity() - Entities %s && %s share the same guid - %u %u %u %u", 
					  pGameEntity->GetName().c_str(), GUIDToEntityIt->second->GetName().c_str(),
					  m_GUID.m_iGuid[0], m_GUID.m_iGuid[1], m_GUID.m_iGuid[2], m_GUID.m_iGuid[3] );
		}

		g_GUIDToEntityMap[m_GUID] = pGameEntity;
	}

	// Enter into entity index map
	m_EntityIndex = pGameEntity->m_EntityId;

	std::unordered_map<int, kbGameEntity *>::const_iterator IDToEntityIt = g_IndexToEntityMap.find( m_EntityIndex );

	if ( IDToEntityIt != g_IndexToEntityMap.cend() && IDToEntityIt->second != pGameEntity && IDToEntityIt->second != nullptr ) {
		kbError( "kbGameEntityPtr::SetEntity() - Entities %s && %s share the same guid - %u %u %u %u", 
				 pGameEntity->GetName().c_str(), IDToEntityIt->second->GetName().c_str(),
				 m_GUID.m_iGuid[0], m_GUID.m_iGuid[1], m_GUID.m_iGuid[2], m_GUID.m_iGuid[3] );
	}

	g_IndexToEntityMap[m_EntityIndex] = pGameEntity;
}

/**
 *	kbGameEntityPtr::GetEntity
 */
kbGameEntity * kbGameEntityPtr::GetEntity() {

	if ( m_EntityIndex != INVALID_ENTITYID ) {

		std::unordered_map<int, kbGameEntity *>::const_iterator it = g_IndexToEntityMap.find( m_EntityIndex );
		if ( it != g_IndexToEntityMap.cend() ) {
			return it->second;
		}
	}

	std::unordered_map<kbGUID, kbGameEntity *, EntPtrHash>::const_iterator it = g_GUIDToEntityMap.find( m_GUID );
	if ( it == g_GUIDToEntityMap.cend() ) {
		return nullptr;
	}

	return it->second;
}

const kbGameEntity * kbGameEntityPtr::GetEntity() const {
	kbGameEntityPtr *const pThisNoConst = const_cast<kbGameEntityPtr*>( this );
	return pThisNoConst->GetEntity();
}

/**
 *	kbGameEntityPtr::GetGUID
 */
kbGUID kbGameEntityPtr::GetGUID() const {

	return m_GUID;
}

//===================================================================================================
//	kbGameEntity
//===================================================================================================

uint g_EntityNumber = 0;

/**
 *	kbGameEntity
 */
kbGameEntity::kbGameEntity( const kbGUID *const guid, const bool bIsPrefab ) :
	m_Bounds( kbVec3( -1.0f, -1.0f, -1.0f ), kbVec3( 1.0f, 1.0f, 1.0f ) ),
	m_pActorComponent( nullptr ),
	m_pParentEntity( nullptr ),
	m_EntityId( g_EntityNumber++ ),
	m_bIsDirty( false ),
	m_bIsPrefab( bIsPrefab ),
	m_bDeleteWhenComponentsAreInactive( false ) {

	m_pTransformComponent = new kbTransformComponent();
	m_pTransformComponent->SetPosition( kbVec3::zero );
	AddComponent( m_pTransformComponent );

	if ( bIsPrefab == false ) {
		const std::string newName = "Entity_" + std::to_string( m_EntityId );
		m_pTransformComponent->SetName( newName.c_str() );

		if ( guid != nullptr ) {
			m_GUID = *guid;
		}
	} else {
		m_pTransformComponent->SetName( "Prefab" );

		if ( guid != nullptr ) {
			m_GUID = *guid;
		} else if ( g_UseEditor == true ) {
			CoCreateGuid( &m_GUID.m_Guid );
		} else {
			kbError( "kbGameEntity::kbGameEntity() - Prefab created with invalid GUID" );
		}
	}

	kbGameEntityPtr entityPtr;
	entityPtr.SetEntity( this );
}

/**
 *	kbGameEntity::kbGameEntity( const kbGameEntity * )
 */
kbGameEntity::kbGameEntity( const kbGameEntity * pGameEntity, const bool bIsPrefab, const kbGUID *const guid ) :
	m_Bounds( pGameEntity->GetBounds() ),
	m_pActorComponent( nullptr ),
	m_pParentEntity( nullptr ),
	m_EntityId( g_EntityNumber++ ),
	m_bIsDirty( false ),
	m_bIsPrefab( bIsPrefab ),
	m_bDeleteWhenComponentsAreInactive( false ) {

	for ( int i = 0; i < pGameEntity->m_Components.size(); i++ ) {
		const kbTypeInfoClass *const pTypeInfoClass = g_NameToTypeInfoMap->GetTypeInfoFromClassName( pGameEntity->m_Components[i]->GetComponentClassName() );
		kbComponent * newComponent = pTypeInfoClass->ConstructInstance( pGameEntity->m_Components[i] );
		AddComponent( newComponent );

		if ( i == 0 ) {
			m_pTransformComponent = (kbTransformComponent*)newComponent;
			if ( m_pTransformComponent->IsA( kbTransformComponent::GetType() ) == false ) {
				kbError( "kbGameEntity::kbGameEntity() - Somehow the first component is not the transform component" );
			}
		}

		if ( m_bIsPrefab == false ) {
			if ( pGameEntity->m_Components[i]->IsEnabled() == true ) {
				newComponent->Enable( false );
				newComponent->Enable( true );
			} else {
				newComponent->Enable( false );
			}
		}
	}

	if ( bIsPrefab == false ) {
		if ( guid != nullptr ) {
			m_GUID = *guid;
		}
	} else {
		if ( guid != nullptr ) {
			m_GUID = *guid;
		} else if ( g_UseEditor == true ) {
			CoCreateGuid( &m_GUID.m_Guid );
		} else {
			kbError( "kbGameEntity::kbGameEntity() - Prefab created with invalid GUID" );
		}
	}

	kbGameEntityPtr entityPtr;
	entityPtr.SetEntity( this );
}

/**
 *	kbGameEntity::~kbGameEntity
 */
kbGameEntity::~kbGameEntity() {
	for ( int i = 0; i < m_Components.size(); i++ ) {
		m_Components[i]->Enable( false );
		delete m_Components[i];
	}

	for ( int i = 0; i < m_ChildEntities.size(); i++ ) {
		delete m_ChildEntities[i];
	}

	if ( m_GUID.IsValid() ) {
		g_GUIDToEntityMap.erase( m_GUID );
	} 

	kbErrorCheck( m_EntityId != INVALID_ENTITYID, "kbGameEntity::~kbGameEntity() - Destroying entity with an invalid entity id" );
	g_IndexToEntityMap.erase( m_EntityId );
}

/**
 *	kbGameEntity::PostLoad
 */
void kbGameEntity::PostLoad() {
	for ( int i = 0; i < m_Components.size(); i++ ) {
		m_Components[i]->PostLoad();
	}
}

/**
 *	kbGameEntity::AddComponent
 */
void kbGameEntity::AddComponent( kbComponent *const pComponent, int indexToInsertAt ) {

	if ( pComponent->IsA( kbActorComponent::GetType() ) ) {
		if ( m_pActorComponent != nullptr ) {
			kbError( "%s is trying to add multiple kbGameLogicComponent.", *GetName().c_str() );
			return;
		}

		m_pActorComponent = static_cast<kbActorComponent*>( pComponent );
	}

	kbEntity::AddComponent( pComponent, indexToInsertAt );

	if ( m_bIsPrefab == false ) {
		if ( pComponent->IsEnabled() ) {
			pComponent->Enable( false );
			pComponent->Enable( true );
		} else {
			pComponent->Enable( false );
		}
	}
}

/**
 *	kbGameEntity::AddEntity
 */
void kbGameEntity::AddEntity( kbGameEntity *const pEntity ) {
	pEntity->m_pParentEntity = this;
	m_ChildEntities.push_back( pEntity );

	// Make sure pEntity is not in kbGame's list as it will now be managed by this
	g_pGame->RemoveGameEntity( pEntity );
}

/**
 *	kbGameEntity::Update
 */
void kbGameEntity::Update( const float DeltaTime ) {
	START_SCOPED_TIMER( GAME_ENTITY_UPDATE )

	{
		START_SCOPED_TIMER( COMPONENT_UPDATE )

		for ( int i = 0; i < m_Components.size(); i++ ) {
			// todo: make sure entity is still valid before updating the next component (ex. projectile may have removed the entity)
			if ( GetComponent(i)->IsEnabled() ) {
				GetComponent(i)->Update( DeltaTime );
			}
		}
	}

	for ( int i = 0; i < m_ChildEntities.size(); i++ ) {
		m_ChildEntities[i]->Update( DeltaTime );
	}

	if ( m_bDeleteWhenComponentsAreInactive ) {
		bool bActiveComponentsExist = false;
		for ( int i = 1; i < m_Components.size(); i++ ) {
			if ( m_Components[i]->IsEnabled() ) {
				bActiveComponentsExist = true;
				break;
			}
		}

		if ( bActiveComponentsExist == false ) {
			g_pGame->RemoveGameEntity( this );
			return;
		}		
	}

	m_bIsDirty = false;
}

/**
 *	kbGameEntity::DisableAllComponents
 */
void kbGameEntity::DisableAllComponents() {
	for ( int i = 0; i < m_Components.size(); i++ ) {
		m_Components[i]->Enable( false );
	}
}

/**
 *	kbGameEntity::RenderSync
 */
void kbGameEntity::RenderSync() {
	
	for ( int i = 0; i < m_Components.size(); i++ ) {
		m_Components[i]->RenderSync();
	}

	for ( int i = 0; i < m_ChildEntities.size(); i++ ) {
		m_ChildEntities[i]->RenderSync();
	}
}

/**
 *	kbGameEntity::CalculateWorldMatrix
 */
void kbGameEntity::CalculateWorldMatrix( kbMat4 & inOutMatrix ) const {

	kbMat4 scaleMat( kbMat4::identity );
	scaleMat[0].x = GetScale().x;
	scaleMat[1].y = GetScale().y;
	scaleMat[2].z = GetScale().z;

	inOutMatrix = scaleMat * GetOrientation().ToMat4();
	inOutMatrix[3] = GetPosition();
	inOutMatrix[3].w = 1.0f;
}

/**
 *	kbGameEntity::GetWorldBounds
 */
kbBounds kbGameEntity::GetWorldBounds() const {
	kbBounds returnBounds = m_Bounds;
	returnBounds.Scale( GetScale() );
	returnBounds.Translate( GetPosition() ); 
	return returnBounds; 
}

/**
 *	kbGameEntity::GetOrientation
 */
const kbQuat kbGameEntity::GetOrientation() const {
	if ( m_pParentEntity != nullptr ) {
		// This entity's orientation is in model space while the parent's is in world
		return  m_pTransformComponent->GetOrientation() * m_pParentEntity->GetOrientation();
	}

	return m_pTransformComponent->GetOrientation(); 
}

/**
 *	kbGameEntity::GetPosition
 */
const kbVec3 kbGameEntity::GetPosition() const {
	if ( m_pParentEntity != nullptr ) {
		const kbQuat entityOrientation = GetOrientation();
		const kbVec3 worldSpaceOffset = entityOrientation.ToMat4().TransformPoint( m_pTransformComponent->GetPosition() );
		return worldSpaceOffset + m_pParentEntity->GetPosition();
	}

	return m_pTransformComponent->GetPosition();
}

/**
 *	kbGameEntity::GetComponent
 */
kbComponent * kbGameEntity::GetComponentByType( const void *const pTypeInfoClass ) const {
	if ( pTypeInfoClass == nullptr ) {
		return nullptr;
	}

	for ( int i = 0; i < m_Components.size(); i++ ) {
		if ( m_Components[i]->IsA( pTypeInfoClass ) ) {
			return m_Components[i];
		}
	}

	return nullptr;
}


/**
 *	kbPrefab::kbPrefab
 */
kbPrefab::kbPrefab( const kbPrefab * prefabToCopy ) {
}
