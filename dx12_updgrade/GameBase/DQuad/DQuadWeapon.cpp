//===================================================================================================
// DQuadWeapon.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "kbNetworkingManager.h"
#include "DQuadGame.h"
#include "DQuadActor.h"
#include "DQuadSkelModel.h"
#include "DQuadWeapon.h"

/**
 *	kbDQuadProjectileComponent::Constructor
 */
void kbDQuadProjectileComponent::Constructor() {
	m_Damage = 0;
	m_Velocity = 10.0f;
	m_LifeTime = 5.0f;
	m_TracerLength = 200.0f;
	m_TraceWidth = 5.0f;
	m_OwnerNetId = -1;
}

/**
 *	kbDQuadProjectileComponent::Update
 */
void kbDQuadProjectileComponent::Update( const float DeltaTime ) {
	Super::Update( DeltaTime );

	if ( g_UseEditor ) {
		return;
	}

	if ( GetTimeAliveInSeconds() > m_LifeTime ) {
		g_pGame->RemoveGameEntity( m_pParent );
		return;
	}

	// Draw projectile
	const kbMat4 projectileMat = m_pParent->GetOrientation().ToMat4();
	kbParticleManager::CustomParticleInfo_t ParticleInfo;
	ParticleInfo.m_Position = m_pParent->GetPosition() + projectileMat[2].ToVec3() * 200.0f;
	ParticleInfo.m_Direction = projectileMat[2].ToVec3();
	ParticleInfo.m_Width = m_TracerLength;
	ParticleInfo.m_Height = m_TraceWidth;
	ParticleInfo.m_UVs[0].Set( 0.125f, 0.0f );
	ParticleInfo.m_UVs[1].Set( 0.25f, 0.125f );
	ParticleInfo.m_Type = kbParticleManager::CPT_AxialBillboard;
	g_pGame->GetParticleManager()->AddQuad( ParticleInfo );

	const kbVec3 oldPosition = m_pParent->GetPosition();
	const kbVec3 newPosition = m_pParent->GetPosition() + projectileMat[2].ToVec3() * DeltaTime * m_Velocity;
	m_pParent->SetPosition( newPosition );

	bool bExploded = false;

	// Check if this projectile hit another actors
	const kbCollisionInfo_t collisionInfo = g_CollisionManager.PerformLineCheck( oldPosition, newPosition );
	if ( collisionInfo.m_bHit && collisionInfo.m_pHitComponent != NULL ) {
		kbActorComponent *const actorComponent = collisionInfo.m_pHitComponent->GetParent()->GetActorComponent();
		if ( actorComponent != NULL ) {
			actorComponent->TakeDamage( this, NULL );

			if ( m_ExplosionFX.GetEntity() != NULL ) {
				kbGameEntity *const pExplosionFX = g_pGame->CreateEntity( m_ExplosionFX.GetEntity(), NetOwner_LocalClient );
				pExplosionFX->SetPosition( collisionInfo.m_HitLocation );
				pExplosionFX->SetOrientation( m_pParent->GetOrientation() );
				pExplosionFX->DeleteWhenComponentsAreInactive( true );
			}

			g_pGame->RemoveGameEntity( GetParent() );
			bExploded = true;
		}
	}

	// Check if this projectile hit the world
	if ( bExploded == false ) {
		DQuadGame *const pGame = static_cast<DQuadGame*>( g_pGame );
		kbVec3 collisionPt;
		if ( pGame->TraceAgainstWorld( oldPosition, newPosition, collisionPt, true ) ) {
			if ( m_ExplosionFX.GetEntity() != NULL ) {
				kbGameEntity *const pExplosionFX = g_pGame->CreateEntity( m_ExplosionFX.GetEntity(), NetOwner_LocalClient );
				pExplosionFX->SetPosition( collisionPt );
				pExplosionFX->SetOrientation( m_pParent->GetOrientation() );
				pExplosionFX->DeleteWhenComponentsAreInactive( true );
			}
			g_pGame->RemoveGameEntity( GetParent() );
			bExploded = true;	
		}
	}
}

/**
 *	kbDQuadWeaponComponent::Constructor
 */
void kbDQuadWeaponComponent::Constructor() {
	m_ShotsPerSecond = 1.0f;
	m_bInstantHit = 0;
}

/**
 *	kbDQuadWeaponComponent::Update
 */
void kbDQuadWeaponComponent::Update( const float DeltaTime ) {
	Super::Update( DeltaTime );

}

/**
 *	kbDQuadWeaponComponent::Fire
 */
