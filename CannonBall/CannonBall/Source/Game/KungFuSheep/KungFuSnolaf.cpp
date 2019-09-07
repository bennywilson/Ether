//===================================================================================================
// KungFuSnolaf.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "KungFuLevelComponent.h"
#include "KungFuSnolaf.h"
#include "KungFuSheep.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

/**
 *	KungFuSnolafStateIdle
 */
template<typename T>
class KungFuSnolafStateIdle : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateIdle( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }


	virtual void BeginState( T prevState ) override {

		static const kbString Idle_Anim( "Idle_1" );
		m_pActorComponent->PlayAnimation( Idle_Anim, 0.05f );
	}

	virtual void UpdateState() override {

		if ( GetTarget() != nullptr ) {
			RotateTowardTarget();

			if ( GetDistanceToTarget() > 2.0f ) {
				RequestStateChange( KungFuSnolafState::Run );
				return;
			}
		}
	}

	virtual void EndState( T ) override { }
};

/**
 *	KungFuSnolafStateRun
 */
template<typename T>
class KungFuSnolafStateRun : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateRun( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString Run_Anim( "Run" );
		m_pActorComponent->PlayAnimation( Run_Anim, 0.05f );

		GetSnolaf()->EnableSmallLoveHearts( true );
	}

	virtual void UpdateState() override {

		const float frameDT = g_pGame->GetFrameDT();

		if ( GetTarget() == nullptr ) {
			RequestStateChange( KungFuSnolafState::Idle );
			return;
		}

		const kbVec3 snolafPos = m_pActorComponent->GetOwnerPosition();
		const kbVec3 snolafFacingDir = m_pActorComponent->GetOwnerRotation().ToMat4()[2].ToVec3();

		// Look for actors to hug
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pGameEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent *const pTargetActor = pGameEnt->GetComponent<CannonActorComponent>();
			if ( pTargetActor == nullptr || pTargetActor == m_pActorComponent ) {
				continue;
			}

			const kbVec3 targetPos = pTargetActor->GetOwnerPosition();
			const kbVec3 vSnolafToTarget = targetPos - snolafPos;
			const float snolafToTargetDist = ( targetPos - snolafPos ).Length();
			auto pSnolafComponent = pTargetActor->GetAs<KungFuSnolafComponent>();
			if ( pSnolafComponent != nullptr && pSnolafComponent->GetState() != KungFuSnolafState::Hug && pSnolafComponent->GetState() != KungFuSnolafState::WatchCannonBall ) {
				continue;
			}

			const float radius = ( pSnolafComponent != nullptr ) ? ( 0.5f ) : ( 0.75f );
			if ( snolafToTargetDist < radius ) {

				if ( vSnolafToTarget.Dot( snolafFacingDir ) > 0.0f ) {
					continue;
				}
	
				DealAttackInfo_t<KungFuGame::eAttackType> dealAttackInfo;
				dealAttackInfo.m_BaseDamage = 999999.0f;
				dealAttackInfo.m_pAttacker = m_pActorComponent;
				dealAttackInfo.m_Radius = 0.0f;
				dealAttackInfo.m_AttackType = KungFuGame::Hug;

				KungFuLevelComponent *const pLevelComponent = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
				pLevelComponent->DoAttack( dealAttackInfo );

				RequestStateChange( KungFuSnolafState::Hug );
				return;
			}
		}

		// Move towards target
		RotateTowardTarget();

		kbVec3 moveDir( 0.0f, 0.0f, 0.0f );
		if ( IsTargetOnLeft() ) {
			moveDir.z = 1.0f;
		} else {
			moveDir.z = -1.0f;
		}
		const kbVec3 newSnolafPos = m_pActorComponent->GetOwnerPosition() + moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
		m_pActorComponent->SetOwnerPosition( newSnolafPos );
	}

	virtual void EndState( T ) override {
		GetSnolaf()->EnableSmallLoveHearts( false );
	}
};

/**
 *	KungFuSnolafStateHug - Warm Hugs!
 */
