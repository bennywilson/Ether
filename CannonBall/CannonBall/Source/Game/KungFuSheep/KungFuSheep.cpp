//===================================================================================================
// KungFuSheep.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "KungFuLevelComponent.h"
#include "KungFuSheep.h"
#include "KungFuSnolaf.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

/**
 *	KungFuSheepStateIdle
 */
const kbVec3 g_LeftFacing( -0.01f, 0.0f, -1.0f );
const kbVec3 g_RightFacing( -0.01f, 0.0f, 1.0f );

template<typename T>
class KungFuSheepStateIdle : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateIdle( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T prevState ) override {

		static const kbString IdleL_Anim( "IdleLeft_Basic" );
		static const kbString IdleR_Anim( "IdleRight_Basic" );

		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();
		if ( targetDir.z < 0.0f ) {
			m_pActorComponent->PlayAnimation( IdleL_Anim, 0.05f );
		} else {
			m_pActorComponent->PlayAnimation( IdleR_Anim, 0.05f );
		}
	}

	virtual void UpdateState() override {

		const kbInput_t & input = g_pInputManager->GetInput();

		const float curTime = g_GlobalTimer.TimeElapsedSeconds();

		if ( input.IsKeyPressedOrDown( kbInput_t::KB_SPACE ) ) {
			RequestStateChange( KungFuSheepState::Attack );
		} else if ( input.IsKeyPressedOrDown( 'C' ) && curTime > m_LastCannonBallTime + 2.0f ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( input.IsKeyPressedOrDown( 'A' ) ) {
			RequestStateChange( KungFuSheepState::Run );
			m_pActorComponent->SetTargetFacingDirection( g_LeftFacing );
		} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
			RequestStateChange( KungFuSheepState::Run );
			m_pActorComponent->SetTargetFacingDirection( g_RightFacing );
		}
	}

	virtual void EndState( T nextState ) override { }

private:
	float m_LastCannonBallTime = -1.0f;
};

/**
 *	KungFuSheepStateRun
 */
template<typename T>
class KungFuSheepStateRun : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateRun( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString Run_Anim( "Run_Basic" );
		m_pActorComponent->PlayAnimation( Run_Anim, 0.05f, false );
	}

	virtual void UpdateState() override {

		const kbInput_t & input = g_pInputManager->GetInput();

		const float frameDT = g_pGame->GetFrameDT();
		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();

		if ( input.IsKeyPressedOrDown( 'C' ) ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( targetDir.z < 0.0f ) {
			if ( input.IsKeyPressedOrDown( 'A' ) ) {
				
				// Run
				const kbVec3 moveDir( 0.0f, 0.0f, -1 );
				const kbVec3 targetPos = m_pActorComponent->GetOwnerPosition() - moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
				m_pActorComponent->SetOwnerPosition( targetPos );

			} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
				m_pActorComponent->SetTargetFacingDirection( g_RightFacing );
			} else {
				RequestStateChange( KungFuSheepState::Idle );
			}

		} else {
			if ( input.IsKeyPressedOrDown( 'D' ) ) {

				// Run
				const kbVec3 moveDir( 0.0f, 0.0f, 1 );
				const kbVec3 targetPos = m_pActorComponent->GetOwnerPosition() - moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
				m_pActorComponent->SetOwnerPosition( targetPos );

			} else if ( input.IsKeyPressedOrDown( 'A' ) ) {
				m_pActorComponent->SetTargetFacingDirection( g_LeftFacing );
			} else {
				RequestStateChange( KungFuSheepState::Idle );
			}
		}
	}

	virtual void EndState( T ) override { }
};

/**
 *	KungFuSheepStateAttack
 */
