//===================================================================================================
// EtherActor.cpp
//
//
// 2016-2019 kbEngine 2.0
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
	if (((EtherGame*)g_pGame)->TraceAgainstWorld( GetOwner()->GetPosition() + kbVec3( 0.0f, 10000.0f, 0.0f ), GetOwner()->GetPosition() - kbVec3( 0.0f, 10000.0f, 0.0f ), floorPosition, false ) ) {
		floorPosition.x = GetOwner()->GetPosition().x;
		floorPosition.y += offset;
		floorPosition.z = GetOwner()->GetPosition().z;
		GetOwner()->SetPosition( floorPosition );
	}
}

/**
 *	EtherActorComponent::StartDeath
 *	
 *	If a child class overrides StartDeath() without calling this one, it'll be responsible for calling g_pGame->RemoveGameEntity()
 */
void EtherActorComponent::StartDeath( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) {
	g_pGame->RemoveGameEntity( GetOwner() );
}

/**
 *	EtherComponentToggler::Constructor
 */
void EtherComponentToggler::Constructor() {
	m_MinFirstBurstDelaySec = 0.0f;
	m_MaxFirstBurstDelaySec = 0.0f;
	m_MinOnSeconds = 0.1f;
	m_MaxOnSeconds = 0.15f;
	m_MinOffSeconds = 1.0f;
	m_MaxOffSeconds = 3.0f;
	m_MinSecBetweenBursts = 0.1f;
	m_MaxSecBetweenBursts = 0.1f;
	m_MinNumOnBursts = 1;
	m_MaxNumOnBursts = 5;
 
	m_NextOnOffStartTime = 0.0f;
	m_NumBurstsLeft = 0;
	m_bComponentsEnabled = false;
	m_State = WaitingToBurst;
}

/**
 *	EtherComponentToggler::SetEnable_Internal
 */
void EtherComponentToggler::SetEnable_Internal( const bool bIsEnabled )  {
	if ( bIsEnabled ) {
		ToggleComponents( false );
		m_NextOnOffStartTime = kbfrand( m_MinFirstBurstDelaySec, m_MaxFirstBurstDelaySec );
		m_NumBurstsLeft = 0;//kbirand( m_MinNumOnBursts, m_MaxNumOnBursts );
		m_State = WaitingToBurst;
	}
}

/**
 *	EtherComponentToggler::Update_Internal
 */
void EtherComponentToggler::Update_Internal( const float DeltaTimeSeconds ) {

	const float curTime = g_GlobalTimer.TimeElapsedSeconds();

	switch ( m_State ) {

		// Waiting to burst
		case WaitingToBurst : {
			if ( curTime > m_NextOnOffStartTime ) {
				ToggleComponents( true );

				m_NextOnOffStartTime = g_GlobalTimer.TimeElapsedSeconds() + kbfrand( m_MinOnSeconds, m_MaxOnSeconds );
	
				m_NumBurstsLeft = kbirand( m_MinNumOnBursts, m_MaxNumOnBursts ) - 1;
				m_State = Bursting;
			}
			break;
		}

		case Bursting : {

			if ( curTime < m_NextOnOffStartTime ) {
				break;
			}

			if ( m_bComponentsEnabled ) {
				ToggleComponents( false );

				if ( m_NumBurstsLeft == 0 ) {
					m_State = WaitingToBurst;
					m_NextOnOffStartTime = g_GlobalTimer.TimeElapsedSeconds() + kbfrand( m_MinOffSeconds, m_MaxOffSeconds );
				} else {
					m_NextOnOffStartTime = g_GlobalTimer.TimeElapsedSeconds() + kbfrand( m_MinSecBetweenBursts, m_MaxSecBetweenBursts );
				}
				
			} else {
					ToggleComponents( true );
					m_NumBurstsLeft--;
					m_NextOnOffStartTime = g_GlobalTimer.TimeElapsedSeconds() + kbfrand( m_MinOnSeconds, m_MaxOnSeconds );		
			}
		}
	}
}

/**
 *	EtherComponentToggler::ToggleComponents
 */
void EtherComponentToggler::ToggleComponents( const bool bToggleOn ) {

	for ( int i = 1; i < this->GetOwner()->NumComponents(); i++ ) {
		kbGameComponent *const pComponent = GetOwner()->GetComponent( i );
		if ( pComponent == this ) {
			continue;
		}

		pComponent->Enable( bToggleOn );
	}

	m_bComponentsEnabled = bToggleOn;
}
