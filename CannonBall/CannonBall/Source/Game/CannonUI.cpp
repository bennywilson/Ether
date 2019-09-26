//===================================================================================================
// CannonUI.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonUI.h"


/**
 *	CannonHealthBarUIComponent::Constructor
 */
 void CannonHealthBarUIComponent::Constructor() {
	m_Dummy = 0;

	m_TargetNormalizedHealth = 1.0f;
	m_CurrentNormalizedHealth = 1.0f;
	m_pStaticModelComponent = nullptr;
}

/**
 *	CannonHealthBarUIComponent::SetEnable_Internal
 */
void CannonHealthBarUIComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	m_pStaticModelComponent = nullptr;

	if ( bEnable ) {
		m_pStaticModelComponent = GetOwner()->GetComponent<kbStaticModelComponent>();
	}
}

/**
 *	CannonHealthBarUIComponent::Update_Internal
 */
void CannonHealthBarUIComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	if ( m_pStaticModelComponent != nullptr ) {
		static kbString normalizedHealth( "normalizedHealth" );
		m_pStaticModelComponent->SetMaterialParamVector( 0, normalizedHealth.stl_str(), kbVec4( m_TargetNormalizedHealth, 0.0f, 0.0f, 0.0f ) );
	}
}

/**
 *	CannonHealthBarUIComponent::SetTargetHealth
 */
void CannonHealthBarUIComponent::SetTargetHealth( const float newHealth ) {
	m_TargetNormalizedHealth = newHealth;
}