template<typename T>
class KungFuSnolafStateHug : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateHug( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString HugLeft_Anim( "Hug_Left" );
		static const kbString HugRight_Anim( "Hug_Right" );
		static const kbString HugForward_Anim( "Hug_Forward" );

		if ( GetTarget() == nullptr ) {
			RequestStateChange( KungFuSnolafState::Idle );
			return;

		}

		KungFuSheepComponent *const pSheep = GetTarget()->GetAs<KungFuSheepComponent>();
		if ( pSheep->IsCannonBalling() ) {
			m_pActorComponent->PlayAnimation( HugForward_Anim, 0.05f );
			m_bCanWatchCannonball = false;
		} else {

			if ( IsTargetOnLeft() ) {
				m_pActorComponent->PlayAnimation( HugLeft_Anim, 0.05f );
			} else {
				m_pActorComponent->PlayAnimation( HugRight_Anim, 0.05f );
			}

			GetSnolaf()->EnableLargeLoveHearts( true );
			m_bCanWatchCannonball = true;
		}
	}

	virtual void UpdateState() override {

 		const float frameDT = g_pGame->GetFrameDT();
		
		if ( GetTarget() == nullptr ) {
			RequestStateChange( KungFuSnolafState::Idle );
			return;
		}

		if ( m_bCanWatchCannonball ) {
			KungFuSheepComponent *const pSheep = GetTarget()->GetAs<KungFuSheepComponent>();
			if ( pSheep->IsCannonBalling() ) {
				RequestStateChange( KungFuSnolafState::WatchCannonBall );
				return;
			}
		}

		const kbVec3 snolafPos = m_pActorComponent->GetOwnerPosition();
		const kbVec3 snolafFacingDir = m_pActorComponent->GetOwnerRotation().ToMat4()[2].ToVec3();

		bool bAnyoneInFront = false;
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pGameEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent *const pTargetActor = pGameEnt->GetComponent<CannonActorComponent>();
			if ( pTargetActor == nullptr || pTargetActor == m_pActorComponent ) {
				continue;
			}

			const kbVec3 targetPos = pTargetActor->GetOwnerPosition();
			const kbVec3 vSnolafToTarget = targetPos - snolafPos;
			const float snolafToTargetDist = ( targetPos - snolafPos ).Length();
			if ( snolafToTargetDist < 0.85f ) {

				if ( vSnolafToTarget.Dot( snolafFacingDir ) > 0.0f ) {
					continue;
				}

				bAnyoneInFront = true;
				break;
			}
		}

		if ( bAnyoneInFront == false ) {
			RequestStateChange( KungFuSnolafState::Run );
			return;
		}
	}

	virtual void EndState( T ) override {
		GetSnolaf()->EnableLargeLoveHearts( false );
	}

private:
	bool m_bCanWatchCannonball = false;
};


/**
 *	KungFuSnolafStateWatchCannonBall
 */
template<typename T>
class KungFuSnolafStateWatchCannonBall : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateWatchCannonBall( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }


	virtual void BeginState( T ) override {

		static const kbString Watch_CannonBall( "Watch_CannonBall" );
		m_pActorComponent->PlayAnimation( Watch_CannonBall, 0.05f );
	}

	virtual void UpdateState() override { }

	virtual void EndState( T ) override { }
};


/**
 *	KungFuSnolafStateDead
 */
