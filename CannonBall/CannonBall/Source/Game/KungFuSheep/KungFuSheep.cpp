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

kbConsoleVariable g_CannonBallTest( "cbstresstest", false, kbConsoleVariable::Console_Bool, "Cannon ball stress test", "" );

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

	virtual void BeginState_Internal( T prevState ) override {

		static const kbString IdleL_Anim( "IdleLeft_Basic" );
		static const kbString IdleR_Anim( "IdleRight_Basic" );

		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();
		if ( targetDir.z < 0.0f ) {
			m_pActorComponent->PlayAnimation( IdleL_Anim, 0.05f );
		} else {
			m_pActorComponent->PlayAnimation( IdleR_Anim, 0.05f );
		}

		if ( prevState == KungFuSheepState::CannonBall ) {
			m_LastCannonBallTime = g_GlobalTimer.TimeElapsedSeconds();
		}

		kbLog( "Entered Idle ");
	}

	virtual void UpdateState_Internal() override {

		const kbInput_t & input = g_pInputManager->GetInput();

		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		const float frameDT = g_pGame->GetFrameDT();
		const float blockerCheckReach = KungFuGame::kDistToChase;//frameDT * m_pActorComponent->GetMaxRunSpeed() * 20.0f;
		const kbVec2 leftStick = GetLeftStick();

		const auto pKungFuSheepDirector = KungFuSheepDirector::Get();

		if ( WasAttackJustPressed() && pKungFuSheepDirector->GetNumHuggers() == 0 ) {
			RequestStateChange( KungFuSheepState::Attack );
		} else if ( WasSpecialAttackPressed() && curTime > m_LastCannonBallTime + 2.0f && GetSheep()->GetCannonBallMeterFill() >= 1.0f ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( leftStick.x < 0.0f ) {
							
			// Run
			const kbVec3 moveDir( 0.0f, 0.0f, 1 );
			if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
				RequestStateChange( KungFuSheepState::Run );
			}
			m_pActorComponent->SetTargetFacingDirection( g_LeftFacing );

		} else if ( leftStick.x > 0.0f ) {
							
			// Run
			const kbVec3 moveDir( 0.0f, 0.0f, -1 );
			if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
				RequestStateChange( KungFuSheepState::Run );
			}
			m_pActorComponent->SetTargetFacingDirection( g_RightFacing );
		}

		if ( g_CannonBallTest.GetBool() && g_UseEditor == false ) {
			if ( m_LastCannonBallTime == -1.0f || g_GlobalTimer.TimeElapsedSeconds() > m_LastCannonBallTime + 5.0f ) {
				RequestStateChange( KungFuSheepState::CannonBall );
			}
		}
	}

	virtual void EndState_Internal( T nextState ) override { }

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

	virtual void BeginState_Internal( T ) override {

				kbLog( "Entered Run ");

		static const kbString Run_Anim( "Run_Basic" );
		m_pActorComponent->PlayAnimation( Run_Anim, 0.05f, false );
	}

	virtual void UpdateState_Internal() override {

		const kbInput_t & input = g_pInputManager->GetInput();
		const kbVec2 leftStick = GetLeftStick();

		const float frameDT = g_pGame->GetFrameDT();
		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();
		const float blockerCheckReach = KungFuGame::kDistToChase;

		if ( WasAttackJustPressed() ) {
			RequestStateChange( KungFuSheepState::Attack );
		} else if ( WasSpecialAttackPressed() && GetSheep()->GetCannonBallMeterFill() >= 1.0f ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( targetDir.z < 0.0f ) {
			if ( leftStick.x < 0.0f ) {
				
				// Run
				const kbVec3 moveDir( 0.0f, 0.0f, 1 );

				if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
					const kbVec3 targetPos = m_pActorComponent->GetOwnerPosition() + moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
					m_pActorComponent->SetOwnerPosition( targetPos );
				} else {
					RequestStateChange( KungFuSheepState::Idle );
				}
			} else if ( leftStick.x > 0.0f ) {
				m_pActorComponent->SetTargetFacingDirection( g_RightFacing );
			} else {
				RequestStateChange( KungFuSheepState::Idle );
			}

		} else {
			if ( leftStick.x > 0.0f ) {

				// Run
				const kbVec3 moveDir( 0.0f, 0.0f, -1 );

				if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
					const kbVec3 targetPos = m_pActorComponent->GetOwnerPosition() + moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
					m_pActorComponent->SetOwnerPosition( targetPos );
				} else {
					RequestStateChange( KungFuSheepState::Idle );
				}
			} else if ( leftStick.x < 0.0f ) {
				m_pActorComponent->SetTargetFacingDirection( g_LeftFacing );
			} else {
				RequestStateChange( KungFuSheepState::Idle );
			}
		}
	}

	virtual void EndState_Internal( T ) override { }
};

