/// kbLevelComponent.cpp
///
/// 2019-2025 blk 1.0

#include "blk_core.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"
#include "kbLevelComponent.h"

static const kbLevelComponent * g_pLevelComponent = nullptr;

/// kbLevelComponent::Constructor
void kbLevelComponent::Constructor() {
	m_LevelType = LevelType_2D;
	m_GlobalModelScale = 1.0f;
	m_EditorIconScale = 1.0f;
	m_GlobalVolumeScale = 1.0f;

	g_pLevelComponent = this;
}

/// kbLevelComponent::~kbLevelComponent
kbLevelComponent::~kbLevelComponent() {
	g_pLevelComponent = nullptr;
}

/// kbLevelComponent::enable_internal
void kbLevelComponent::enable_internal( const bool bEnable ) {
	Super::enable_internal( bEnable );

	if ( bEnable ) {
		g_pRenderer->SetWorldAndEditorIconScale( m_GlobalModelScale, m_EditorIconScale );
		g_pLevelComponent = this;	
	} else {
		g_pRenderer->SetWorldAndEditorIconScale( 1.0f, 1.0f );
		g_pLevelComponent = nullptr;
	}
}

/// kbLevelComponent::EditorChange
void kbLevelComponent::editor_change( const std::string & propertyName ) {
	Super::editor_change( propertyName );

	if ( propertyName == "WorldScale" || propertyName == "IconScale" ) {
		g_pRenderer->SetWorldAndEditorIconScale( m_GlobalModelScale , m_EditorIconScale );
	}
}

/// kbLevelComponent::GetGlobalModelScale
float kbLevelComponent::GetGlobalModelScale() {

	if ( g_pLevelComponent == nullptr ) {
		return 1;
	}

	return g_pLevelComponent->m_GlobalModelScale;
}

/// kbLevelComponent::GetEditorIconScale
float kbLevelComponent::GetEditorIconScale() {

	if ( g_pLevelComponent == nullptr ) {
		return 1;
	}

	return g_pLevelComponent->m_EditorIconScale;
}

/// kbLevelComponent::GetGlobalVolumeScale
float kbLevelComponent::GetGlobalVolumeScale() {

	if ( g_pLevelComponent == nullptr ) {
		return 1;
	}

	return g_pLevelComponent->m_GlobalVolumeScale;
}

/// kbCinematicAction
void kbCinematicAction::Constructor() {
	m_fCineParam = 0.0f;
}

/// kbCinematicComponent::~kbCinematicComponent
kbCinematicComponent::~kbCinematicComponent() {

}

/// kbCinematicComponent::Constructor
void kbCinematicComponent::Constructor() {
}

/// kbCinematicComponent::enable_internal
void kbCinematicComponent::enable_internal( const bool bEnable ) {

	Super::enable_internal( true );
}

/// kbCinematicComponent::update_internal
void kbCinematicComponent::update_internal( const float dt ) {
	
	Super::update_internal( dt );

}
