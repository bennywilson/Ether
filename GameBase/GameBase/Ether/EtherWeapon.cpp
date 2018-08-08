//===================================================================================================
// EtherWeapon.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "EtherGame.h"
#include "EtherActor.h"
#include "EtherSkelModel.h"
#include "EtherWeapon.h"
#include "EtherPlayer.h"
#include "DX11/kbRenderer_DX11.h"

kbVec3 g_ProjectileStuckOffset = kbVec3( 0.0f, 5.0f, 0.0f );

/**
 *	EtherProjectileComponent::Constructor
 */
void EtherProjectileComponent::Constructor() {
	m_Damage = 0;
	m_Velocity = 10.0f;
	m_LifeTime = 5.0f;
	m_TracerLength = 200.0f;
	m_TraceWidth = 5.0f;
	m_bUseBillboard = false;
	m_bExplodeOnImpact = true;
	m_DetonationTimer = -1.0f;
	m_DamageRadius = 0.0f;
}

/**
 *	EtherProjectileComponent::Launch
 */
void EtherProjectileComponent::Launch() {

	if ( m_LaunchSoundData.size() > 0 ) {
		m_LaunchSoundData[0].PlaySoundAtPosition( GetOwner()->GetPosition() );
	}

	kbLightComponent *const pLightComp = (kbLightComponent*)GetOwner()->GetComponentByType( kbLightComponent::GetType() );
	if ( pLightComp != nullptr ) {
		pLightComp->Enable( true );
	}
}

/**
 *	EtherProjectileComponent::LifeTimeExpired
 */
void EtherProjectileComponent::LifeTimeExpired()  {
	Super::LifeTimeExpired();
	g_pGame->RemoveGameEntity( GetOwner() );
}

/**
 *	EtherProjectileComponent::Update_Internal
 */
void EtherProjectileComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( g_UseEditor ) {
		return;
	}

	const kbMat4 projectileMat = GetOwner()->GetOrientation().ToMat4();

	// Draw projectile
	if ( m_bUseBillboard ) {
		kbParticleManager::CustomParticleInfo_t ParticleInfo;
		ParticleInfo.m_Position = GetOwner()->GetPosition();
		ParticleInfo.m_Direction = projectileMat[2].ToVec3();
		ParticleInfo.m_Width = m_TracerLength;
		ParticleInfo.m_Color.Set( 1.0f, 1.0f, 1.0f );
		ParticleInfo.m_Height = m_TraceWidth;
		ParticleInfo.m_UVs[0].Set( 0.125f, 0.0f );
		ParticleInfo.m_UVs[1].Set( 0.25f, 0.125f );
		ParticleInfo.m_Type = BT_AxialBillboard;
		g_pGame->GetParticleManager()->AddQuad( ParticleInfo );
	} else {
		kbParticleManager::CustomParticleInfo_t ParticleInfo;
		ParticleInfo.m_Position = GetOwner()->GetPosition();
		ParticleInfo.m_Direction = projectileMat[2].ToVec3();
		ParticleInfo.m_Width = 32.0f;
		ParticleInfo.m_Color.Set( 1.0f, 1.0f, 1.0f );
		ParticleInfo.m_Height = 32.0f;
		ParticleInfo.m_UVs[0].Set( 0.5f, 0.0f );
		ParticleInfo.m_UVs[1].Set( 0.625f, 0.125f );
		ParticleInfo.m_Type = BT_FaceCamera;
		g_pGame->GetParticleManager()->AddQuad( ParticleInfo );
	}

	const kbVec3 oldPosition = GetOwner()->GetPosition();
	const kbVec3 newPosition = GetOwner()->GetPosition() + projectileMat[2].ToVec3() * DeltaTime * m_Velocity;
	GetOwner()->SetPosition( newPosition );

	// Hack to get around dirty flag issues
	kbStaticModelComponent *const pSM = (kbStaticModelComponent*)GetOwner()->GetComponentByType( kbStaticModelComponent::GetType() );
	if ( pSM != nullptr ) {
		pSM->Update( DeltaTime );
	}

	bool bExploded = false;
	float hitT = FLT_MAX;

	// Check if this projectile hit another actor
	kbActorComponent * pHitActorComponent = nullptr;
	const kbCollisionInfo_t collisionInfo = g_CollisionManager.PerformLineCheck( oldPosition, newPosition );
	if ( collisionInfo.m_bHit && collisionInfo.m_pHitComponent != nullptr && collisionInfo.m_T <= 1.0f ) {
		pHitActorComponent = collisionInfo.m_pHitComponent->GetOwner()->GetActorComponent();
		hitT = collisionInfo.m_T;
	}

	// Check if this projectile hit the world
	EtherGame *const pGame = static_cast<EtherGame*>( g_pGame );
	kbVec3 worldHitCollisionPt;
	if ( pGame->TraceAgainstWorld( oldPosition, newPosition, worldHitCollisionPt, true ) ) {
		const float T = ( worldHitCollisionPt - oldPosition ).Length() / ( newPosition - oldPosition ).Length();
		if ( T < hitT ) {
			hitT = T;
			if ( m_bExplodeOnImpact ) {
				bExploded = true;
				g_pGame->RemoveGameEntity( GetOwner() );

				if ( m_ExplosionFX.GetEntity() != nullptr ) {
					kbGameEntity *const pExplosionFX = g_pGame->CreateEntity( m_ExplosionFX.GetEntity() );
					pExplosionFX->SetPosition( worldHitCollisionPt );
					pExplosionFX->SetOrientation( GetOwner()->GetOrientation() );
					pExplosionFX->DeleteWhenComponentsAreInactive( true );
				}

				if ( m_ImpactEnvironmentSoundData.size() > 0 ) {
					const int indexToPlay = rand() % m_ImpactEnvironmentSoundData.size();
					m_ImpactEnvironmentSoundData[indexToPlay].PlaySoundAtPosition( newPosition );
				}

				DealRadiusDamage();
				return;
			}
		}
	}

	if ( hitT < FLT_MAX && m_bExplodeOnImpact == false ) {
		const kbVec3 newPosition = oldPosition + projectileMat[2].ToVec3() * hitT;
		GetOwner()->SetPosition( newPosition + g_ProjectileStuckOffset );
		this->m_Velocity = 0.0f;
	} else if ( bExploded == false && pHitActorComponent != nullptr ) {

		bool bShouldDamage = true;
		if ( m_OwnerEntity.GetEntity() != nullptr && m_OwnerEntity.GetEntity()->GetComponentByType( EtherPlayerComponent::GetType() ) != pHitActorComponent->GetOwner()->GetComponentByType( EtherPlayerComponent::GetType() ) ) {
			pHitActorComponent->TakeDamage( this, nullptr );
		}
	
		if ( m_ImpactCharacterSoundData.size() > 0 ) {
			m_ImpactCharacterSoundData[0].PlaySoundAtPosition( newPosition );
		}
		DealRadiusDamage();
		g_pGame->RemoveGameEntity( GetOwner() );
		return;
	}

	if ( m_DetonationTimer > 0 ) {
		m_DetonationTimer -= DeltaTime;
		if ( m_DetonationTimer <= 0 ) {

			if ( m_ExplosionFX.GetEntity() != nullptr ) {
				kbGameEntity *const pExplosionFX = g_pGame->CreateEntity( m_ExplosionFX.GetEntity() );
			//	pExplosionFX->SetScale( kbVec3( 25.0f, 25.0f, 25.0f ) );
				pExplosionFX->SetPosition( GetOwner()->GetPosition() + g_ProjectileStuckOffset );
				pExplosionFX->SetOrientation( GetOwner()->GetOrientation() );
				pExplosionFX->DeleteWhenComponentsAreInactive( true );

				if ( m_ExplosionSoundData.size() > 0 ) {
					const int indexToPlay = rand() % m_ExplosionSoundData.size();
					m_ExplosionSoundData[indexToPlay].PlaySoundAtPosition( newPosition );
				}

				// Hack
				kbLightComponent *const pLightComp = ( kbLightComponent* ) pExplosionFX->GetComponentByType( kbLightComponent::GetType() );
				g_pRenderer->HackClearLight( pLightComp );
				g_pRenderer->AddLight( pLightComp, pExplosionFX->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ) );

				DealRadiusDamage();
			}

			g_pGame->RemoveGameEntity( GetOwner() );
		}
	}
