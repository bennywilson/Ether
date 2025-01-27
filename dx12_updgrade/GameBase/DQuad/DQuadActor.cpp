//===================================================================================================
// DQuadActor.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "kbNetworkingManager.h"
#include "DQuadGame.h"
#include "DQuadActor.h"
#include "DQuadSkelModel.h"
#include "DQuadAI.h"
#include "DQuadWeapon.h"

/**
 *	kbDQuadActorComponent::Constructor
 */
void kbDQuadActorComponent::Constructor() {
	m_UpperBodyState = UBS_Idle;
	m_pEquippedItem = NULL;
}

/**
 *	kbDQuadActorComponent::TakeDamage
 */
void kbDQuadActorComponent::TakeDamage( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) {
	const float previousHealth = GetHealth();

	if ( damageComponent == NULL || damageComponent->GetParent()->GetNetOwner() != NetOwner_Server ) {

		// Clients inform the server if they've hit something
		int damageComponentOwner = -1;
		if ( damageComponent != NULL && damageComponent->IsA( kbDQuadProjectileComponent::GetType() ) ) {
			damageComponentOwner = ((kbDQuadProjectileComponent*)damageComponent)->GetOwnerNetId();

			if ( damageComponentOwner == g_pNetworkingManager->GetLocalPlayerId() ) {
				kbCustomNetMsg_t customMsg;
				customMsg.m_bNotifyGame = true;
				DQuadDamageActorNetMsg_t damageMsg;
				damageMsg.m_SourceNetId = g_pNetworkingManager->GetLocalPlayerId();
				damageMsg.m_TargetNetId = GetParent()->GetNetId();
				customMsg.SetCustomData( &damageMsg, sizeof( damageMsg ) );
				g_pNetworkingManager->QueueNetMessage( -1, &customMsg );
			}
		}
		// Return since we're not the server
		return;
	}

	Super::TakeDamage( damageComponent, attackerComponent );

	if ( previousHealth > 0.0f && GetHealth() <= 0.0f ) {
		// Actor was killed from this damage
		g_AIManager.UnregisterActor( this );
		g_pGame->RemoveGameEntity( GetParent() );
	}
}

/**
 *	kbDQuadActorComponent::SetEnable_Internal
 */
void kbDQuadActorComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	if ( isEnabled ) {
		g_AIManager.RegisterActor( this );
	} else {
		g_AIManager.UnregisterActor( this );
	}
}

/**
 *	kbDQuadActorComponent::PlaceOnGround
 */
void kbDQuadActorComponent::PlaceOnGround() {
	kbVec3 floorPosition;
	if (((DQuadGame*)g_pGame)->TraceAgainstWorld( m_pParent->GetPosition() + kbVec3( 0.0f, 10000.0f, 0.0f ), m_pParent->GetPosition() - kbVec3( 0.0f, 10000.0f, 0.0f ), floorPosition, false ) ) {
		floorPosition.x = m_pParent->GetPosition().x;
		floorPosition.y += 10.0f;
		floorPosition.z = m_pParent->GetPosition().z;
		m_pParent->SetPosition( floorPosition );
	}
}