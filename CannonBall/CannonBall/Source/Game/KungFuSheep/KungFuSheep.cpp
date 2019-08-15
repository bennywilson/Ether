//===================================================================================================
// KungFuSheep.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "KungFuSheep.h"
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
	KungFuSheepStateIdle( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString IdleL_Anim( "IdleLeft_Basic" );
		static const kbString IdleR_Anim( "IdleRight_Basic" );

		const kbVec3 & targetDir = m_pPlayerComponent->GetTargetFacingDirection();
		if ( targetDir.z < 0.0f ) {
			PlayAnimation( IdleL_Anim, 0.05f );
		} else {
			PlayAnimation( IdleR_Anim, 0.05f );
		}
	}

	virtual void UpdateState() override {

		g_pRenderer->DrawDebugText( "Idle", 0.85f, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );

		const kbInput_t & input = g_pInputManager->GetInput();

		if ( input.IsKeyPressedOrDown( kbInput_t::KB_SPACE ) ) {
			RequestStateChange( KungFuSheepState::Attack );
		} else if ( input.IsKeyPressedOrDown( 'C' ) ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( input.IsKeyPressedOrDown( 'A' ) ) {
			RequestStateChange( KungFuSheepState::Run );
			m_pPlayerComponent->SetTargetFacingDirection( g_LeftFacing );
		} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
			RequestStateChange( KungFuSheepState::Run );
			m_pPlayerComponent->SetTargetFacingDirection( g_RightFacing );
		}
	}

	virtual void EndState( T ) override { }
};

/**
 *	KungFuSheepStateRun
 */
