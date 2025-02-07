/// kbEditorEntity.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "Matrix.h"
#include "kbBounds.h"
#include "DX11/kbRenderer_DX11.h"
#include "kbGameEntityHeader.h"
#include "kbEditorEntity.h"

/// kbEditorEntity::kbEditorEntity
kbEditorEntity::kbEditorEntity() :
	m_pGameEntity( new kbGameEntity() ),
	m_bIsSelected( false ) {

}

/// kbEditorEntity::kbEditorEntity
kbEditorEntity::kbEditorEntity( kbGameEntity * pGameEntity ) :
	m_pGameEntity( pGameEntity ),
	m_bIsSelected( false ) {
}

/// kbEditorEntity::~kbEditorEntity
kbEditorEntity::~kbEditorEntity() {
	delete m_pGameEntity;
}

/// kbEditorEntity::GetPropertyMetaData
varMetaData_t *	kbEditorEntity::GetPropertyMetaData( const kbComponent * pComponent, const size_t Offset ) {
	if ( m_pGameEntity == NULL || pComponent == NULL )
		return NULL;

	std::string metaDataLookUp = std::to_string( ( UINT_PTR )pComponent);
	metaDataLookUp += "_";
	metaDataLookUp += std::to_string( ( unsigned int ) (Offset));
	
	return &m_PropertyMetaData[metaDataLookUp];
}

/// kbEditorEntity::Update
void kbEditorEntity::Update( const float DT ) {
	m_pGameEntity->Update( DT );
}

/// kbEditorEntity::RenderSync
void kbEditorEntity::RenderSync() {
	m_pGameEntity->RenderSync();
}

/// kbEditorEntity::GetWorldBounds
const kbBounds kbEditorEntity::GetWorldBounds() const {
	return m_pGameEntity->GetWorldBounds(); 
}

/// kbEditorEntity::GetPosition
const Vec3 kbEditorEntity::GetPosition() const {
	return m_pGameEntity->GetPosition(); 
}

/// kbEditorEntity::SetPosition
void kbEditorEntity::SetPosition( const Vec3 & newPosition ) {
	m_pGameEntity->SetPosition( newPosition ); 
}

/// kbEditorEntity::GetOrientation
const Quat4 kbEditorEntity::GetOrientation() const {
	return m_pGameEntity->GetOrientation(); 
}

/// kbEditorEntity::SetOrientation
void kbEditorEntity::SetOrientation( const Quat4 & newOrientation ) {
	m_pGameEntity->SetOrientation( newOrientation ); 
}

/// kbEditorEntity::GetScale
const Vec3 kbEditorEntity::GetScale() const {
	return m_pGameEntity->GetScale(); 
}

/// kbEditorEntity::SetScale
void kbEditorEntity::SetScale( const Vec3 & newScale ) {
	m_pGameEntity->SetScale( newScale ); 
}

/// kbEditorEntity::GetGameEntity
kbGameEntity * kbEditorEntity::GetGameEntity() const {
	return m_pGameEntity; 
}