//	g_pRenderer->DrawSphere( GetOwner()->GetPosition(), 10.0f, 12, kbColor::red );
}


/**
 *	EtherProjectileComponent::DealRadiusDamage
 */
void EtherProjectileComponent::DealRadiusDamage() {

	for ( int i = 0; i < g_pEtherGame->GetGameEntities().size(); i++ ) {
		kbGameEntity *const pCurEnt = g_pEtherGame->GetGameEntities()[i];
		EtherAIComponent *const pAI = (EtherAIComponent*) pCurEnt->GetComponentByType( EtherAIComponent::GetType() );
		if ( pAI == nullptr ) {
			continue;
		}

		const float distFromExplosion = ( pCurEnt->GetPosition() - ( GetOwner()->GetPosition() + g_ProjectileStuckOffset ) ).Length();
		if ( distFromExplosion > m_DamageRadius ) {
			continue;
		}

		pAI->TakeDamage( this, nullptr );
		
		if ( m_ImpactCharacterSoundData.size() > 0 ) {
			m_ImpactCharacterSoundData[0].PlaySoundAtPosition( pCurEnt->GetPosition() );
		}
	}
}

/**
 *	EtherWeaponComponent::Constructor
 */
void EtherWeaponComponent::Constructor() {
	m_SecondsBetweenShots = 1.0f;
	m_bInstantHit = 0;

	m_bIsFiring = false;
	m_BurstCount = 1;
	m_SecondsBetweenBursts = 0.5f;
	m_ShotTimer = 0.0f;
	m_BurstTimer = 0.0f;

	m_CurrentBurstCount = 0;
}

/**
 *	EtherWeaponComponent::Update_Internal
 */
void EtherWeaponComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_bIsFiring ) {
		if ( m_CurrentBurstCount >= m_BurstCount ) {
			m_bIsFiring = false;
			m_BurstTimer = 0.0f;
		} else {
			m_ShotTimer += DeltaTime;
			if ( m_ShotTimer >= m_SecondsBetweenShots ) {
				m_ShotTimer = 0.0f;
				m_CurrentBurstCount++;
				Fire_Internal();
			}
		}
	} else {
		m_BurstTimer += DeltaTime;
	}
}

/**
 *	EtherWeaponComponent::Fire
 */
bool EtherWeaponComponent::Fire( const bool bActivatedThisFrame ) {

	if ( m_bIsFiring == true || bActivatedThisFrame == false ) {
		return false;
	}

	if ( m_BurstTimer < m_SecondsBetweenBursts ) {
		return false;
	}

	m_CurrentBurstCount = 1;
	m_ShotTimer = 0.0f;

	return Fire_Internal();
}

/**
 *	EtherWeaponComponent::Fire_Internal
 */