bool kbDQuadWeaponComponent::Fire() {

	const static kbString ShootName( "Shoot" );

	kbDQuadSkelModelComponent * pWeaponModel = NULL;

	for ( int i = 0; i < m_pParent->NumComponents(); i++ ) {
		kbComponent *const pCurComponent = m_pParent->GetComponent(i);
		if ( pCurComponent->IsA( kbDQuadSkelModelComponent::GetType() ) == false ) {
			continue;
		}

		kbDQuadSkelModelComponent *const pSkelModel = static_cast<kbDQuadSkelModelComponent*>( pCurComponent );
		if ( pSkelModel->IsFirstPersonModel() && pSkelModel->IsPlaying( ShootName ) ) {
			return false;
		}
	}

	for ( int i = 0; i < m_pParent->NumComponents(); i++ ) {
		kbComponent *const pCurComponent = m_pParent->GetComponent(i);
		if ( pCurComponent->IsA( kbDQuadSkelModelComponent::GetType() ) == false ) {
			continue;
		}

		kbDQuadSkelModelComponent *const pSkelModel = static_cast<kbDQuadSkelModelComponent*>( pCurComponent );
		if ( pSkelModel->IsFirstPersonModel()  ) {
			pSkelModel->PlayAnimation( kbString( ShootName ), true );
			pWeaponModel = pSkelModel;
		}
	}

	// Muzzle Flash
	const kbGameEntity *const pMuzzleFlashEntity = m_MuzzleFlashEntity.GetEntity();
	if ( pMuzzleFlashEntity != NULL ) {

		for ( int i = 1; i < pMuzzleFlashEntity->NumComponents(); i++ ) {
			if ( pMuzzleFlashEntity->GetComponent(i)->IsA( kbParticleComponent::GetType() ) == false ) {
				continue;
			}
	
			const kbParticleComponent *const pParticleComponentPrefab = static_cast<kbParticleComponent*>( pMuzzleFlashEntity->GetComponent(i) );
			kbParticleComponent *const pParticle = g_pGame->GetParticleManager()->GetParticleComponent( pParticleComponentPrefab );
			if ( pParticle != NULL ) {
				m_pParent->AddComponent( pParticle );
				pParticle->Enable( true );
			}

			break;
		}
	}

	// Spawn projectile
	const kbGameEntity *const pProjectileEntity = m_Projectile.GetEntity();
	if ( pProjectileEntity != NULL  && pWeaponModel != NULL ) {
		kbQuat WeaponOrientation;
		kbMat4 WeaponMatrix;
		kbVec3 WeaponPos;
		if ( g_pRenderer->UsingHMD() ) {
			WeaponOrientation = static_cast<kbTransformComponent*>( pWeaponModel->GetParent()->GetComponent( 0 ) )->GetOrientation();
			WeaponMatrix = WeaponOrientation.ToMat4();
			WeaponPos = static_cast<kbTransformComponent*>( pWeaponModel->GetParent()->GetComponent( 0 ) )->GetPosition();
		} else {
			WeaponOrientation = m_pParent->GetOrientation();
			WeaponPos = m_pParent->GetPosition();
			WeaponMatrix = WeaponOrientation.ToMat4();
		}

		kbGameEntity *const newProjectile = g_pGame->CreateEntity( pProjectileEntity, NetOwner_LocalClient );
		newProjectile->SetPosition( kbVec3( 0.0f, -5.0f, 0.0f ) + WeaponPos + WeaponMatrix[0].ToVec3() * 4.0f + WeaponMatrix[2].ToVec3() * 1.0f );
		newProjectile->SetOrientation( WeaponOrientation );

		kbDQuadProjectileComponent *const pProjectileComponent = static_cast<kbDQuadProjectileComponent*>( newProjectile->GetComponentByType( kbDQuadProjectileComponent::GetType() ) );

		if ( pProjectileComponent == NULL ) {
			kbError( "kbDQuadWeaponComponent::Fire - No Projectile Component found on the projectile entity :o" );
		} else {
			kbGameEntity * pParent = GetParent();
			while( pParent->GetParent() != NULL ) {
				pParent = pParent->GetParent();
			}
			pProjectileComponent->m_OwnerNetId = pParent->GetNetId();
		}

		// Muzzle Flash
		kbParticleManager::CustomParticleInfo_t ParticleInfo;
		if ( pWeaponModel->GetBoneWorldPosition( kbString( "MuzzleFlash" ), ParticleInfo.m_Position ) == true ) {
			kbVec3 pos2 =  kbVec3( 0.0f, -16.0f, 0.0f ) + WeaponPos + WeaponMatrix[0].ToVec3() * 14 + WeaponMatrix[2].ToVec3() * 64.0f;
			ParticleInfo.m_Direction = WeaponMatrix[2].ToVec3();
			ParticleInfo.m_Width = 8.0f;
			ParticleInfo.m_Height = 8.0f;
			ParticleInfo.m_UVs[0].Set( 0.0f, 0.0f );
			ParticleInfo.m_UVs[1].Set( 0.125f, 0.125f );
			ParticleInfo.m_Type = kbParticleManager::CPT_FaceCamera;
			ParticleInfo.m_Color.Set( 1.0f, 1.0f, 1.0f );
			g_pGame->GetParticleManager()->AddQuad( ParticleInfo );
		}

		if ( g_pNetworkingManager != NULL ) {
			kbCustomNetMsg_t customMsg;
			customMsg.m_bNotifyGame = true;
			DQuadSpawnActorNetMsg_t spawnActorMsg;
			spawnActorMsg.m_MsgType = DQuadMsg_SpawnActor;
			spawnActorMsg.m_PrefabGUID = pProjectileEntity->GetGUID();
			spawnActorMsg.m_Position = WeaponPos;
			spawnActorMsg.m_Orientation = WeaponOrientation;
			customMsg.SetCustomData( &spawnActorMsg, sizeof( spawnActorMsg ) );
			g_pNetworkingManager->QueueNetMessage( -1, &customMsg );
		}
	}

	return true;
}
