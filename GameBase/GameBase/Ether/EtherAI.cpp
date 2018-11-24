//===================================================================================================
// EtherAI.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "EtherGame.h"
#include "EtherAI.h"
#include "EtherPlayer.h"
#include "EtherSkelModel.h"
#include "EtherWeapon.h"
#include "kbRenderer.h"

EtherAIManager * g_pAIManager = nullptr;
static const float g_DetachZDist = 200.0f;
static const float g_KillZDist = 500.0f;

kbConsoleVariable g_DebugAI( "debugai", false, kbConsoleVariable::Console_Bool, "Draw Debug ai info", "" );
kbConsoleVariable g_DisableAIFire( "disableaifire", false, kbConsoleVariable::Console_Bool, "Enable/disable AI Firing", "" );
kbConsoleVariable g_EnableAI( "enableai", true, kbConsoleVariable::Console_Bool, "Enable/disable AI state machine", "" );
kbConsoleVariable g_ShowAICollision( "showaicollision", false, kbConsoleVariable::Console_Bool, "Shows AI collision", "" );

const float EtherAIManager::CombatantInfo_t::CLOSE_COMBAT_DISTANCE = 120.0f;

/**
 *	EtherAIComponent::Constructor
 */
void EtherAIComponent::Constructor() {
	m_BaseMoveSpeed = 200.0f;
	m_LastNetPositionUpdateTime = 0.0f;

	m_AIState = Enemy_Idle;
	m_PursueMinDistance = 120.0f;
	m_DeathStartTimer = 0.0f;
	m_TargetLocation.Set( 0.0f, 0.0f, 0.0f );
	m_TargetRotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	m_DeathVelocity.Set( 0.0f, 0.0f, 0.0f );
}

/**
 *	EtherEnemyAIComponent::StartDeath
 */
void EtherAIComponent::StartDeath( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) {

	m_DeathStartTimer = 0.0f;
	m_AIState = Enemy_Dead;

	kbCollisionComponent *const pCollisionComponent = (kbCollisionComponent*) GetOwner()->GetComponentByType( kbCollisionComponent::GetType() );
	if ( pCollisionComponent != nullptr ) {
		g_CollisionManager.UnregisterComponent( pCollisionComponent );
	}

	EtherSkelModelComponent * pSkelModelComponent = (EtherSkelModelComponent*)GetOwner()->GetComponentByType( EtherSkelModelComponent::GetType() );
	if ( pSkelModelComponent == nullptr || pSkelModelComponent->GetModel() == nullptr ) {
		kbError( "EtherEnemyAIComponent::StartDeath() - No skeletal model found" );
		return;
	}

	const float deathChance = kbfrand();
	if ( deathChance < 0.2f ) {
		pSkelModelComponent->PlayAnimation( kbString( "Death_Back_1" ), 0.05f, false );

		const float velModifider = ( kbfrand() * 60.0f ) + 120.0f;
		m_DeathVelocity = -GetOwner()->GetOrientation().ToMat4()[2].ToVec3() * velModifider;
	} else if ( deathChance < 0.4f ) {
		pSkelModelComponent->PlayAnimation( kbString( "Death_Back_2" ), 0.05f, false );

		const float velModifider = ( kbfrand() * 60.0f ) + 140.0f;
		m_DeathVelocity = -GetOwner()->GetOrientation().ToMat4()[2].ToVec3() * velModifider;
	} else if ( deathChance < 0.6f ) {
		pSkelModelComponent->PlayAnimation( kbString( "Death_Spin" ), 0.05f, false );

		const float velModifider = ( kbfrand() * 60.0f ) + 140.0f;
		m_DeathVelocity = -GetOwner()->GetOrientation().ToMat4()[2].ToVec3() * velModifider;
	} else if ( deathChance < 0.8f ) {
		pSkelModelComponent->PlayAnimation( kbString( "Death_KneelForward" ), 0.05f, false );

		const float velModifider = ( kbfrand() * 60.0f ) + 140.0f;
		m_DeathVelocity = kbVec3::zero;//-GetOwner()->GetOrientation().ToMat4()[2].ToVec3() * velModifider;
	} else {
		pSkelModelComponent->PlayAnimation( kbString( "Death_Flip" ), 0.05f, false );

		const float velModifider = ( kbfrand() * 60.0f ) + 120.0f;
		m_DeathVelocity = -GetOwner()->GetOrientation().ToMat4()[2].ToVec3() * velModifider;
	}
}

/**
 *	EtherAIComponent::State_Dead
 */