template<typename T>
class KungFuSnolafStateDead : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateDead( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		GetSnolaf()->EnableSmallLoveHearts( false );
		GetSnolaf()->EnableLargeLoveHearts( false );

		const DealAttackInfo_t<KungFuGame::eAttackType> & lastAttackInfo = GetSnolaf()->GetLastAttackInfo();
		if ( lastAttackInfo.m_AttackType == KungFuGame::Shake ) {
			m_DeathSelection = 0;
		}  else {
			const int numDeaths = 4;
			m_DeathSelection = rand() % numDeaths;
		}

		m_OwnerStartPos = GetSnolaf()->GetOwnerPosition();
		m_OwnerStartRotation = m_pActorComponent->GetOwnerRotation();

		kbGameEntity *const pOwner = GetSnolaf()->GetOwner();
		for ( int i = 0; i < pOwner->NumComponents(); i++ ) {
			kbParticleComponent *const pParticle = pOwner->GetComponent( i )->GetAs<kbParticleComponent>();
			if ( pParticle == nullptr ) {
				continue;
			}

			pParticle->Enable( false );
		}

		kbMat4 worldMatrix;
		m_pActorComponent->GetOwner()->CalculateWorldMatrix( worldMatrix );
		const XMMATRIX inverseMat = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( worldMatrix ) );
		worldMatrix = kbMat4FromXMMATRIX( inverseMat );

		if ( m_DeathSelection == 0 ) {

			// Super Fly Off
			m_Velocity = kbVec3Rand( m_MinLinearVelocity, m_MaxLinearVelocity );
			if ( m_Velocity.x < 0.0f ) {
				m_Velocity.x -= 0.0025f;
			} else {
				m_Velocity.x += 0.0025f;
			}

			m_Velocity = m_Velocity * worldMatrix;

			m_RotationAxis = kbVec3( kbfrand() - 1.3f, 0.0f, 0.0f );
			if ( m_RotationAxis.LengthSqr() < 0.01f ) {
				m_RotationAxis.Set( 1.0f, 0.0f, 0.0f );
			} else {
				m_RotationAxis.Normalize();
			}
			m_RotationSpeed = kbfrand() * ( m_MaxAngularVelocity - m_MinAngularVelocity ) + m_MinAngularVelocity;

			const kbVec3 initialSnolafOffset = m_Velocity.Normalized() * 2.0f;
			m_OwnerPosOverride = m_pActorComponent->GetOwnerPosition() + initialSnolafOffset;
			m_OwnerStartPos = m_OwnerPosOverride;
			GetSnolaf()->ApplyAnimSmear( -initialSnolafOffset * 0.75f, 0.067f );
			UpdateFlyingDeath( 0.0f );

		} else if ( m_DeathSelection == 1 ) {

			// Straight up poof, homie
			GetSnolaf()->DoPoofDeath();

		} else if ( m_DeathSelection == 2 ) {

			// Decapitation
			GetSnolaf()->SpawnAndFlingDecapHead();
			m_Velocity = kbVec3Rand( m_MinLinearVelocity, m_MaxLinearVelocity );
			if ( m_Velocity.x < 0.0f ) {
				m_Velocity.x -= 0.0025f;
			} else {
				m_Velocity.x += 0.0025f;
			}

			m_Velocity = 2.0f * m_Velocity * worldMatrix;

			m_RotationAxis = kbVec3( kbfrand() - 1.3f, 0.0f, 0.0f );
			if ( m_RotationAxis.LengthSqr() < 0.01f ) {
				m_RotationAxis.Set( 1.0f, 0.0f, 0.0f );
			} else {
				m_RotationAxis.Normalize();
			}
			m_RotationSpeed = 0.15f * ( kbfrand() * ( m_MaxAngularVelocity - m_MinAngularVelocity ) + m_MinAngularVelocity );

			const kbVec3 initialSnolafOffset = m_Velocity.Normalized() * 2.0f;
			m_OwnerPosOverride = m_pActorComponent->GetOwnerPosition() + initialSnolafOffset;
			m_OwnerStartPos = m_OwnerPosOverride;
			GetSnolaf()->ApplyAnimSmear( -initialSnolafOffset * 0.75f, 0.067f );
			UpdateFlyingDeath( 0.0f );

			const static kbString clipMapMaskParam = "clipMapMask";
			kbGameEntity *const pOwner = m_pActorComponent->GetOwner();
			for ( int i = 0; i < pOwner->NumComponents(); i++ ) {
				kbSkeletalModelComponent *const pSkelModelComp = pOwner->GetComponent(i)->GetAs<kbSkeletalModelComponent>();
				if ( pSkelModelComp == nullptr ) {
					continue;
				}
				pSkelModelComp->SetMaterialParamVector( 0, clipMapMaskParam.stl_str(), kbVec4( 1.0f, 0.0f, 0.0f, 0.0f ) );
			}
		} else if ( m_DeathSelection == 3 ) {
			GetSnolaf()->SpawnAndFlingTopAndBottomHalf();
		}

		m_DeathStartTime = g_GlobalTimer.TimeElapsedSeconds();

	}

	void UpdateFlyingDeath( const float dt ) {
		
		kbGameEntity *const pOwner = m_pActorComponent->GetOwner();
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		const float elapsedDeathTime = curTime - m_DeathStartTime;

		m_OwnerPosOverride.x += m_Velocity.x * dt;
		m_OwnerPosOverride.z += m_Velocity.z * dt;

		m_OwnerPosOverride.y = m_OwnerStartPos.y + m_Velocity.y * elapsedDeathTime - ( 0.5f * -m_Gravity.y * elapsedDeathTime * elapsedDeathTime );

		m_pActorComponent->SetOwnerPosition( m_OwnerPosOverride );

		const static kbString spine3BoneName( "Spine3" );
		kbSkeletalModelComponent *const pSnolafComp = pOwner->GetComponent<kbSkeletalModelComponent>();
		kbVec3 spine3WorldPos = kbVec3::zero;

		if ( pSnolafComp->GetBoneWorldPosition( spine3BoneName, spine3WorldPos ) ) {

			kbVec3 vecOffset = m_OwnerPosOverride - spine3WorldPos;
			pOwner->SetPosition( m_OwnerPosOverride + vecOffset );
		}

		m_CurRotationAngle += m_RotationSpeed * dt;
		kbQuat rot;
		rot.FromAxisAngle( m_RotationAxis, m_CurRotationAngle );
		rot =  m_OwnerStartRotation * rot;
		m_pActorComponent->SetOwnerRotation( rot );
	}

	virtual void UpdateState() override {

		kbGameEntity *const pOwner = m_pActorComponent->GetOwner();
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		const float elapsedDeathTime = curTime - m_DeathStartTime;
		if ( elapsedDeathTime > 2.0f ) {
			g_pCannonGame->RemoveGameEntity( pOwner );
			return;
		}

		const float dt = g_pGame->GetFrameDT();

		if ( m_DeathSelection == 0 || m_DeathSelection == 2 ) {
			UpdateFlyingDeath( dt );
		}

	}

	virtual void EndState( T ) override { }