template<typename T>
class KungFuSheepStateAttack : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateAttack( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString PunchL_Anim( "PunchLeft_Basic" );
		static const kbString KickL_Anim( "KickLeft_Basic" );
		static const kbString PunchR_Anim( "PunchRight_Basic" );
		static const kbString KickR_Anim( "KickRight_Basic" );

		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();
		if ( targetDir.z < 0.0f ) {
			if ( rand() % 2 == 0 ) {
				m_pActorComponent->PlayAnimation( PunchL_Anim, 0.05f );
			} else {
				m_pActorComponent->PlayAnimation( KickL_Anim, 0.05f );
			}
		} else {
			if ( rand() % 2 == 0 ) {
				m_pActorComponent->PlayAnimation( PunchR_Anim, 0.05f );
			} else {
				m_pActorComponent->PlayAnimation( KickR_Anim, 0.05f );
			}
		}
	}

	virtual void UpdateState() override {

		static const kbString PunchL_Anim( "PunchLeft_Basic" );
		static const kbString KickL_Anim( "KickLeft_Basic" );
		static const kbString PunchR_Anim( "PunchRight_Basic" );
		static const kbString KickR_Anim( "KickRight_Basic" );

		if ( m_pActorComponent->IsPlayingAnim( PunchL_Anim ) == false &&
			 m_pActorComponent->IsPlayingAnim( KickL_Anim ) == false &&
			 m_pActorComponent->IsPlayingAnim( PunchR_Anim ) == false && 
			 m_pActorComponent->IsPlayingAnim( KickR_Anim ) == false ) {
			
			RequestStateChange( KungFuSheepState::Idle );
		}
	}
};

/**
 *	KungFuSheepStateHugged
 */
template<typename T>
class KungFuSheepStateHugged : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateHugged( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {
		m_NumDirectionChanges = 0;
		m_CurrentDirection = 0;
	}

	virtual void UpdateState() override {
		const kbInput_t & input = g_pInputManager->GetInput();

		const float frameDT = g_pGame->GetFrameDT();
		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();

		if ( input.IsKeyPressedOrDown( 'C' ) ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( input.IsKeyPressedOrDown( 'A' ) ) {
			if ( m_CurrentDirection == 1 ) {
				m_CurrentDirection = 0;
				m_NumDirectionChanges++;
				m_LastDirectionChangeTime = curTime;
			}
			m_pActorComponent->SetTargetFacingDirection( g_LeftFacing );
		} else if ( input.IsKeyPressedOrDown( 'D' ) ) {

			if ( m_CurrentDirection == 0 ) {
				m_CurrentDirection = 1;
				m_NumDirectionChanges++;
				m_LastDirectionChangeTime = curTime;

			}

			m_pActorComponent->SetTargetFacingDirection( g_RightFacing );
		}

		if ( curTime > m_LastDirectionChangeTime + m_DirectionChangeWindowSec ) {
			m_NumDirectionChanges = 0;
		}

		static const kbString IdleL_Anim( "IdleLeft_Basic" );
		static const kbString IdleR_Anim( "IdleRight_Basic" );
		if ( m_pActorComponent->GetTargetFacingDirection().z < 0.0f ) {
			m_pActorComponent->PlayAnimation( IdleL_Anim, 0.05f );
		} else {
			m_pActorComponent->PlayAnimation( IdleR_Anim, 0.05f );
		}

		int numHuggers = 0;
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pEnt =  g_pCannonGame->GetGameEntities()[i];
			if ( pEnt->GetActorComponent() == nullptr ) {
				continue;
			}

			KungFuSnolafComponent *const pSnolaf = g_pCannonGame->GetGameEntities()[i]->GetActorComponent()->GetAs<KungFuSnolafComponent>();
			if ( pSnolaf == nullptr ) {
				continue;
			}

			if ( pSnolaf->IsHugging() ) {
				numHuggers++;
			}
		}
		if ( m_NumDirectionChanges > numHuggers ) {
			RequestStateChange( KungFuSheepState::Idle );

			DealAttackInfo_t<KungFuGame::eAttackType> dealAttackInfo;
			dealAttackInfo.m_BaseDamage = 999999.0f;
			dealAttackInfo.m_pAttacker = m_pActorComponent;
			dealAttackInfo.m_Radius = 10.0f;
			dealAttackInfo.m_AttackType = KungFuGame::Shake;
			KungFuLevelComponent *const pLevelComponent = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
			const AttackHitInfo_t attackInfo = pLevelComponent->DoAttack( dealAttackInfo );
		}
	}

	virtual void EndState( T nextState ) override { }

	const float m_DirectionChangeWindowSec = 0.5f;
	int m_NumDirectionChanges = 0;
	int m_CurrentDirection = 0;
	float m_LastDirectionChangeTime = 0.0f;

};

