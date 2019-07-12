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

/**
 *	kbLevelComponent::Constructor
 */
void kbLevelComponent::Constructor() {
	m_LevelType = LevelType_2D;
	m_WorldScale = 1.0f;
	m_EditorIconScale = 1.0f;
}

/**
 *	kbLevelComponent::SetEnable_Internal
 */
void kbLevelComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	g_pRenderer->SetWorldAndEditorIconScale( m_WorldScale, m_EditorIconScale );
}

/**
 *	kbLevelComponent::EditorChange
 */
void kbLevelComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "WorldScale" || propertyName == "IconScale" ) {
		g_pRenderer->SetWorldAndEditorIconScale( m_WorldScale , m_EditorIconScale );
	}
}