void EtherAIComponent::State_Dead( const float DeltaTime ) {

	m_DeathStartTimer += DeltaTime;
	if ( m_DeathStartTimer > 1.5f ) {
		g_pGame->RemoveGameEntity( GetOwner() );
		return;
	}

	kbVec3 FinalPosition = GetOwner()->GetPosition();
	EtherSkelModelComponent *const pSkelModelComponent = ( EtherSkelModelComponent* ) GetOwner()->GetComponentByType( EtherSkelModelComponent::GetType() );
	if ( pSkelModelComponent != nullptr && pSkelModelComponent->HasFinishedAnimation() == false ) {
		FinalPosition += ( m_DeathVelocity * DeltaTime );
	} 

	if ( pSkelModelComponent != nullptr && pSkelModelComponent->GetCurAnimLengthSeconds() > 0.0f && pSkelModelComponent->IsTransitioningAnimations() == false ) {
		const float curAnimLength = pSkelModelComponent->GetCurAnimLengthSeconds();
		const float curTime = pSkelModelComponent->GetCurAnimTimeSeconds();
		const float curAnimTime = kbClamp( pSkelModelComponent->GetCurAnimTimeSeconds(), 0.0f, curAnimLength );
		const float NormalizedAnimTime = curAnimTime / curAnimLength;
		m_GroundHoverDist = 0.5f - ( NormalizedAnimTime * 13.0f );
	}

	GetOwner()->SetPosition( FinalPosition );
}

/**
 *	EtherEnemySoldierAIComponent::ClientNetUpdate
 */
void EtherEnemySoldierAIComponent::ClientNetUpdate( const kbNetMsg_t * NetMsg ) {
	PlaceOnGround( m_GroundHoverDist );
}

/**
 *	EtherAIComponent::Update_Internal
 */
void EtherAIComponent::Update_Internal( const float DeltaTimeSeconds ) {
	Super::Update_Internal( DeltaTimeSeconds );

	EtherSkelModelComponent *const pSkelModel = (EtherSkelModelComponent*)GetOwner()->GetComponentByType( EtherSkelModelComponent::GetType() );
	kbCollisionComponent *const pCollisionComponent = (kbCollisionComponent*)GetOwner()->GetComponentByType( kbCollisionComponent::GetType() );

	if ( pCollisionComponent != nullptr && pSkelModel != nullptr ) {

		// Debug collision
		if ( g_ShowAICollision.GetBool() ) {

			for ( int iCollision = 0; iCollision < pCollisionComponent->GetWorldSpaceCollisionSpheres().size(); iCollision++ ) {
				g_pRenderer->DrawSphere( pCollisionComponent->GetWorldSpaceCollisionSpheres()[iCollision].ToVec3(), pCollisionComponent->GetWorldSpaceCollisionSpheres()[iCollision].w, 12, kbColor::red );

			}
		}
	}

	if ( g_EnableAI.GetBool() == false && m_AIState != Enemy_Dead ) {
		return;
	}

	switch( m_AIState ) {
		case Enemy_Pursue : {
			State_Pursue( DeltaTimeSeconds );
			break;
		}

		case Enemy_CloseCombat : {
			State_CloseCombat( DeltaTimeSeconds );
			break;
		}

		case Enemy_Dead : {
			State_Dead( DeltaTimeSeconds );
			break;
		}
	}

	if ( m_AIState != Enemy_Dead ) {
		UpdateMovementAndAnimation( DeltaTimeSeconds );
	} else {
		PlaceOnGround( m_GroundHoverDist );
	}
}

/**
 *	EtherEnemySoldierAIComponent::Constructor
 */
void EtherEnemySoldierAIComponent::Constructor() {
	m_AIState = Enemy_Pursue;
	m_pEyeBall = nullptr;
	m_bEyeballAdded = false;

	m_bIsSpraying = false;
	m_LastSprayTime = 0.0f;
	m_CurSprayStartTime = 0.0f;
	m_NextShotTime = 0.0f;
	m_NextSprayTime = 5.0f;

	m_SprayDurationSec = 1.25f;
	m_SecTimeBetweenSprays = 5.0f;
	m_SecBetweenShots = 0.25f;

	m_EyeBallRenderObject.m_pComponent = this;
	m_EyeBallRenderObject.m_bCastsShadow = false;
	m_EyeBallRenderObject.m_bIsSkinnedModel = false;
	m_EyeBallRenderObject.m_pComponent = this;
	m_EyeBallRenderObject.m_pModel = m_pEyeBall;

	m_EyeBallRenderObject.m_RenderPass = RP_PostLighting;
}

/**
 *	EtherEnemySoldierAIComponent::SetEnable_Internal
 */
void EtherEnemySoldierAIComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	if ( m_pEyeBall == nullptr ) {
		return;
	}

	if ( isEnabled && m_bEyeballAdded == false ) {
		m_EyeBallRenderObject.m_pModel = m_pEyeBall;

		g_pRenderer->AddRenderObject( m_EyeBallRenderObject );
		m_bEyeballAdded = true;
	} else {
		g_pRenderer->RemoveRenderObject( m_EyeBallRenderObject );
		m_bEyeballAdded = false;
	}
}