/**
 *	KungFuSheepStateDead
 */
template<typename T>
class KungFuSheepStateDead : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateDead( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSheepStateCannonBall
 */
template<typename T>
class KungFuSheepStateCannonBall : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateCannonBall( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
	
	kbVec3 m_OldFacingDirection;

	virtual void BeginState( T prevState ) override {

		static const kbString CannonBall_Anim( "CannonBall" );
		static const kbString CannonBallWindUp_Anim( "CannonBallWindUp" );

		m_pActorComponent->PlayAnimation( CannonBallWindUp_Anim, 0.05f, false, CannonBall_Anim, 0.01f );

		kbString cur, next;
		if ( m_pActorComponent->GetOwner()->GetComponent<kbSkeletalModelComponent>()->GetCurAnimationName() != nullptr ) {
			cur = *m_pActorComponent->GetOwner()->GetComponent<kbSkeletalModelComponent>()->GetCurAnimationName();
		}

		if ( m_pActorComponent->GetOwner()->GetComponent<kbSkeletalModelComponent>()->GetNextAnimationName() != nullptr ) {
			next = *m_pActorComponent->GetOwner()->GetComponent<kbSkeletalModelComponent>()->GetNextAnimationName();
		}

		m_OldFacingDirection = m_pActorComponent->GetTargetFacingDirection();
		m_pActorComponent->SetTargetFacingDirection( kbVec3( -1.0f, 0.0f, 0.0f ) );

		m_StartCannonBallTime = g_GlobalTimer.TimeElapsedSeconds();
	}

	virtual void UpdateState() override {
		
		// Hack to avoid exiting Cannonball early.  Not sure if needed at this point
		if ( g_GlobalTimer.TimeElapsedSeconds() < m_StartCannonBallTime + 1.0f ) {
			return;
		}

		static const kbString CannonBall_Anim( "CannonBall" );
		if ( m_pActorComponent->HasFinishedAnim( CannonBall_Anim  ) ) {
			RequestStateChange( KungFuSheepState::Idle );
		}
	}

	virtual void EndState( T nextState ) override {
		m_pActorComponent->SetTargetFacingDirection( m_OldFacingDirection );
	}

	float m_StartCannonBallTime = 0.0f;
};

/**
 *	KungFuSheepComponent::Constructor
 */
void KungFuSheepComponent::Constructor() {
	m_TargetFacingDirection.Set( 0.0f, 0.0f, -1.0f );
	m_bIsPlayer = true;
}

/**
 *	KungFuSheepComponent::SetEnable_Internal
 */
void KungFuSheepComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	// Make sure sheep package is loaded
	g_ResourceManager.GetPackage( "./assets/Packages/Sheep.kbPkg" );

	if ( bEnable ) {

		KungFuSheepStateBase<KungFuSheepState::SheepStates_t> * sheepStates[] = {
			new KungFuSheepStateIdle<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateRun<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateAttack<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateHugged<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateDead<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateCannonBall<KungFuSheepState::SheepStates_t>( this )
		};

		InitializeStates( sheepStates );
		RequestStateChange( KungFuSheepState::Idle );
	}
}