private:

	const kbVec3 m_MinLinearVelocity = kbVec3( -0.015f, 0.015f, 0.03f );
	const kbVec3 m_MaxLinearVelocity = kbVec3( 0.015f, 0.025f, 0.02f );
	const float m_MinAngularVelocity = 10.0f;
	const float m_MaxAngularVelocity = 15.0f;
	const kbVec3 m_Gravity = kbVec3( 0.0f, -20.0f, 0.0f );

	kbVec3 m_OwnerStartPos = kbVec3::zero;
	kbQuat m_OwnerStartRotation = kbQuat( 0.0f, 0.0f, 0.0f, 1.0f );
	kbVec3 m_OwnerPosOverride = kbVec3::zero;

	kbVec3 m_Velocity = kbVec3::zero;
	kbVec3 m_RotationAxis = kbVec3( 1.0f, 0.0f, 0.0f );

	float m_CurRotationAngle = 0.0f;
	float m_RotationSpeed = 1.0f;
	int m_DeathSelection = 0;
	float m_DeathStartTime = 0.0f;
};

/**
 *	KungFuSnolafComponent::Constructor
 */
void KungFuSnolafComponent::Constructor() {
	m_pSmallLoveHearts = nullptr;
	m_pLargeLoveHearts = nullptr;
}

/**
 *	KungFuSnolafComponent::SetEnable_Internal
 */
void KungFuSnolafComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	// Make sure sheep package is loaded
	g_ResourceManager.GetPackage( "./assets/Packages/Snolaf.kbPkg" );

	m_pSmallLoveHearts = nullptr;
	if ( bEnable ) {

		// TODO: NEEDED?
		if ( m_SkelModelsList.size() > 1 ) {
			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), kbVec4::zero );
		}

		static const kbString SmallLoveHearts( "Small_LoveHearts" );
		static const kbString LargeLoveHearts( "Large_LoveHearts" );

		for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
			if ( GetOwner()->GetComponent(i)->IsA( kbParticleComponent::GetType() ) == false ) {
				continue;
			}

			kbParticleComponent *const pParticle = (kbParticleComponent*) GetOwner()->GetComponent(i);

			if ( pParticle->GetName() == SmallLoveHearts.stl_str() ) {
				m_pSmallLoveHearts = pParticle;
				m_pSmallLoveHearts->EnableNewSpawns( false );
			} else if ( pParticle->GetName() == LargeLoveHearts.stl_str() ) {
				m_pLargeLoveHearts = pParticle;
				m_pLargeLoveHearts->EnableNewSpawns( false );
			}
		}

		KungFuSnolafStateBase<KungFuSnolafState::SnolafState_t> * snolafStates[] = {
			new KungFuSnolafStateIdle<KungFuSnolafState::SnolafState_t>( this ),
			new KungFuSnolafStateRun<KungFuSnolafState::SnolafState_t>( this ),
			new KungFuSnolafStateHug<KungFuSnolafState::SnolafState_t>( this ),
			new KungFuSnolafStateDead<KungFuSnolafState::SnolafState_t>( this ),
			new KungFuSnolafStateWatchCannonBall<KungFuSnolafState::SnolafState_t>( this )
		};

		InitializeStates( snolafStates );
		RequestStateChange( KungFuSnolafState::Idle );
	}
}

/**
*	KungFuSnolafComponent::OnAnimEvent
*/
void KungFuSnolafComponent::OnAnimEvent( const kbAnimEventInfo_t & animEventInfo ) {

	static const kbString LeftFootStep( "Step_LeftFoot" );
	static const kbString RightFootStep( "Step_RightFoot" );
	static const kbString LeftFootBone( "L_Foot" );
	static const kbString RightFootBone( "R_Foot" );

	const kbAnimEvent & animEvent = animEventInfo.m_AnimEvent;
	if ( animEvent.GetEventName() == LeftFootStep || animEvent.GetEventName() == RightFootStep ) {
		if ( m_FootStepImpactFX.GetEntity() != nullptr ) {
			kbGameEntity *const pFootStepFX = g_pGame->CreateEntity( m_FootStepImpactFX.GetEntity() );

			const kbString footBone = ( animEvent.GetEventName() == LeftFootStep ) ? ( LeftFootBone ) : ( RightFootBone );
			kbVec3 decalPosition = kbVec3::zero;

			//const kbString boneName
			m_SkelModelsList[0]->GetBoneWorldPosition( footBone, decalPosition  ) ;

			pFootStepFX->SetPosition( decalPosition );
			pFootStepFX->DeleteWhenComponentsAreInactive( true );
		}
	}
}

 /**
  *	KungFuSnolafComponent::Update_Internal
  */
void KungFuSnolafComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	UpdateStateMachine();

	kbVec4 fxDot( 1.0f, 0.0f, 0.0f, 0.0f );
	if ( m_CurrentState == KungFuSnolafState::Hug ) {
		fxDot.Set( 0.0f, 1.0f, 0.0f, 0.0f );
	} else if ( m_CurrentState == KungFuSnolafState::Dead ) {
		fxDot.Set( 0.0f, 0.0f, 1.0f, 0.0f );
	}

	if ( m_SkelModelsList.size() > 0 ) {
		const static kbString fxMapMaskParam = "fxMapMask";
		m_SkelModelsList[0]->SetMaterialParamVector( 0, fxMapMaskParam.stl_str(), fxDot );
	}
}

/**
 *	KungFuSnolafComponent::EnableSmallLoveHearts
 */
void KungFuSnolafComponent::EnableSmallLoveHearts( const bool bEnable ) {
	if ( m_pSmallLoveHearts == nullptr ) {
		return;
	}

	m_pSmallLoveHearts->EnableNewSpawns( bEnable );
}

/**
 *	KungFuSnolafComponent::EnableLargeLoveHearts
 */
void KungFuSnolafComponent::EnableLargeLoveHearts( const bool bEnable ) {
	if ( m_pLargeLoveHearts == nullptr ) {
		return;
	}

	m_pLargeLoveHearts->EnableNewSpawns( bEnable );
}

/**
 *	KungFuSnolafComponent::TakeDamage
 */