/**
 *	EtherEnemySoldierAIComponent::Update_Internal
 */
void  EtherEnemySoldierAIComponent::Update_Internal( const float DeltaTime ) {
	if ( m_pEyeBall != nullptr ) {

		m_EyeBallRenderObject.m_Scale = GetOwner()->GetScale();
		m_EyeBallRenderObject.m_Position = GetOwner()->GetPosition();
		m_EyeBallRenderObject.m_Orientation = GetOwner()->GetOrientation();
		m_EyeBallRenderObject.m_pModel = m_pEyeBall;

		EtherSkelModelComponent *const pSkelModelComponent = (EtherSkelModelComponent*)GetOwner()->GetComponentByType( EtherSkelModelComponent::GetType() );
		if ( pSkelModelComponent != nullptr ) {
			static kbString EyeBone( "Dummy07" );
			kbBoneMatrix_t EyeMatrix;
			if ( pSkelModelComponent->GetBoneWorldMatrix( EyeBone, EyeMatrix ) ) {

				kbMat4 rotMat( kbMat4::identity );
				rotMat[0] = EyeMatrix.GetAxis(0);
				rotMat[1] = EyeMatrix.GetAxis(1);
				rotMat[2] = EyeMatrix.GetAxis(2);

				m_EyeBallRenderObject.m_OverrideShaderList.clear();
				m_EyeBallRenderObject.m_OverrideShaderList.push_back( (kbShader*)g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/SimpleAdditive.kbShader", true ) );
				if ( m_bEyeballAdded == false ) {
					g_pRenderer->AddRenderObject( m_EyeBallRenderObject );
				} else {
					m_EyeBallRenderObject.m_Position = EyeMatrix.GetOrigin();
					g_pRenderer->UpdateRenderObject( m_EyeBallRenderObject );
				}
				m_bEyeballAdded = true;
			}
		}
	}

	Super::Update_Internal( DeltaTime );

	// Firing behavior
	if ( m_AIState != Enemy_Dead ) {
		UpdateFiringBehavior( DeltaTime );
	}

	const kbGameEntity *const pTarget = m_TargetEntity.GetEntity();

	// Kill AI if it's too far behind the player
	if ( pTarget != nullptr && pTarget->GetPosition().z > GetOwner()->GetPosition().z + g_KillZDist ) {
		g_pGame->RemoveGameEntity( GetOwner() );
		return;
	}
}

/**
 *	EtherEnemySoldierAIComponent::UpdateFiringBehavior
 */
void EtherEnemySoldierAIComponent::UpdateFiringBehavior( const float DeltaSeconds ) {

	if ( g_DisableAIFire.GetBool() == true ) {
		return;
	}

	const kbGameEntity *const pTarget = m_TargetEntity.GetEntity();
	if ( pTarget == nullptr ) {
		return;
	}

	kbActorComponent *const pTargetActor = pTarget->GetActorComponent();
	if ( pTargetActor == nullptr || pTargetActor->IsDead() ) {
		return;
	}

	const float currentTimeSec = g_GlobalTimer.TimeElapsedSeconds();

	// If out in open 
	if ( m_bIsSpraying ) {

		if ( currentTimeSec >= m_NextShotTime ) {
			EtherGame *const pGame = static_cast<EtherGame*>( g_pGame );
			if ( Fire() ) {
				m_NextShotTime = currentTimeSec + m_SecBetweenShots;
			}
		}

		if ( currentTimeSec >= m_CurSprayStartTime + m_SprayDurationSec ) {
			m_bIsSpraying = false;
			m_NextSprayTime = currentTimeSec + m_SecTimeBetweenSprays;
		}

	} else {
		if ( currentTimeSec >= m_NextSprayTime ) {
			m_bIsSpraying = true;
			m_CurSprayStartTime = currentTimeSec;
		}
	}
}

/**
 *	EtherEnemySoldierAIComponent::Fire
 */	
bool EtherEnemySoldierAIComponent::Fire() {

	static bool bHackNoFire = true;
	if ( bHackNoFire ) {
		return false;
	}

	const kbGameEntity *const pTarget = m_TargetEntity.GetEntity();
	const kbGameEntity *const pProjectileEntity = m_Projectile.GetEntity();
	if ( pProjectileEntity == nullptr || pTarget == nullptr ) {
		return false;
	}

	const float SpraySpread = 10.0f;
	const kbVec3 TargetPosition = pTarget->GetPosition() + kbVec3( ( kbfrand() * SpraySpread ) - ( SpraySpread * 0.5f ), 0.0f, 0.0f );
	kbVec3 fireLoc = GetOwner()->GetPosition() + kbVec3( 0.0f, 17.5f, 0.0f );
	float targetDist = ( TargetPosition - fireLoc ).Length();
	const kbVec3 zAxis = ( TargetPosition - fireLoc ).Normalized();
	fireLoc += zAxis * 25.0f;

	EtherGame *const pGame = static_cast<EtherGame*>( g_pGame );
	kbVec3 hitPt;
	if ( pGame->TraceAgainstWorld( fireLoc, fireLoc + zAxis * targetDist * 0.75f, hitPt, true ) ) {
		return false;
	}

	// Spawn projectile
	const kbVec3 xAxis = kbVec3::up.Cross( zAxis ).Normalized();
	const kbVec3 yAxis = zAxis.Cross( xAxis ).Normalized();
	kbMat4 weaponMatrix;
	weaponMatrix[0].Set( xAxis.x, xAxis.y, xAxis.z, 0.0f );
	weaponMatrix[1].Set( yAxis.x, yAxis.y, yAxis.z, 0.0f );
	weaponMatrix[2].Set( zAxis.x, zAxis.y, zAxis.z, 0.0f );
	weaponMatrix[3].Set( 0.0f, 0.0f, 0.0f, 1.0f );

	kbGameEntity *const newProjectile = g_pGame->CreateEntity( pProjectileEntity );
	newProjectile->SetPosition( fireLoc );
	newProjectile->SetOrientation( kbQuatFromMatrix( weaponMatrix ) );

	EtherProjectileComponent *const pProjectile = ( EtherProjectileComponent* ) newProjectile->GetComponentByType( EtherProjectileComponent::GetType() );
	kbGameEntityPtr owner;
	owner.SetEntity( GetOwner() );
	pProjectile->SetOwner( owner );

	pProjectile->Launch();

	return true;
}

/**
 *	EtherEnemySoldierAIComponent::State_Pursue
 */
void EtherEnemySoldierAIComponent::State_Pursue( const float DeltaTime ) {

	if ( g_pGame->GetPlayersList().size() == 0 ) {
		return;
	}

	if ( m_TargetEntity.GetEntity() == nullptr ) {
		const int numPlayers = (int) g_pGame->GetPlayersList().size();
		if ( numPlayers > 0 ) {
			EtherPlayerComponent *const pPlayer = (EtherPlayerComponent*)g_pGame->GetPlayersList()[0]->GetComponentByType( EtherPlayerComponent::GetType() );
			if ( pPlayer->IsDead() == false ) {
				 m_TargetEntity.SetEntity( g_pGame->GetPlayersList()[0] );
			}
		} else {
			return;
		}
	}

	const kbGameEntity *const pTarget = m_TargetEntity.GetEntity();
	if ( pTarget == nullptr ) {
		return;
	}
		
	EtherPlayerComponent *const pPlayer = (EtherPlayerComponent*)pTarget->GetComponentByType( EtherPlayerComponent::GetType() );
	if ( pPlayer != nullptr && pPlayer->IsDead() ) {
		return;
	}

	g_pEtherGame->GetAIManager().GetCloseCombatSpotOnTarget( m_TargetLocation, this, (EtherActorComponent*)pTarget->GetComponentByType( EtherActorComponent::GetType() ) );
}

/**
 *	EtherEnemySoldierAIComponent::State_CloseCombat
 */
void EtherEnemySoldierAIComponent::State_CloseCombat( const float DeltaTime ) {
	if ( g_pGame->GetPlayersList().size() == 0 ) {
		return;
	}

	const kbGameEntity *const pTarget = m_TargetEntity.GetEntity();
	if ( pTarget == nullptr ) {
		m_AIState = Enemy_Pursue;
		return;
	}

	const bool bSpotFound = g_pEtherGame->GetAIManager().GetCloseCombatSpotOnTarget( m_TargetLocation, this, (EtherActorComponent*)pTarget->GetComponentByType( EtherActorComponent::GetType() ) );
	if ( bSpotFound == false ) {
//		kbError( "EtherEnemySoldierAIComponent::State_CloseCombat() - But no available spot" );
		return;
	}

	const kbVec3 vecToTargetLocation = ( m_TargetLocation - GetOwner()->GetPosition() ).ToVecXZ();
	if ( vecToTargetLocation.Length() > m_PursueMinDistance + m_PursueMinDistance * 0.25f ) {
		m_AIState = Enemy_Pursue;
		return;
	}
}

/**
 *	EtherEnemySoldierAIComponent::UpdateMovementAndAnimation
 */
void EtherEnemySoldierAIComponent::UpdateMovementAndAnimation( const float DeltaTimeSeconds ) {

	const float epsilon = 0.01f;

	EtherSkelModelComponent *const pSkelModel = ( EtherSkelModelComponent* ) GetOwner()->GetComponentByType( EtherSkelModelComponent::GetType() );
	if ( pSkelModel == nullptr ) {
		return;
	}

	// Face the target
	const kbGameEntity *const pTarget = m_TargetEntity.GetEntity();
	if ( pTarget == nullptr ) {
		return;
	}

	// Stop chasing if player is too far ahead.  Can keep shooting
	if ( pTarget->GetPosition().z > GetOwner()->GetPosition().z + g_DetachZDist ) {
		return;
	}

	kbVec3 vecToTarget = pTarget->GetPosition() - GetOwner()->GetPosition();
	if ( vecToTarget.Compare( kbVec3::zero ) == false ) {
		vecToTarget.Normalize();
		if ( vecToTarget.Compare( kbVec3( 0.0f, 1.0f, 0.0f ) ) == false ) {
			const kbVec3 rightVec = kbVec3( 0.0f, 1.0f, 0.0f ).Cross( vecToTarget ).Normalized();
			const kbVec3 forwardVec = rightVec.Cross( kbVec3( 0.0f, 1.0f, 0.0f ) ).Normalized();
			const kbMat4 facingMatrix( rightVec, kbVec3( 0.0f, 1.0f, 0.0f ), forwardVec, kbVec3::zero );
			GetOwner()->SetOrientation( kbQuatFromMatrix( facingMatrix ) );
		}
	}

	kbVec3 vecToTargetLocation = ( m_TargetLocation - GetOwner()->GetPosition() ).ToVecXZ();
	const float distToTargetLocSqr = vecToTargetLocation.LengthSqr();
	if ( distToTargetLocSqr < 3.0f ) {
		pSkelModel->PlayAnimation( kbString( "Aim" ), 0.0f, false );
	} else {
		const kbMat4 orientationMatrix = GetOwner()->GetOrientation().ToMat4();
		const kbVec3 forwardVec = orientationMatrix[2].ToVec3().ToVecXZ();

		if ( vecToTargetLocation.Compare( kbVec3::zero ) ) {
			pSkelModel->PlayAnimation( kbString( "Aim" ), 0.3f, false );
		} else {
			vecToTargetLocation.Normalize();
			float runSpeedScale = 0.25f;
	
			const float vecBetween = forwardVec.Dot( vecToTargetLocation );
			static float BlendBetweenAnimTime = 0.5f;
			if ( vecBetween < 0.0f ) {
				// Backwards
				pSkelModel->PlayAnimation( kbString( "BackPedal" ), BlendBetweenAnimTime, false );
			} else {
				if ( vecBetween >= 0.785f ) {
					pSkelModel->PlayAnimation( kbString( "Run" ), BlendBetweenAnimTime, false );
				} else {
					const kbVec3 rightVec = orientationMatrix[0].ToVec3().ToVecXZ();
					const float rightDotToTargetVec = rightVec.Dot( vecToTargetLocation );
					if ( rightDotToTargetVec < 0.0f ) {
						pSkelModel->PlayAnimation( kbString( "StrafeLeft" ), BlendBetweenAnimTime, false );
					} else {
						pSkelModel->PlayAnimation( kbString( "StrafeRight" ), BlendBetweenAnimTime, false );
					}
				}
			}

			kbVec3 finalPosition = GetOwner()->GetPosition() + ( vecToTargetLocation * DeltaTimeSeconds * m_BaseMoveSpeed * runSpeedScale );
			const EtherGame *const pGame = static_cast<EtherGame*>( g_pGame );
			const EtherCoverObject * pCoverObject = nullptr;

			// If the finalPosition is inside of a cover object, push it out
			if ( pGame->CoverObjectsPointTest( pCoverObject, finalPosition ) ) {
				kbVec3 vecToPlayer = ( finalPosition - pCoverObject->GetPosition() );
				vecToPlayer.y = 0.0f;
				vecToPlayer.Normalize();

				kbVec3 Extent = ( pCoverObject->GetBounds().Max() - pCoverObject->GetPosition() );
				Extent.y = 0.0f;
				const float coverRadius = Extent.Length();
				finalPosition = pCoverObject->GetPosition() + vecToPlayer * coverRadius;
			}

			GetOwner()->SetPosition( finalPosition );
		}
	}

	// Place on the ground
	if ( g_pGame->IsPlaying() ) {
		PlaceOnGround( m_GroundHoverDist );
	}
}

/**
 *	CombatantInfo_t::CombatantInfo_t
 */
EtherAIManager::CombatantInfo_t::CombatantInfo_t() : 
	m_MyTargetId( -1 ) {

	for ( int i = 0; i < NUM_CLOSE_RANGE_SPOTS; i++ ) {
		m_CloseRangeSpots[i] = -1;
	}	
}

/**
 *	CombatantInfo_t::GetCloseOffset
 */
kbVec3 EtherAIManager::CombatantInfo_t::GetCloseOffset( const int iCloseSpot ) {


	float slotRotation = kbPI / ( NUM_CLOSE_RANGE_SPOTS + 2 );
	slotRotation = ( iCloseSpot * slotRotation ) + slotRotation;
	kbVec3 startVec( CombatantInfo_t::CLOSE_COMBAT_DISTANCE, 0.0f, 0.0f );
	kbMat4 rotationMat;
	rotationMat[0].Set( cos( slotRotation ), 0.0f, sin( slotRotation ), 0.0f );
	rotationMat[1].Set( 0.0f, 1.0f, 0.0f, 0.0f );
	rotationMat[2].Set( -sin( slotRotation ), 0.0f, cos( slotRotation ), 0.0f );
	rotationMat[3].Set( 0.0f, 0.0f, 0.0f, 1.0f );

	startVec = startVec * rotationMat;
	return startVec;
}

/**
 *	EtherAIManager::EtherAIManager
 */
EtherAIManager::EtherAIManager() :
	m_DebugAIIdx( 0 ) {
	if ( g_pAIManager != nullptr ) {
		kbError( "EtherAIManager::EtherAIManager() - g_pAIManager is not nullptr" );
	}

	g_pAIManager = this;
}
/**
 *	EtherAIManager::~EtherAIManager
 */
EtherAIManager::~EtherAIManager() {
	if ( g_pAIManager == nullptr ) {
		kbError( "EtherAIManager::~EtherAIManager() - g_pAIManager is nullptr" );
	}

	g_pAIManager = nullptr;
}

/**
 *	EtherAIManager::Initialize
 */
void EtherAIManager::Initialize() {
	g_pInputManager->MapKeysToCallback( "ctrl a", this, 0, "Enable AI debugging" );
	g_pInputManager->MapKeysToCallback( "ctrl n", this, 1, "Switch targets while AI debugging is on." );
}

/**
 *	EtherAIManager::RegisterCombatant
 */
void EtherAIManager::RegisterCombatant( EtherActorComponent *const actorComponent ) {
	
	if ( actorComponent->IsA( EtherAIComponent::GetType() ) ) {
		kbGameEntityPtr entPtr;
		entPtr.SetEntity( actorComponent->GetOwner() );

		kbLog( "Registering %d", entPtr.GetEntityIndex() );
		m_AIList.push_back( entPtr );
	}
/*
	if ( actorComponent == nullptr || actorComponent->GetOwner() == nullptr ) {
		kbError( "EtherAIManager::RegisterCombatant() - nullptr actorComponent passed in" );
		return;
	}

	const uint combatantNetId = actorComponent->GetOwner()->GetNetId();
	std::map<int, CombatantInfo_t>::iterator combatantIt = m_CombatantMap.find( combatantNetId );
	if ( combatantIt != m_CombatantMap.end() ) {
		kbError( "EtherAIManager::RegisterCombatant() - Combatant with id %d has already been registered", combatantNetId );
		return;
	}

	m_CombatantMap[combatantNetId] = CombatantInfo_t();*/
}

/**
 *	EtherAIManager::UnregisterCombatant
 */
void EtherAIManager::UnregisterCombatant( EtherActorComponent *const actorComponent ) {

	kbGameEntityPtr entPtr;
	entPtr.SetEntity( actorComponent->GetOwner() );

		kbLog( "Unregistering %d", entPtr.GetEntityIndex() );


	VectorRemoveFast( m_AIList, entPtr );
/*
	if ( actorComponent == nullptr || actorComponent->GetOwner() == nullptr ) {
		kbError( "EtherAIManager::UnregisterCombatant() - nullptr actorComponent passed in" );
		return;
	}

	// Remove from attacker map and from attack spots in the target map
	const uint combatantNetId = actorComponent->GetOwner()->GetNetId();
	std::map<int, CombatantInfo_t>::iterator combatantIt = m_CombatantMap.find( combatantNetId );
	if ( combatantIt != m_CombatantMap.end() ) {

		// Inform target that his combatant is no longer targeting it
		const uint TargetId = combatantIt->second.m_MyTargetId;
		std::map<int, CombatantInfo_t>::iterator targetIt = m_CombatantMap.find( TargetId );
		if ( targetIt != m_CombatantMap.end() ) {
			CombatantInfo_t & TargetInfo = targetIt->second;
			for ( int iCloseSpot = 0; iCloseSpot < NUM_CLOSE_RANGE_SPOTS; iCloseSpot++ ) {
				if ( TargetInfo.m_CloseRangeSpots[iCloseSpot] != combatantNetId ) {
					continue;
				}

				// Callback?
				TargetInfo.m_CloseRangeSpots[iCloseSpot] = -1;
			}
		}

		// Inform attackers that this combatant is no longer alive
		CombatantInfo_t & CombatantInfo = combatantIt->second;
		for ( int iAttackers = 0; iAttackers < NUM_CLOSE_RANGE_SPOTS; iAttackers++ ) {
			const int attackerNetId = CombatantInfo.m_CloseRangeSpots[iAttackers];
			if ( attackerNetId == -1 ) {
				continue;
			}

			std::map<int, CombatantInfo_t>::iterator attackerIt = m_CombatantMap.find( attackerNetId );
			if ( attackerIt == m_CombatantMap.end() ) {
				continue;
			}

			if ( attackerIt->second.m_MyTargetId != combatantNetId ) {
				kbError( "EtherAIManager::UnregisterCombatant() - Strafe spot occupied by an attacker with a different target" );
			}

			// Callback?
			attackerIt->second.m_MyTargetId = -1;
		}
		m_CombatantMap.erase( combatantNetId );
	}*/
}

/**
 *	EtherAIManager::GetCloseCombatSpotOnTarget
 */
bool EtherAIManager::GetCloseCombatSpotOnTarget( kbVec3 & out_GoalPosition, const EtherActorComponent *const attacker, const EtherActorComponent *const target ) {

/*	if ( attacker == nullptr || attacker->GetOwner() == nullptr ) {
		kbError( "EtherAIManager::GetCloseCombatSpotOnTarget() - nullptr attacker passed in" );
		return false;
	}

	if ( target == nullptr || target->GetOwner() == nullptr ) {
		kbError( "EtherAIManager::GetCloseCombatSpotOnTarget() - nullptr target passed in" );
		return false;
	}

	// Get attacker's info
	const uint attackerNetId = attacker->GetOwner()->GetNetId();
	std::map<int, CombatantInfo_t>::iterator attackerIt = m_CombatantMap.find( attackerNetId );
	if ( attackerIt == m_CombatantMap.end() ) {
		kbError( "EtherAIManager::GetCloseCombatSpotOnTarget() - Attacker with id %d has not previously registered", attackerNetId );
		return false;
	}
	CombatantInfo_t & attackerInfo = attackerIt->second;

	// Get target's info
	const uint nextTargetNetId = target->GetOwner()->GetNetId();
	std::map<int, CombatantInfo_t>::iterator targetIt = m_CombatantMap.find( nextTargetNetId );
	if ( targetIt == m_CombatantMap.end() ) {
		kbError( "EtherAIManager::GetCloseCombatSpotOnTarget() - Target with id %d has not previously registered", nextTargetNetId );
		return false;
	}
	CombatantInfo_t & nextTargetInfo = targetIt->second;

	// Check if the target has any available spots
	bool bHasAvailableSpot = false;
	for ( int iCloseSpot = 0; iCloseSpot < NUM_CLOSE_RANGE_SPOTS; iCloseSpot++ ) {
		if ( nextTargetInfo.m_CloseRangeSpots[iCloseSpot] == -1 ) {
			bHasAvailableSpot = true;
			break;
		}
	}

	if ( bHasAvailableSpot == false ) {
		return false;
	}

	if ( attackerInfo.m_MyTargetId != -1 && attackerInfo.m_MyTargetId != nextTargetNetId ) {
		// If the attacker is changing targets, inform the previous target
		std::map<int, CombatantInfo_t>::iterator previousTargetIt = m_CombatantMap.find( attackerInfo.m_MyTargetId );
		if ( previousTargetIt != m_CombatantMap.end() ) {
			CombatantInfo_t & PreviousTargetInfo = previousTargetIt->second;
			bool targetSpotFound = false;
			for ( int iCloseSpot = 0; iCloseSpot < NUM_CLOSE_RANGE_SPOTS; iCloseSpot++ ) {
				if ( PreviousTargetInfo.m_CloseRangeSpots[iCloseSpot] != attackerNetId ) {
					continue;
				}

				targetSpotFound = true;
				PreviousTargetInfo.m_CloseRangeSpots[iCloseSpot] = -1;
			}

			if ( targetSpotFound == false ) {
				kbError( "EtherAIManager::GetCloseCombatSpotOnTarget() - Attacker %d has target %d, but isn't in the target's attacker list", attackerNetId, attackerInfo.m_MyTargetId );
			}
		}
	}

	for ( int i = 0; i < NUM_CLOSE_RANGE_SPOTS; i++ ) {
		if ( nextTargetInfo.m_CloseRangeSpots[i] == attackerNetId ) {
			out_GoalPosition = target->GetOwner()->GetPosition() + CombatantInfo_t::GetCloseOffset( i );
			attackerInfo.m_MyTargetId = nextTargetNetId;
			return true;
		}
	}

	// Find ideal spot
	std::vector<int> shuffledSlots;
	for ( int i = 0; i < NUM_CLOSE_RANGE_SPOTS; i++ ) {
		shuffledSlots.push_back( i );
	}

	int foundSpot = -1;
	for ( int i = 0; i < NUM_CLOSE_RANGE_SPOTS; i++ ) {
		int randIdx = rand() % ( NUM_CLOSE_RANGE_SPOTS - i );
		if ( nextTargetInfo.m_CloseRangeSpots[randIdx] == -1 ) {
			nextTargetInfo.m_CloseRangeSpots[randIdx] = attackerNetId;
			foundSpot = randIdx;
			break;
		}
	}

	if ( foundSpot == -1 ) {
		return false;
	}



	out_GoalPosition = target->GetOwner()->GetPosition() + CombatantInfo_t::GetCloseOffset( foundSpot );

	attackerInfo.m_MyTargetId = nextTargetNetId;*/
	return true;
}

/**
 *	EtherAIManager::Update 
 */
void EtherAIManager::Update( const float DeltaTime ) {
	if ( g_DebugAI.GetBool() && m_AIList.size() > 0 ) {

		std::string aiDebugText = "Num Registered Combatants: ";
		aiDebugText += std::to_string( m_CombatantMap.size() );
		g_pRenderer->DrawDebugText( aiDebugText, 0, 0, g_DebugTextSize, 0.1f, kbColor::red );

		if ( m_DebugAIIdx >= m_AIList.size() ) {
			m_DebugAIIdx = 0;
		}

		kbGameEntity *const pTargetEntity = m_AIList[m_DebugAIIdx].GetEntity();
		if ( pTargetEntity != nullptr && pTargetEntity->GetComponentByType( EtherAIComponent::GetType() ) ) {
			const kbBounds attackerBounds( pTargetEntity->GetPosition() + kbVec3( 10.0f, 25.0f, 10.0f ), pTargetEntity->GetPosition() - kbVec3( 10.0f, 25.0f, 10.0f ) );
			g_pRenderer->DrawBox( attackerBounds, kbColor::red );
		
			g_pRenderer->DrawDebugText( pTargetEntity->GetName(), 0.01f, 0.25f, 0.02f, 0.02f, kbColor::red );
		
			const EtherSkelModelComponent *const pSkelModelComponent = (EtherSkelModelComponent*)pTargetEntity->GetComponentByType( EtherSkelModelComponent::GetType() );
			if ( pSkelModelComponent != nullptr && pSkelModelComponent->GetCurAnimationName() != nullptr ) {
				g_pRenderer->DrawDebugText( pSkelModelComponent->GetCurAnimationName()->stl_str(), 0.01f, 0.28f, 0.02f, 0.02f, kbColor::red );
			
			}
		}

/*
		for ( int i = 0; i < g_pGame->GetPlayersList().size(); i++ ) {
			const kbGameEntity *const pTargetPlayer = g_pGame->GetPlayersList()[i];
			if ( pTargetPlayer == nullptr ) {
				continue;
			}

			kbGameEntityPtr entPtr;
			entPtr.SetEntity( pTargetPlayer->GetOwner() );
			std::map<kbGameEntityPtr, CombatantInfo_t>::const_iterator playerIt = m_CombatantMap.find( entPtr );
			if ( playerIt == m_CombatantMap.end() ) {
				continue;
			}

			const CombatantInfo_t & playerInfo = playerIt->second;
			for ( int iAttacker = 0; iAttacker < NUM_CLOSE_RANGE_SPOTS; iAttacker++ ) {
				if ( playerInfo.m_CloseRangeSpots[iAttacker] != -1 ) {
					const kbGameEntity *const pAttacker = g_pGame->GetGameEntitybyId( playerInfo.m_CloseRangeSpots[iAttacker] );
					if ( pAttacker == nullptr ) {
						continue;
					}

					const kbVec3 goalPos = pTargetPlayer->GetPosition() + CombatantInfo_t::GetCloseOffset( iAttacker );
					kbVec3 finalPos;
					((EtherGame*)g_pGame)->TraceAgainstWorld( goalPos - kbVec3( 0.0f, 1000.0f, 0.0f ), goalPos + ( 0.0f, -1000.0f, 0.0f ), finalPos, false );
					g_pRenderer->DrawLine( pAttacker->GetPosition(), finalPos, kbColor::red );

					g_pRenderer->DrawSphere( finalPos, 5.0f, 12, kbColor::red );
				}
			}
		}*/;
	}
}

/**
 *	EtherAIManager::InputKeyPressedCB
 */
void EtherAIManager::InputKeyPressedCB( const int cbParam ) {
	if ( cbParam == 0 ) {
		g_DebugAI.SetBool( !g_DebugAI.GetBool() );
	} else if ( cbParam == 1 ) {
		if ( g_DebugAI.GetBool() == false ) {
			return;
		}

		m_DebugAIIdx++;
		if ( m_DebugAIIdx >= this->m_AIList.size() ) {
			m_DebugAIIdx = 0;
		}
	}
}