template<typename T>
class KungFuSheepStateRun : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateRun( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString Run_Anim( "Run_Basic" );
		PlayAnimation( Run_Anim, 0.05f, false );
	}

	virtual void UpdateState() override {

		const kbInput_t & input = g_pInputManager->GetInput();

		const float frameDT = g_pGame->GetFrameDT();
		const kbVec3 & targetDir = m_pPlayerComponent->GetTargetFacingDirection();

		if ( input.IsKeyPressedOrDown( 'C' ) ) {
			RequestStateChange( KungFuSheepState::CannonBall );
		} else if ( targetDir.z < 0.0f ) {
			if ( input.IsKeyPressedOrDown( 'A' ) ) {
				
				// Run
				const kbVec3 targetPos = m_pPlayerComponent->GetOwnerPosition() - targetDir * frameDT * m_pPlayerComponent->GetMaxRunSpeed();
				m_pPlayerComponent->SetOwnerPosition( targetPos );

			} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
				m_pPlayerComponent->SetTargetFacingDirection( g_RightFacing );
			} else {
				RequestStateChange( KungFuSheepState::Idle );
			}

		} else {
			if ( input.IsKeyPressedOrDown( 'D' ) ) {

				// Run
				const kbVec3 targetPos = m_pPlayerComponent->GetOwnerPosition() - targetDir * frameDT * m_pPlayerComponent->GetMaxRunSpeed();
				m_pPlayerComponent->SetOwnerPosition( targetPos );

			} else if ( input.IsKeyPressedOrDown( 'A' ) ) {
				m_pPlayerComponent->SetTargetFacingDirection( g_LeftFacing );
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
	KungFuSheepStateAttack( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString PunchL_Anim( "PunchLeft_Basic" );
		static const kbString KickL_Anim( "KickLeft_Basic" );
		static const kbString PunchR_Anim( "PunchRight_Basic" );
		static const kbString KickR_Anim( "KickRight_Basic" );

		const kbVec3 & targetDir = m_pPlayerComponent->GetTargetFacingDirection();
		if ( targetDir.z < 0.0f ) {
			if ( rand() % 2 == 0 ) {
				PlayAnimation( PunchL_Anim, 0.05f );
			} else {
				PlayAnimation( KickL_Anim, 0.05f );
			}
		} else {
			if ( rand() % 2 == 0 ) {
				PlayAnimation( PunchR_Anim, 0.05f );
			} else {
				PlayAnimation( KickR_Anim, 0.05f );
			}
		}
	}

	virtual void UpdateState() override {

		g_pRenderer->DrawDebugText( "Attack", 0.85f, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );

		static const kbString PunchL_Anim( "PunchLeft_Basic" );
		static const kbString KickL_Anim( "KickLeft_Basic" );
		static const kbString PunchR_Anim( "PunchRight_Basic" );
		static const kbString KickR_Anim( "KickRight_Basic" );

		if ( m_pPlayerComponent->IsPlayingAnim( PunchL_Anim ) == false &&
			 m_pPlayerComponent->IsPlayingAnim( KickL_Anim ) == false &&
			 m_pPlayerComponent->IsPlayingAnim( PunchR_Anim ) == false && 
			 m_pPlayerComponent->IsPlayingAnim( KickR_Anim ) == false ) {
			
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
	KungFuSheepStateHugged( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSheepStateDead
 */
template<typename T>
class KungFuSheepStateDead : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateDead( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSheepStateCannonBall
 */
template<typename T>
class KungFuSheepStateCannonBall : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSheepStateCannonBall( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
	
	kbVec3 m_OldFacingDirection;

	virtual void BeginState( T ) override {
		static const kbString CannonBall_Anim( "CannonBall" );
		static const kbString CannonBallWindUp_Anim( "CannonBallWindUp" );

		PlayAnimation( CannonBallWindUp_Anim, 0.05f, false, CannonBall_Anim, 0.01f );

		m_OldFacingDirection = m_pPlayerComponent->GetTargetFacingDirection();
		m_pPlayerComponent->SetTargetFacingDirection( kbVec3( -1.0f, 0.0f, 0.0f ) );
	}

	virtual void UpdateState() override {
			
		g_pRenderer->DrawDebugText( "Cannon Ball", 0.85f, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );

		if ( m_pPlayerComponent->HasFinishedAnim() ) {
			RequestStateChange( KungFuSheepState::Idle );
		}
	}

	virtual void EndState( T ) override {
		m_pPlayerComponent->SetTargetFacingDirection( m_OldFacingDirection );
	}
};

/**
 *	KungFuSheepComponent::Constructor
 */
void KungFuSheepComponent::Constructor() {
	m_TargetFacingDirection.Set( 0.0f, 0.0f, -1.0f );
	m_AnimSmearStartTime = 0.0f;
}

/**
 *	KungFuSheepComponent::IsPlayingAnim
 */
bool KungFuSheepComponent::IsPlayingAnim( const kbString animName ) const {
	if ( m_SkelModelsList.size() == 0 ) {
		return false;
	}

	return m_SkelModelsList[0]->IsPlaying( animName );
}

/**
 *	KungFuSheepComponent::SetEnable_Internal
 */
void KungFuSheepComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	// Make sure sheep package is loaded
	g_ResourceManager.GetPackage( "./assets/Packages/Sheep.kbPkg" );

	if ( bEnable ) {
		if ( m_SkelModelsList.size() > 1 ) {
			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), kbVec4::zero );
		}

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
 *	KungFuSheepComponent::PlayAnimation
 */
void KungFuSheepComponent::PlayAnimation( const kbString animName, const float animBlendInLen, const bool bRestartIfAlreadyPlaying, const kbString nextAnimName, const float nextAnimBlendInLen ) {
	for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
		kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
		pSkelModel->PlayAnimation( animName, animBlendInLen, bRestartIfAlreadyPlaying, nextAnimName, nextAnimBlendInLen );
	}
}

/**
 *	KungFuSheepComponent::HasFinishedAnim
 */
bool KungFuSheepComponent::HasFinishedAnim() const {
	if ( m_SkelModelsList.size() == 0 ) {
		kbWarning( "KungFuSheepComponent::HasFinishedAnim() - Called with empty m_SkelModels list" );
		return true;
	}

	return m_SkelModelsList[0]->HasFinishedAnimation();
}

/**
*	KungFuSheepComponent::OnAnimEvent
*/
void KungFuSheepComponent::OnAnimEvent( const kbAnimEvent & animEvent ) {

	const static kbString CannonBallImpact = "CannonBall_Impact";
	const static kbString CannonBallVO = "CannonBall_VO";
	const static kbString CannonBallJumpSmear = "CannonBall_JumpSmear";
	const static kbString CannonBallDropSmear = "CannonBall_DropSmear";

	const kbString & animEventName = animEvent.GetEventName();
	const float animEventVal = animEvent.GetEventValue();

	kbLog( "Anim event fired %s", animEvent.GetEventName().c_str() );

	if ( animEventName == CannonBallImpact ) {
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
	} else if (animEventName == CannonBallDropSmear) {
		m_AnimSmearStartTime = g_GlobalTimer.TimeElapsedSeconds();
		m_AnimSmearVec.Set( 0.0f, m_DropSmearMagnitude, 0.0f, 0.0f);
		m_AnimSmearDuration = animEventVal;
	}
}

 /**
  *	KungFuSheepComponent::Update_Internal
  */
void KungFuSheepComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	UpdateStateMachine();
	

	const kbQuat curRot = GetOwnerRotation();

	kbMat4 facingMat;
	facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + m_TargetFacingDirection, kbVec3::up );

	const kbQuat targetRot = kbQuatFromMatrix( facingMat );
	GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) ); 
	
	// Anim Smear
	if (m_AnimSmearStartTime > 0.0f) {
		const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_AnimSmearStartTime;
		if (elapsedTime > m_AnimSmearDuration) {
			m_AnimSmearStartTime = -1.0f;
			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector(0, smearParam.stl_str(), kbVec4::zero);
		}
		else {
			const float strength = 1.0f - kbClamp(elapsedTime / m_AnimSmearDuration, 0.0f, 1.0f);
			const static kbString smearParam = "smearParams";
			kbVec4 smearVec = m_AnimSmearVec;
			smearVec.x *= strength;
			smearVec.y *= strength;
			smearVec.z *= strength;

			m_SkelModelsList[1]->SetMaterialParamVector(0, smearParam.stl_str(), smearVec);
		}
	}
}

 /**
  *	KungFuSheepComponent::HandleInput
  *//*
 void KungFuSheepComponent::HandleInput( const kbInput_t & input, const float DT ) {
	// this->UpdateStateMachine();

/*	static kbVec3 lastMove( 0.0f, 0.0f, -1.0f );
	static const kbString Run_Anim( "Run_Basic" );
	static const kbString IdleL_Anim( "IdleLeft_Basic" );
	static const kbString IdleR_Anim( "IdleRight_Basic" );
	static const kbString PunchL_Anim( "PunchLeft_Basic" );
	static const kbString KickL_Anim( "KickLeft_Basic" );
	static const kbString PunchR_Anim( "PunchRight_Basic" );
	static const kbString KickR_Anim( "KickRight_Basic" );
	static const kbString CannonBall_Anim( "CannonBall" );
	static const kbString CannonBallWindUp_Anim( "CannonBallWindUp" );

	static bool bIsPunching = false;
	static bool bIsCannonBalling = false;

	// Anim Smear
	if (m_AnimSmearStartTime > 0.0f) {
		const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_AnimSmearStartTime;
		if (elapsedTime > m_AnimSmearDuration) {
			m_AnimSmearStartTime = -1.0f;
			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector(0, smearParam.stl_str(), kbVec4::zero);
		}
		else {
			const float strength = 1.0f - kbClamp(elapsedTime / m_AnimSmearDuration, 0.0f, 1.0f);
			const static kbString smearParam = "smearParams";
			kbVec4 smearVec = m_AnimSmearVec;
			smearVec.x *= strength;
			smearVec.y *= strength;
			smearVec.z *= strength;

			m_SkelModelsList[1]->SetMaterialParamVector(0, smearParam.stl_str(), smearVec);
		}
	}

	if ( bIsCannonBalling ) {
		kbMat4 facingMat;
		facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -1.0f, 0.0f, 0.0f ), kbVec3::up );		

		const kbQuat curRot = GetOwnerRotation();
		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

//		if ( m_SkelModelsList[0]->IsPlaying( CannonBall_Anim ) == true || ( m_SkelModelsList[0]->IsPlaying( CannonBallWindUp_Anim ) == true && !m_SkelModelsList[0]->HasFinishedAnimation() ) ) {
		if ( m_SkelModelsList[0]->HasFinishedAnimation() == false ) {
			return;
		}

		bIsCannonBalling = false;
	}

	if ( input.WasKeyJustPressed( 'C' ) ) {
		bIsCannonBalling = true;
		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			kbString returnIdle = IdleL_Anim;
			if ( lastMove.z > 0 ) {
				returnIdle = IdleR_Anim;
			}
			pSkelModel->PlayAnimation( CannonBallWindUp_Anim, 0.05f, false, CannonBall_Anim, 0.01f );
		}
		return;
	}

	if ( bIsPunching ) {
		if ( m_SkelModelsList[0]->IsPlaying( PunchL_Anim ) == true || 
			 m_SkelModelsList[0]->IsPlaying( KickL_Anim ) == true ||
			 m_SkelModelsList[0]->IsPlaying( PunchR_Anim ) == true || 
			 m_SkelModelsList[0]->IsPlaying( KickR_Anim ) == true ) {
			return;
		}
		bIsPunching = false;
	}

	if ( input.WasKeyJustPressed( kbInput_t::KB_SPACE ) ) {
		bIsPunching = true;
		kbString directionToAttackMap[][3] = {  { PunchL_Anim, KickL_Anim, IdleL_Anim },
												{ PunchR_Anim, KickR_Anim, IdleR_Anim } };

		const int dirIndex = ( lastMove.z < 0 ) ? ( 0 ) : ( 1 );

		const kbString AttackAnim = directionToAttackMap[dirIndex][rand() %2];
		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( AttackAnim, 0.08f, false, directionToAttackMap[dirIndex][2], 0.15f );
		}
		return;
	}

	kbVec3 moveVec( kbVec3::zero );
	if ( input.IsKeyPressedOrDown( 'A' ) ) {
		moveVec = kbVec3( 0.0f, 0.0f, -1.0f ).Normalized();
	} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
		moveVec = kbVec3( 0.0f, 0.0f, 1.0f ).Normalized();
	}

	if ( moveVec.Compare( kbVec3::zero ) == false ) {

		const kbVec3 targetPos = GetOwnerPosition() - moveVec * DT * m_MaxRunSpeed;
		GetOwner()->SetPosition( targetPos );
		lastMove = moveVec;

		const kbVec3 facingVec = moveVec + kbVec3( -0.1f, 0.0f, 0.0f );	// Nudge a little so slerp below rotates player towards the camera
		const kbQuat curRot = GetOwnerRotation();

		kbMat4 facingMat;
		facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + facingVec, kbVec3::up );

		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( Run_Anim, 0.08f, false );
		}
	} else {

		const kbQuat curRot = GetOwnerRotation();

		kbMat4 facingMat;
		kbString idleAnimToPlay;
		if ( lastMove.z > 0 ) {
			facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -0.1f, 0.0f, 1.0f ), kbVec3::up );
			idleAnimToPlay = IdleR_Anim;
		} else {
			facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -0.1f, 0.0f, -1.0f ), kbVec3::up );
			idleAnimToPlay =IdleL_Anim;
		}
		
		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( idleAnimToPlay, 0.08f, false );
		}
	}

	//		m_AnimSmearStartTime = g_GlobalTimer.TimeElapsedSeconds();
	//	m_MaxAnimSmearStrength = 1.0f;
	//	m_AnimSmearDuration = animEventVal;
 }*/