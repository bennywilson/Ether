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

//#define CANNONBALL_STRESSTEST 1

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

		if ( prevState == KungFuSheepState::CannonBall ) {
			m_LastCannonBallTime = g_GlobalTimer.TimeElapsedSeconds();
		}
	}

	virtual void UpdateState() override {

		const kbInput_t & input = g_pInputManager->GetInput();

		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		const float frameDT = g_pGame->GetFrameDT();
		const float blockerCheckReach = frameDT * m_pActorComponent->GetMaxRunSpeed() * 20.0f;
		if ( input.WasKeyJustPressed( 'K' ) ) {
			RequestStateChange( KungFuSheepState::Attack );
		} else if ( input.IsKeyPressedOrDown( 'J' ) && curTime > m_LastCannonBallTime + 2.0f ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( input.IsKeyPressedOrDown( 'A' ) ) {
							
			// Run
			const kbVec3 moveDir( 0.0f, 0.0f, 1 );
			if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
				RequestStateChange( KungFuSheepState::Run );
			}
			m_pActorComponent->SetTargetFacingDirection( g_LeftFacing );

		} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
							
			// Run
			const kbVec3 moveDir( 0.0f, 0.0f, -1 );
			if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
				RequestStateChange( KungFuSheepState::Run );
			}
			m_pActorComponent->SetTargetFacingDirection( g_RightFacing );
		}

#ifdef CANNONBALL_STRESSTEST
		if ( g_UseEditor == false ) {
			if ( m_LastCannonBallTime == -1.0f || g_GlobalTimer.TimeElapsedSeconds() > m_LastCannonBallTime + 5.0f ) {
				RequestStateChange( KungFuSheepState::CannonBall );
			}
		}
#endif
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
		const float blockerCheckReach = frameDT * m_pActorComponent->GetMaxRunSpeed() * 20.0f;

		if ( input.WasKeyJustPressed( 'K' ) ) {
			RequestStateChange( KungFuSheepState::Attack );
		} else if ( input.IsKeyPressedOrDown( 'J' ) ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( targetDir.z < 0.0f ) {
			if ( input.IsKeyPressedOrDown( 'A' ) ) {
				
				// Run
				const kbVec3 moveDir( 0.0f, 0.0f, 1 );

				if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
					const kbVec3 targetPos = m_pActorComponent->GetOwnerPosition() + moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
					m_pActorComponent->SetOwnerPosition( targetPos );
				} else {
					RequestStateChange( KungFuSheepState::Idle );
				}
			} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
				m_pActorComponent->SetTargetFacingDirection( g_RightFacing );
			} else {
				RequestStateChange( KungFuSheepState::Idle );
			}

		} else {
			if ( input.IsKeyPressedOrDown( 'D' ) ) {

				// Run
				const kbVec3 moveDir( 0.0f, 0.0f, -1 );

				if ( CheckForBlocker( moveDir * blockerCheckReach ) == false ) {
					const kbVec3 targetPos = m_pActorComponent->GetOwnerPosition() + moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
					m_pActorComponent->SetOwnerPosition( targetPos );
				} else {
					RequestStateChange( KungFuSheepState::Idle );
				}
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
	}

	virtual void UpdateState() override {

		static const kbString PunchL_Anim( "PunchLeft_Basic" );
		static const kbString KickL_Anim( "KickLeft_Basic" );
		static const kbString PunchR_Anim( "PunchRight_Basic" );
		static const kbString KickR_Anim( "KickRight_Basic" );

		if ( g_GlobalTimer.TimeElapsedSeconds() - m_StartTime > 0.15f ) {
		
			const kbInput_t & input = g_pInputManager->GetInput();
			if ( input.WasKeyJustPressed( 'K' ) ) {
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

	virtual void BeginState( T ) override {
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
			GetSheep()->PlayBaa( 0 );
			m_NextBaaTime = g_GlobalTimer.TimeElapsedSeconds() + 1 + 3.0f * kbfrand();
			m_NumBaasPlayed = 1;
		}
	}

	virtual void UpdateState() override {
		const kbInput_t & input = g_pInputManager->GetInput();

		const float frameDT = g_pGame->GetFrameDT();
		const kbVec3 & targetDir = m_pActorComponent->GetTargetFacingDirection();
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();

		// Baaa
		if ( m_NumBaasPlayed < 2 && g_GlobalTimer.TimeElapsedSeconds() > m_NextBaaTime ) {
			m_NumBaasPlayed++;
			GetSheep()->PlayBaa( 0 );
			m_NextBaaTime = g_GlobalTimer.TimeElapsedSeconds() + 3.0f + kbfrand();
		}

		if ( input.IsKeyPressedOrDown( 'J' ) ) {
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

			if ( pSnolaf->GetState() == KungFuSnolafState::Hug ) {
				numHuggers++;
			}
		}
#ifdef CANNONBALL_STRESSTEST
		if ( g_UseEditor == false && numHuggers > 5 ) {
			RequestStateChange( KungFuSheepState::CannonBall );
			return;
		}
#endif

		if ( m_ShakeNBakeActivationStartTime < 0.0f && numHuggers > 0 && m_NumDirectionChanges > numHuggers ) {
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

	virtual void EndState( T nextState ) override {

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
	m_LastVOTime = 0.0f;
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
			new KungFuSheepStateCannonBall<KungFuSheepState::SheepStates_t>( this )
		};

		InitializeStates( sheepStates );
		RequestStateChange( KungFuSheepState::Idle );

		if ( GetOwner()->IsPrefab() == false ) {
			m_HeadBandInstance[0].SetEntity( g_pGame->CreateEntity( m_HeadBand.GetEntity() ) );
			m_HeadBandInstance[0].GetEntity()->SetPosition( GetOwnerPosition() );
			m_HeadBandInstance[0].GetEntity()->SetOrientation( GetOwnerRotation() );

			m_HeadBandInstance[1].SetEntity( g_pGame->CreateEntity( m_HeadBand.GetEntity() ) );
			m_HeadBandInstance[1].GetEntity()->SetPosition( GetOwnerPosition() );
			m_HeadBandInstance[1].GetEntity()->SetOrientation( GetOwnerRotation() );
		}
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

		if ( kbfrand() > 0.8f ) {
			PlayAttackVO( 0 );
		}
	}
}

 /**
  *	KungFuSheepComponent::Update_Internal
  */
void KungFuSheepComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	UpdateStateMachine();


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

	m_BaaaVO[rand() % m_BaaaVO.size()].PlaySoundAtPosition( GetOwnerPosition() );
}