void KungFuSnolafComponent::TakeDamage( const DealAttackInfo_t<KungFuGame::eAttackType> & attackInfo ) {

	m_LastAttackInfo = attackInfo;
	if ( attackInfo.m_AttackType == KungFuGame::Shake ) {
		// Shake 'n Bake only kills huggers
		if ( m_CurrentState == KungFuSnolafState::Hug ) {
			m_Health = -1.0f;
			RequestStateChange( KungFuSnolafState::Dead );
		}
		return;
	}
	m_Health = -1.0f;
	RequestStateChange( KungFuSnolafState::Dead );
}

/**
 *	KungFuSnolafComponent::DoPoofDeath
 */
void KungFuSnolafComponent::DoPoofDeath() {

	m_SkelModelsList[0]->Enable( false );
	m_SkelModelsList[1]->Enable( false );

	if ( m_PoofDeathFX.GetEntity() == nullptr ) {
		return;
	}

	kbGameEntity *const pCannonBallImpact = g_pGame->CreateEntity( m_PoofDeathFX.GetEntity() );
	pCannonBallImpact->SetPosition( GetOwnerPosition() );
	pCannonBallImpact->SetOrientation( GetOwnerRotation() );
	pCannonBallImpact->DeleteWhenComponentsAreInactive( true );

	g_pCannonGame->RemoveGameEntity( this->GetOwner() );
}

/**
 *	KungFuSnolafComponent::SpawnAndFlingDecapHead
 */
void KungFuSnolafComponent::SpawnAndFlingDecapHead() {

	if ( m_DecapitatedHead.GetEntity() == nullptr ) {
		return;
	}

	kbGameEntity *const pDecapHead = g_pGame->CreateEntity( m_DecapitatedHead.GetEntity() );
	const kbVec3 headPos = GetOwnerPosition() + kbVec3( 0.0f, 1.75f, 0.0f );
	pDecapHead->SetPosition( headPos );
	pDecapHead->SetOrientation( GetOwnerRotation() );
	pDecapHead->DeleteWhenComponentsAreInactive( true );

	kbFlingPhysicsComponent *const pFlingComp = pDecapHead->GetComponent<kbFlingPhysicsComponent>();
	if ( pFlingComp != nullptr ) {
		pFlingComp->Enable( false );
		pFlingComp->Enable( true );
	}
}


/**
 *	KungFuSnolafComponent::SpawnAndFlingTopAndBottomHalf
 */
void KungFuSnolafComponent::SpawnAndFlingTopAndBottomHalf() {

	m_SkelModelsList[0]->Enable( false );
	m_SkelModelsList[1]->Enable( false );

	if ( m_TopHalfOfBody.GetEntity() == nullptr || m_BottomHalfOfBody.GetEntity() == nullptr ) {
		return;
	}

	kbGameEntity *const pTopHalf = g_pGame->CreateEntity( m_TopHalfOfBody.GetEntity() );
	const kbVec3 topPos = GetOwnerPosition() + kbVec3( 0.0f, 1.0f, 0.0f );
	pTopHalf->SetPosition( topPos );
	pTopHalf->SetOrientation( GetOwnerRotation() );
	pTopHalf->DeleteWhenComponentsAreInactive( true );

	kbFlingPhysicsComponent *const pTopFlingComp = pTopHalf->GetComponent<kbFlingPhysicsComponent>();
	if ( pTopFlingComp != nullptr ) {
		pTopFlingComp->Enable( false );
		pTopFlingComp->Enable( true );
	}

	kbGameEntity *const pBottomHalf = g_pGame->CreateEntity( m_BottomHalfOfBody.GetEntity() );
	const kbVec3 bottomPos = GetOwnerPosition() + kbVec3( 0.0f, 0.2f, 0.0f );
	pBottomHalf->SetPosition( bottomPos );
	pBottomHalf->SetOrientation( GetOwnerRotation() );
	pBottomHalf->DeleteWhenComponentsAreInactive( true );

	kbFlingPhysicsComponent *const pBottomFlingComp = pBottomHalf->GetComponent<kbFlingPhysicsComponent>();
	if ( pBottomFlingComp != nullptr ) {
		pBottomFlingComp->Enable( false );
		pBottomFlingComp->Enable( true );
	}
}

