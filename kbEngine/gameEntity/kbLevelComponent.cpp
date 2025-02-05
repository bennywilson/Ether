/// kbLevelComponent.cpp
///
/// 2019-2025 blk 1.0

#include "kbCore.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"
#include "kbLevelComponent.h"

static const kbLevelComponent * g_pLevelComponent = nullptr;

/**
 *	kbLevelComponent::Constructor
 */
void kbLevelComponent::Constructor() {
	m_LevelType = LevelType_2D;
	m_GlobalModelScale = 1.0f;
	m_EditorIconScale = 1.0f;
	m_GlobalVolumeScale = 1.0f;

	g_pLevelComponent = this;
}

/**
 *	kbLevelComponent::~kbLevelComponent
 */
kbLevelComponent::~kbLevelComponent() {
	g_pLevelComponent = nullptr;
}

/**
 *	kbLevelComponent::SetEnable_Internal
 */
void kbLevelComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		g_pRenderer->SetWorldAndEditorIconScale( m_GlobalModelScale, m_EditorIconScale );
		g_pLevelComponent = this;	
	} else {
		g_pRenderer->SetWorldAndEditorIconScale( 1.0f, 1.0f );
		g_pLevelComponent = nullptr;
	}
}

/**
 *	kbLevelComponent::EditorChange
 */
void kbLevelComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "WorldScale" || propertyName == "IconScale" ) {
		g_pRenderer->SetWorldAndEditorIconScale( m_GlobalModelScale , m_EditorIconScale );
	}
}

/**
 *	kbLevelComponent::GetGlobalModelScale
 */
float kbLevelComponent::GetGlobalModelScale() {

	if ( g_pLevelComponent == nullptr ) {
		return 1;
	}

	return g_pLevelComponent->m_GlobalModelScale;
}

/**
 *	kbLevelComponent::GetEditorIconScale
 */
float kbLevelComponent::GetEditorIconScale() {

	if ( g_pLevelComponent == nullptr ) {
		return 1;
	}

	return g_pLevelComponent->m_EditorIconScale;
}

/**
 *	kbLevelComponent::GetGlobalVolumeScale
 */
float kbLevelComponent::GetGlobalVolumeScale() {

	if ( g_pLevelComponent == nullptr ) {
		return 1;
	}

	return g_pLevelComponent->m_GlobalVolumeScale;
}

/**
 *	kbCinematicAction
 */
void kbCinematicAction::Constructor() {
	m_fCineParam = 0.0f;
}

/**
 *	kbCinematicComponent::~kbCinematicComponent
 */
kbCinematicComponent::~kbCinematicComponent() {

}

/**
 *	kbCinematicComponent::Constructor
 */
void kbCinematicComponent::Constructor() {
}

/**
 *	kbCinematicComponent::SetEnable_Internal
 */
void kbCinematicComponent::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( true );
}

/**
 *	kbCinematicComponent::Update_Internal
 */
void kbCinematicComponent::Update_Internal( const float dt ) {
	
	Super::Update_Internal( dt );

}
