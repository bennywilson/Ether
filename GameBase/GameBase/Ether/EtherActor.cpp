//===================================================================================================
// EtherActor.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "EtherGame.h"
#include "EtherActor.h"
#include "EtherSkelModel.h"
#include "EtherAI.h"
#include "EtherWeapon.h"


/**
 *	EtherActorComponent::Constructor
 */
void EtherActorComponent::Constructor() {
	m_UpperBodyState = UBS_Idle;
	m_pEquippedItem = NULL;
	m_GroundHoverDist = 0.5f;
}

/**
 *	EtherActorComponent::TakeDamage
 */
void EtherActorComponent::TakeDamage( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) {
	const float previousHealth = GetHealth();

	if ( IsDead() ) {
		return;
	}

	Super::TakeDamage( damageComponent, attackerComponent );

	// Check if this actor died from this damage
	if ( previousHealth > 0.0f && GetHealth() <= 0.0f ) {
		g_pEtherGame->GetAIManager().UnregisterCombatant( this );
		StartDeath( damageComponent, attackerComponent );
	}
}

/**
 *	EtherActorComponent::SetEnable_Internal
 */
void EtherActorComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	if ( isEnabled ) {
		g_pEtherGame->GetAIManager().RegisterCombatant( this );
	} else {
		g_pEtherGame->GetAIManager().UnregisterCombatant( this );
	}
}

/**
 *	EtherActorComponent::PlaceOnGround
 */
void EtherActorComponent::PlaceOnGround( const float offset ) {
	kbVec3 floorPosition;
	if (((EtherGame*)g_pGame)->TraceAgainstWorld( m_pParent->GetPosition() + kbVec3( 0.0f, 10000.0f, 0.0f ), m_pParent->GetPosition() - kbVec3( 0.0f, 10000.0f, 0.0f ), floorPosition, false ) ) {
		floorPosition.x = m_pParent->GetPosition().x;
		floorPosition.y += offset;
		floorPosition.z = m_pParent->GetPosition().z;
		m_pParent->SetPosition( floorPosition );
	}
}

/**
 *	EtherActorComponent::StartDeath
 *	
 *	If a child class overrides StartDeath() without calling this one, it'll be responsible for calling g_pGame->RemoveGameEntity()
 */
void EtherActorComponent::StartDeath( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) {
	g_pGame->RemoveGameEntity( GetParent() );
}