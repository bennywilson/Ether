//===================================================================================================
// kbLevelComponent.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"
#include "kbLevelComponent.h"

float g_GlobalModelScale = 1.0f;
float g_EditorIconScale = 1.0f;

/**
 *	kbLevelComponent::Constructor
 */
void kbLevelComponent::Constructor() {
	m_LevelType = LevelType_2D;
	m_GlobalModelScale = 1.0f;
	m_EditorIconScale = 1.0f;
}

/**
 *	kbLevelComponent::SetEnable_Internal
 */
void kbLevelComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		g_pRenderer->SetWorldAndEditorIconScale( m_GlobalModelScale, m_EditorIconScale );
		g_GlobalModelScale = m_GlobalModelScale;
		g_EditorIconScale = m_EditorIconScale;	
	} else {
		g_pRenderer->SetWorldAndEditorIconScale( 1.0f, 1.0f );

		g_GlobalModelScale = 1.0f;
		g_EditorIconScale = 1.0f;
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
