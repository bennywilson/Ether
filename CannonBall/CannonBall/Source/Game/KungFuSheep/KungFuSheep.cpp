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
template<typename T>
class KungFuSheepStateIdle : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------'
public:
	KungFuSheepStateIdle( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString IdleR_Anim( "IdleRight_Basic" );

		PlayAnimation( IdleR_Anim );
	}

	virtual void UpdateState() {

	}

	virtual void EndState( T ) {
	
	}
};

/**
 *	KungFuSheepStateRun
 */
template<typename T>
class KungFuSheepStateRun : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------'
public:
	KungFuSheepStateRun( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }

};

/**
 *	KungFuSheepStateAttack
 */
template<typename T>
class KungFuSheepStateAttack : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------'
public:
	KungFuSheepStateAttack( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSheepStateHugged
 */
template<typename T>
class KungFuSheepStateHugged : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------'
public:
	KungFuSheepStateHugged( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSheepStateDead
 */
template<typename T>
class KungFuSheepStateDead : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------'
public:
	KungFuSheepStateDead( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSheepStateCannonBall
 */
template<typename T>
class KungFuSheepStateCannonBall : public KungFuSheepStateBase<T> {

//---------------------------------------------------------------------------------------------------'
public:
	KungFuSheepStateCannonBall( KungFuSheepComponent *const pPlayerComponent ) : KungFuSheepStateBase( pPlayerComponent ) { }
};


/**
 *	KungFuSheepComponent::Constructor
 */
void KungFuSheepComponent::Constructor() {
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
	}
}

/**
*	KungFuSheepComponent::OnAnimEvent
*/
void KungFuSheepComponent::PlayAnimation( const kbString animationName ) {
	for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
		kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
		pSkelModel->PlayAnimation( animationName, 0.05f, false, kbString::EmptyString, 0.01f );
	}
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