/**
 *	KungFuSheepStateAttack
 */
template<typename T>
class KungFuSheepStateAttack : public KungFuSheepStateBase<T> {

	bool m_bHitSnolaf = false;

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateAttack( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState_Internal( T ) override {

		m_StartTime = g_GlobalTimer.TimeElapsedSeconds();
		m_bQueueAttack = false;

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

		if ( kbfrand() > 0.8f ) {
			GetSheep()->PlayAttackVO( 0 );
		}

		m_bHitSnolaf = false;
	}

	virtual void UpdateState_Internal() override {

		static const kbString PunchL_Anim( "PunchLeft_Basic" );
		static const kbString KickL_Anim( "KickLeft_Basic" );
		static const kbString PunchR_Anim( "PunchRight_Basic" );
		static const kbString KickR_Anim( "KickRight_Basic" );

		if ( m_bHitSnolaf == false ) {
			KungFuLevelComponent *const pLevelComponent = KungFuLevelComponent::Get();
		
			DealAttackInfo_t<KungFuGame::eAttackType> dealAttackInfo;
			dealAttackInfo.m_BaseDamage = 999999.0f;
			dealAttackInfo.m_pAttacker = this->GetSheep();
			dealAttackInfo.m_Radius = 0.0f;
			dealAttackInfo.m_AttackType = KungFuGame::Punch_Kick;

			const AttackHitInfo_t attackInfo = pLevelComponent->DoAttack( dealAttackInfo );
			if ( attackInfo.m_bHit ) {
				GetSheep()->HitASnolaf();
				m_bHitSnolaf = true;
			}
		}

		if ( g_GlobalTimer.TimeElapsedSeconds() - m_StartTime > KungFuGame::kTimeUntilRequeueReady ) {
		
			const kbInput_t & input = g_pInputManager->GetInput();
			if ( WasAttackJustPressed() && KungFuSheepDirector::Get()->GetNumHuggersAndPrehuggers() == 0 ) {
				m_bQueueAttack = true;
			}
		}
		if ( m_pActorComponent->IsPlayingAnim( PunchL_Anim ) == false &&
			 m_pActorComponent->IsPlayingAnim( KickL_Anim ) == false &&
			 m_pActorComponent->IsPlayingAnim( PunchR_Anim ) == false && 
			 m_pActorComponent->IsPlayingAnim( KickR_Anim ) == false ) {
			
		//	kbLog( "Took %f sec", g_GlobalTimer.TimeElapsedSeconds() - m_StartTime );
			if ( m_bQueueAttack ) {
				BeginState( KungFuSheepState::Attack );
			} else {
				RequestStateChange( KungFuSheepState::Idle );
			}
		}
	}

	float m_StartTime;
	bool m_bQueueAttack;
};

/**
 *	KungFuSheepStateHugged
 */
template<typename T>
class KungFuSheepStateHugged : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateHugged( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState_Internal( T ) override {

				kbLog( "Entered Hugged ");

		m_NumDirectionChanges = 0;
		m_CurrentDirection = 0;
		m_ShakeNBakeActivationStartTime = -1;

		m_NumBaasPlayed = 0;
		m_NextBaaTime = g_GlobalTimer.TimeElapsedSeconds() + kbfrand() * 1.0f;

		static const kbString IdleL_Anim( "IdleLeft_Basic" );
		static const kbString IdleR_Anim( "IdleRight_Basic" );

		KungFuSheepComponent *const pSheep = m_pActorComponent->GetAs<KungFuSheepComponent>();
		pSheep->SetAnimationTimeScaleMultiplier( IdleL_Anim, 2.0f );
		pSheep->SetAnimationTimeScaleMultiplier( IdleR_Anim, 2.0f );

		if ( kbfrand() > 0.5f ) {
			GetSheep()->PlayBaa( -1 );
			m_NextBaaTime = g_GlobalTimer.TimeElapsedSeconds() + 1 + 3.0f * kbfrand();
			m_NumBaasPlayed = 1;
		}
	}

	virtual void UpdateState_Internal() override {
		const kbInput_t & input = g_pInputManager->GetInput();

		const float frameDT = g_pGame->GetFrameDT();
		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();

		// Baaa
		if ( m_NumBaasPlayed < 2 && g_GlobalTimer.TimeElapsedSeconds() > m_NextBaaTime ) {
			m_NumBaasPlayed++;
			GetSheep()->PlayBaa( -1 );
			m_NextBaaTime = g_GlobalTimer.TimeElapsedSeconds() + 3.0f + kbfrand();
		}

		const kbVec2 leftStick = GetLeftStick();
		if ( WasSpecialAttackPressed() && GetSheep()->GetCannonBallMeterFill() >= 1.0f ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( leftStick.x < 0.0f ) {
			if ( m_CurrentDirection == 1 ) {
				m_CurrentDirection = 0;
				m_NumDirectionChanges++;
				m_LastDirectionChangeTime = curTime;
			}
			m_pActorComponent->SetTargetFacingDirection( g_LeftFacing );
		} else if ( leftStick.x > 0.0f ) {

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

/*		int numHuggers = 0;
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pEnt =  g_pCannonGame->GetGameEntities()[i];
			if ( pEnt->GetActorComponent() == nullptr ) {
				continue;
			}

			KungFuSnolafComponent *const pSnolaf = g_pCannonGame->GetGameEntities()[i]->GetActorComponent()->GetAs<KungFuSnolafComponent>();
			if ( pSnolaf == nullptr || pSnolaf->IsEnabled() == false ) {
				continue;
			}

			if ( pSnolaf->GetState() == KungFuSnolafState::Hug || pSnolaf->GetState() == KungFuSnolafState::PreHug ) {
				numHuggers++;
			}
		}*/

		const int numSnolafHuggers = KungFuSheepDirector::Get()->GetNumHuggers();
		if ( g_CannonBallTest.GetBool() && g_UseEditor == false && numSnolafHuggers > 5 ) {
			RequestStateChange( KungFuSheepState::CannonBall );
			return;
		}

		if ( m_ShakeNBakeActivationStartTime < 0.0f && numSnolafHuggers > 0 && m_NumDirectionChanges > numSnolafHuggers ) {
			m_ShakeNBakeActivationStartTime = g_GlobalTimer.TimeElapsedSeconds();
			KungFuSheepComponent *const pSheep = m_pActorComponent->GetAs<KungFuSheepComponent>();
			pSheep->PlayShakeNBakeFX();
		}

		if ( m_ShakeNBakeActivationStartTime > 0.0f && g_GlobalTimer.TimeElapsedSeconds() > m_ShakeNBakeActivationStartTime + 0.15f ) {

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

	virtual void EndState_Internal( T nextState ) override {

		static const kbString IdleL_Anim( "IdleLeft_Basic" );
		static const kbString IdleR_Anim( "IdleRight_Basic" );

		KungFuSheepComponent *const pSheep = m_pActorComponent->GetAs<KungFuSheepComponent>();
		pSheep->SetAnimationTimeScaleMultiplier( IdleL_Anim, 1.0f );
		pSheep->SetAnimationTimeScaleMultiplier( IdleR_Anim, 1.0f );
	}

	const float m_DirectionChangeWindowSec = 0.5f;
	int m_NumDirectionChanges = 0;
	int m_CurrentDirection = 0;
	float m_LastDirectionChangeTime = 0.0f;
	float m_ShakeNBakeActivationStartTime = -1.0f;
	int m_NumBaasPlayed = 0;
	float m_NextBaaTime = 0.0f;
};

/**
 *	KungFuSheepStateDead
 */
template<typename T>
class KungFuSheepStateDead : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateDead( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState_Internal( T ) override {
		kbFlingPhysicsComponent *const pFlingPhysics = GetSheep()->GetComponent<kbFlingPhysicsComponent>();
		pFlingPhysics->Enable( true );
		m_bSplashDone = false;
	}

	virtual void UpdateState_Internal() override {

		if ( m_bSplashDone == false && GetSheep()->GetOwnerPosition().y < -60.0f ) {
			m_bSplashDone = true;
			GetSheep()->SpawnSplash();
		}
	};

	bool m_bSplashDone = false;
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

	virtual void BeginState_Internal( T prevState ) override {

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

	virtual void UpdateState_Internal() override {
		
		// Hack to avoid exiting Cannonball early.  Not sure if needed at this point
		if ( g_GlobalTimer.TimeElapsedSeconds() < m_StartCannonBallTime + 1.0f ) {
			return;
		}

		static const kbString CannonBall_Anim( "CannonBall" );
		if ( m_pActorComponent->HasFinishedAnim( CannonBall_Anim  ) ) {
			RequestStateChange( KungFuSheepState::Idle );
		}
	}

	virtual void EndState_Internal( T nextState ) override {
		m_pActorComponent->SetTargetFacingDirection( m_OldFacingDirection );
		GetSheep()->CannonBallActivatedCB();
	}

	float m_StartCannonBallTime = 0.0f;
};

/**
 *	KungFuSheepStateCinema
 */
template<typename T>
class KungFuSheepStateCinema : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateCinema( CannonActorComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState_Internal( T ) override {
	}

	virtual void UpdateState_Internal() override {
	}

	virtual void EndState_Internal( T nextState ) override {
	}

	const float m_DirectionChangeWindowSec = 0.5f;
	int m_NumDirectionChanges = 0;
	int m_CurrentDirection = 0;
	float m_LastDirectionChangeTime = 0.0f;
	float m_ShakeNBakeActivationStartTime = -1.0f;
	int m_NumBaasPlayed = 0;
	float m_NextBaaTime = 0.0f;
};

/**
 *	KungFuSheepComponent::Constructor
 */
void KungFuSheepComponent::Constructor() {
	m_TargetFacingDirection.Set( 0.0f, 0.0f, -1.0f );
	m_bIsPlayer = true;
	m_LastVOTime = 0.0f;

	m_CannonBallMeter = 0.0f;
}

/**
 *	KungFuSheepComponent::SetEnable_Internal
 */
void KungFuSheepComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {

		KungFuSheepStateBase<KungFuSheepState::SheepStates_t> * sheepStates[] = {
			new KungFuSheepStateIdle<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateRun<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateAttack<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateHugged<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateDead<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateCannonBall<KungFuSheepState::SheepStates_t>( this ),
			new KungFuSheepStateCinema<KungFuSheepState::SheepStates_t>( this )
		};

		InitializeStateMachine( sheepStates );
		RequestStateChange( KungFuSheepState::Idle );

		if ( GetOwner()->IsPrefab() == false ) {
			m_HeadBandInstance[0].SetEntity( g_pGame->CreateEntity( m_HeadBand.GetEntity() ) );
			m_HeadBandInstance[0].GetEntity()->SetPosition( GetOwnerPosition() );
			m_HeadBandInstance[0].GetEntity()->SetOrientation( GetOwnerRotation() );

			m_HeadBandInstance[1].SetEntity( g_pGame->CreateEntity( m_HeadBand.GetEntity() ) );
			m_HeadBandInstance[1].GetEntity()->SetPosition( GetOwnerPosition() );
			m_HeadBandInstance[1].GetEntity()->SetOrientation( GetOwnerRotation() );
		}

		m_Health = 1.0f;
		m_CannonBallMeter = 0.0f;
		if ( g_UseEditor == false ) {
			KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
			pLevelComp->UpdateSheepHealthBar( m_Health );
			pLevelComp->UpdateCannonBallMeter( m_CannonBallMeter, false );
		}
	} else {
		if ( m_HeadBandInstance[0].GetEntity() != nullptr ) {
			g_pGame->RemoveGameEntity( m_HeadBandInstance[0].GetEntity() );
			g_pGame->RemoveGameEntity( m_HeadBandInstance[1].GetEntity() );

			m_HeadBandInstance[0].SetEntity( nullptr );
			m_HeadBandInstance[1].SetEntity( nullptr );
		}

		ShutdownStateMachine();
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
		dealAttackInfo.m_Radius = 999999999.0f;
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

/*		KungFuLevelComponent *const pLevelComponent = KungFuLevelComponent::Get();
		
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

			m_CannonBallMeter += 0.25f;
			pLevelComponent->UpdateCannonBallMeter( m_CannonBallMeter, false );
		}

		if ( kbfrand() > 0.8f ) {
			PlayAttackVO( 0 );
		}
		*/
	}
}

 /**
  *	KungFuSheepComponent::Update_Internal
  */
void KungFuSheepComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	if ( DT > 0.0f ) {
		UpdateStateMachine();
	}

	{
		static int blinkState = 0;
		static float lastBlinkTime = g_GlobalTimer.TimeElapsedSeconds();
		float curTime = g_GlobalTimer.TimeElapsedSeconds();
		const float eyeOpenTime = ( m_CurrentState == KungFuSheepState::Hugged ) ? ( 1.0f ) : ( 2.0f );
		const float eyeClosedTime = ( m_CurrentState == KungFuSheepState::Hugged ) ? ( 0.1f ) : ( 0.5f );
		const static kbString fxMapMaskParam = "fxMapMask";
		kbVec4 vFXMaskMapParam( 1.0f, 0.0f, 0.0f, 0.0f );
		if ( blinkState == 0 ) {
			if ( curTime > lastBlinkTime + eyeOpenTime ) {
				blinkState = 1;
				lastBlinkTime = curTime;
			} else {

				if ( m_CurrentState == KungFuSheepState::Hugged ) {
					vFXMaskMapParam.Set( 0.0f, 0.0f, 1.0f, 0.0f );
				} else {
					vFXMaskMapParam.Set( 1.0f, 0.0f, 0.0f, 0.0f );
				}
			}
		} else {
			if ( curTime > lastBlinkTime + eyeClosedTime ) {
				blinkState = 0;
				lastBlinkTime = curTime;
			} else {
				vFXMaskMapParam.Set( 0.0f, 1.0f, 0.0f, 0.0f );
			}
		}

		m_SkelModelsList[0]->SetMaterialParamVector( 0, fxMapMaskParam.stl_str(), vFXMaskMapParam );
	}

	// Headband
	kbBoneMatrix_t boneMat;
	if ( m_SkelModelsList[0]->GetBoneWorldMatrix( kbString( "Head" ), boneMat ) ) {
		const kbVec3 axis1 = boneMat.GetAxis(0).Normalized();
		const kbVec3 axis2 = boneMat.GetAxis(1).Normalized();
		const kbVec3 axis3 = boneMat.GetAxis(2).Normalized();

		m_HeadBandInstance[0].GetEntity()->SetPosition( boneMat.GetOrigin() + axis1 * 0.1f - axis3 * 0.15f );
		m_HeadBandInstance[1].GetEntity()->SetPosition( boneMat.GetOrigin() + axis1 * 0.1f + axis2 * 0.01f - axis3 * 0.15f );
	}

	const kbVec3 curFacing = GetOwnerRotation().ToMat4()[2].ToVec3();
	const float t = ( curFacing.z * 0.5f ) + 0.5f;
	kbVec4 collisionSphere = kbLerp( kbVec4( 0.280000f, -0.080000f, 0.019997f, 0.25f ), kbVec4( 0.140f, -0.060f, 0.279997f, 0.25f ), t );
	kbClothComponent *const pCloth1 = m_HeadBandInstance[0].GetEntity()->GetComponent<kbClothComponent>();
	pCloth1->SetClothCollisionSphere( 0, collisionSphere );
	
	kbClothComponent *const pCloth2 = m_HeadBandInstance[1].GetEntity()->GetComponent<kbClothComponent>();
	pCloth2->SetClothCollisionSphere( 0, collisionSphere );

	// Do health check
	if ( g_CannonBallTest.GetBool() == false && m_Health > 0.0f && IsCannonBalling() == false ) {
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

			if ( pSnolaf->GetState() == KungFuSnolafState::Hug ) {
				numHuggers++;
			}
		}

		if ( numHuggers > 0 ) {
			const float healthDrain = DT * 0.05f * (float)numHuggers;
			m_Health = kbSaturate( m_Health - healthDrain );

			KungFuLevelComponent::Get()->UpdateSheepHealthBar( m_Health );

			if ( m_Health == 0.0f ) {
				RequestStateChange( KungFuSheepState::Dead );
			}
		}
	}

	if ( KungFuSheepDirector::Get()->GetNumHuggers() > 0 && IsCannonBalling() == false ) {
		RequestStateChange( KungFuSheepState::Hugged );
	}
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
	
	if ( attackInfo.m_AttackType == KungFuGame::DebugDeath ) {
		RequestStateChange( KungFuSheepState::Dead );
		m_Health = -1000.0f;
		return;
	}

	if ( IsCannonBalling() ) {
		return;
	}
}

/**
 *	KungFuSheepComponent::PlayShakeNBakeFX
 */
void KungFuSheepComponent::PlayShakeNBakeFX() {

	if ( m_ShakeNBakeFX.GetEntity() == nullptr ) {
		return;
	}

	kbGameEntity *const pShakeNBakeFX = g_pGame->CreateEntity( m_ShakeNBakeFX.GetEntity() );
	pShakeNBakeFX->SetPosition( GetOwnerPosition() );
	pShakeNBakeFX->SetOrientation( GetOwnerRotation() );
	pShakeNBakeFX->DeleteWhenComponentsAreInactive( true );
}

/**
 *	KungFuSheepComponent::PlayBaa
 */
void KungFuSheepComponent::PlayBaa( const int baaType ) {
	
	if ( m_BaaaVO.size() == 0 ) {
		return;
	}

	if ( g_GlobalTimer.TimeElapsedSeconds() < m_LastVOTime + 1.0f ) {
		return;
	}

	if ( baaType == -1 ) {
		m_BaaaVO[rand() % m_BaaaVO.size()].PlaySoundAtPosition( GetOwnerPosition() );
	} else {
		m_BaaaVO[baaType].PlaySoundAtPosition( GetOwnerPosition() );
	}
}

/**
 *	KungFuSheepComponent::HitASnolaf
 */	
void KungFuSheepComponent::HitASnolaf() {

	m_CannonBallMeter += 0.25f;
	KungFuLevelComponent::Get()->UpdateCannonBallMeter( m_CannonBallMeter, false );

	if ( m_BasicAttackImpactSound.size() > 0 ) {
		m_BasicAttackImpactSound[rand() % m_BasicAttackImpactSound.size()].PlaySoundAtPosition( GetOwnerPosition() );
	}
}

/**
 *	KungFuSheepComponent::SpawnSplash
 */	
void KungFuSheepComponent::SpawnSplash() {
	if ( m_SplashFX.GetEntity() == nullptr ) {
		return;
	}

	kbGameEntity *const pSplash = g_pGame->CreateEntity( m_SplashFX.GetEntity() );
	pSplash->SetPosition( GetOwnerPosition() );
	pSplash->DeleteWhenComponentsAreInactive( true );

	KungFuLevelComponent::Get()->DoSplashSound();

	if ( kbfrand() > 0.75f ) {
		KungFuLevelComponent::Get()->DoWaterDropletScreenFX();
	}
}

/**
 *	KungFuSheepComponent::CannonBallActivatedCB
 */	
void KungFuSheepComponent::CannonBallActivatedCB() {
	m_CannonBallMeter = 0.0f;
	
	KungFuLevelComponent::Get()->UpdateCannonBallMeter( 0.0f, true );
}

/**
 *	KungFuSheepComponent::ExternalRequestStateChange
 */	
void KungFuSheepComponent::ExternalRequestStateChange( const KungFuSheepState::SheepStates_t requestedState ) {

	RequestStateChange( requestedState);
}