bool EtherWeaponComponent::Fire_Internal() {

	const static kbString ShootName( "Shoot" );

	EtherSkelModelComponent * pWeaponModel = nullptr;

	for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
		kbComponent *const pCurComponent = GetOwner()->GetComponent(i);
		if ( pCurComponent->IsA( EtherSkelModelComponent::GetType() ) == false ) {
			continue;
		}

		EtherSkelModelComponent *const pSkelModel = static_cast<EtherSkelModelComponent*>( pCurComponent );
		if ( pSkelModel->IsFirstPersonModel()  ) {
			pSkelModel->PlayAnimation( kbString( ShootName ), -1.0f, true );
			pWeaponModel = pSkelModel;
			break;
		}
	}

	// Muzzle Flash
	const kbGameEntity *const pMuzzleFlashEntity = m_MuzzleFlashEntity.GetEntity();
	if ( pMuzzleFlashEntity != nullptr ) {

		for ( int i = 1; i < pMuzzleFlashEntity->NumComponents(); i++ ) {
			if ( pMuzzleFlashEntity->GetComponent(i)->IsA( kbParticleComponent::GetType() ) == false ) {
				continue;
			}
	
			const kbParticleComponent *const pParticleComponentPrefab = static_cast<kbParticleComponent*>( pMuzzleFlashEntity->GetComponent(i) );
			kbParticleComponent *const pParticle = g_pGame->GetParticleManager()->GetParticleComponent( pParticleComponentPrefab );
			if ( pParticle != nullptr ) {
				GetOwner()->AddComponent( pParticle );
				pParticle->Enable( true );
			}

			break;
		}
	}

	// Spawn projectile
	const kbGameEntity *const pProjectileEntity = m_Projectile.GetEntity();

	if ( pProjectileEntity != nullptr  && pWeaponModel != nullptr ) {

		EtherGame *const pEtherGame = static_cast<EtherGame*>( g_pGame );;
		const kbCamera & gameCamera = pEtherGame->GetCamera();

		// Determine projectile's orientation
		kbMat4 WeaponMatrix = GetOwner()->GetOrientation().ToMat4();

		const kbVec3 aimAtPoint = gameCamera.m_Position + 9999.0f * WeaponMatrix[2].ToVec3();
		const kbVec3 zAxis = ( aimAtPoint - GetOwner()->GetPosition() ).Normalized();
		const kbVec3 xAxis = kbVec3::up.Cross( zAxis ).Normalized();
		const kbVec3 yAxis = zAxis.Cross( xAxis ).Normalized();
 
		WeaponMatrix[0].Set( xAxis.x, xAxis.y, xAxis.z, 0.0f );
		WeaponMatrix[1].Set( yAxis.x, yAxis.y, yAxis.z, 0.0f );
		WeaponMatrix[2].Set( zAxis.x, zAxis.y, zAxis.z, 0.0f );
		WeaponMatrix[3].Set( 0.0f, 0.0f, 0.0f, 1.0f );

		const kbQuat WeaponOrientation = kbQuatFromMatrix( WeaponMatrix );
		const kbVec3 WeaponPos = GetOwner()->GetPosition();

		kbGameEntity *const newProjectile = g_pGame->CreateEntity( pProjectileEntity );
		newProjectile->SetPosition( WeaponPos );
		newProjectile->SetOrientation( WeaponOrientation );

		EtherProjectileComponent *const pProjectileComponent = static_cast<EtherProjectileComponent*>( newProjectile->GetComponentByType( EtherProjectileComponent::GetType() ) );

		kbErrorCheck( pProjectileComponent != nullptr, "EtherWeaponComponent::Fire - No Projectile Component found on the projectile entity :o" );

		kbGameEntity * pParent = GetOwner();
		while( pParent->GetOwner() != nullptr ) {
			pParent = pParent->GetOwner();
		}

		pProjectileComponent->m_OwnerEntity.SetEntity( pParent );
		pProjectileComponent->Launch();

		// Muzzle Flash
		kbParticleManager::CustomParticleInfo_t ParticleInfo;
		if ( pWeaponModel->GetBoneWorldPosition( kbString( "MuzzleFlash" ), ParticleInfo.m_Position ) == true ) {
			kbVec3 pos2 =  kbVec3( 0.0f, -16.0f, 0.0f ) + WeaponPos + WeaponMatrix[0].ToVec3() * 14 + WeaponMatrix[2].ToVec3() * 64.0f;
			ParticleInfo.m_Direction = WeaponMatrix[2].ToVec3();
			ParticleInfo.m_Width = 8.0f;
			ParticleInfo.m_Height = 8.0f;
			ParticleInfo.m_UVs[0].Set( 0.0f, 0.0f );
			ParticleInfo.m_UVs[1].Set( 0.125f, 0.125f );
			ParticleInfo.m_Type = BT_FaceCamera;
			ParticleInfo.m_Color.Set( 1.0f, 1.0f, 1.0f );
			g_pGame->GetParticleManager()->AddQuad( ParticleInfo );
		}
	}

	m_bIsFiring = true;

	return true;
}