/**
*	KungFuSheepComponent::OnAnimEvent
*/
void KungFuSheepComponent::OnAnimEvent( const kbAnimEventInfo_t & animEventInfo ) {

	const static kbString CannonBallImpact = "CannonBall_Impact";
	const static kbString CannonBallVO = "CannonBall_VO";
	const static kbString CannonBallJumpSmear = "CannonBall_JumpSmear";
	const static kbString CannonBallDropSmear = "CannonBall_DropSmear";
	const static kbString PunchAttack = "Punch_Attack";
	const static kbString KickAttack = "Kick_Attack";

	const kbAnimEvent & animEvent = animEventInfo.m_AnimEvent;
	const kbString & animEventName = animEvent.GetEventName();
	const float animEventVal = animEvent.GetEventValue();

	if ( animEventName == CannonBallImpact ) {

		DealAttackInfo_t<KungFuGame::eAttackType >dealAttackInfo;
		dealAttackInfo.m_BaseDamage = 999999.0f;
		dealAttackInfo.m_pAttacker = this;
		dealAttackInfo.m_Radius = 10.0f;
		dealAttackInfo.m_AttackType = KungFuGame::Cannonball;

		KungFuLevelComponent *const pLevelComponent = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
		const AttackHitInfo_t attackInfo = pLevelComponent->DoAttack( dealAttackInfo );

		if ( m_CannonBallImpactFX.GetEntity() != nullptr ) {
			kbGameEntity *const pCannonBallImpact = g_pGame->CreateEntity( m_CannonBallImpactFX.GetEntity() );
			pCannonBallImpact->SetPosition( GetOwnerPosition() );
			pCannonBallImpact->SetOrientation( GetOwnerRotation() );
			pCannonBallImpact->DeleteWhenComponentsAreInactive( true );

			CannonCameraShakeComponent *const pCamShakeComponent = (CannonCameraShakeComponent*)pCannonBallImpact->GetComponentByType( CannonCameraShakeComponent::GetType() );
			CannonCameraComponent *const pCam = (CannonCameraComponent*)g_pCannonGame->GetMainCamera();
			if ( pCamShakeComponent != nullptr && pCam != nullptr ) {
				pCam->StartCameraShake( pCamShakeComponent );
			}
			if ( m_CannonBallImpactSound.size() > 0 ) {
				m_CannonBallImpactSound[rand() % m_CannonBallImpactSound.size()].PlaySoundAtPosition( GetOwnerPosition() );
			}
		}
	} else if ( animEventName == CannonBallVO ) {
		if ( m_CannonBallVO.size() > 0 ) {
			m_CannonBallVO[rand() % m_CannonBallVO.size()].PlaySoundAtPosition( GetOwnerPosition() );
		}
	} else if ( animEventName == CannonBallJumpSmear ) {
		m_AnimSmearStartTime = g_GlobalTimer.TimeElapsedSeconds();
		m_AnimSmearVec.Set( 0.0f, m_JumpSmearMagnitude, 0.0f, 0.0f );
		m_AnimSmearDuration = animEventVal;
	} else if ( animEventName == CannonBallDropSmear ) {
		m_AnimSmearStartTime = g_GlobalTimer.TimeElapsedSeconds();
		m_AnimSmearVec.Set( 0.0f, m_DropSmearMagnitude, 0.0f, 0.0f);
		m_AnimSmearDuration = animEventVal;
	} else if ( animEventName == PunchAttack || animEventName == KickAttack ) {

		KungFuLevelComponent *const pLevelComponent = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
		
		DealAttackInfo_t<KungFuGame::eAttackType> dealAttackInfo;
		dealAttackInfo.m_BaseDamage = 999999.0f;
		dealAttackInfo.m_pAttacker = this;
		dealAttackInfo.m_Radius = 0.0f;
		dealAttackInfo.m_AttackType = KungFuGame::Punch_Kick;

		const AttackHitInfo_t attackInfo = pLevelComponent->DoAttack( dealAttackInfo );
		if ( attackInfo.m_bHit ) {
			if ( m_BasicAttackImpactSound.size() > 0 ) {
				m_BasicAttackImpactSound[rand() % m_BasicAttackImpactSound.size()].PlaySoundAtPosition( GetOwnerPosition() );
			}
		}

		if ( m_AttackVO.size() > 0 ) {
			m_AttackVO[rand() % m_AttackVO.size()].PlaySoundAtPosition( GetOwnerPosition() );
		}
	}
}

 /**
  *	KungFuSheepComponent::Update_Internal
  */
void KungFuSheepComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	UpdateStateMachine();
}

 /**
  *	KungFuSheepComponent::IsCannonBalling
  */
bool KungFuSheepComponent::IsCannonBalling() const {
	return ( m_CurrentState == KungFuSheepState::CannonBall );
}

 /**
  *	KungFuSheepComponent::TakeDamage
  */
void KungFuSheepComponent::TakeDamage( const DealAttackInfo_t<KungFuGame::eAttackType> & attackInfo ) {
	
	if ( IsCannonBalling() ) {
		return;
	}

	KungFuSnolafComponent *const pSnolaf = attackInfo.m_pAttacker->GetAs<KungFuSnolafComponent>();
	if ( pSnolaf != nullptr && m_CurrentState != KungFuSheepState::Attack ) {
		RequestStateChange( KungFuSheepState::Hugged );
	}
}