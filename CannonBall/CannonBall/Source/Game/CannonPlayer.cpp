//===================================================================================================
// CannonPlayer.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

/**
 *	CannonPlayerComponent::Constructor
 */
 void CannonPlayerComponent::Constructor() {
	m_MaxRunSpeed = 3.0f;
	m_MaxRotateSpeed = 15.0f;

	m_AnimSmearStartTime = -1.0f;
	m_AnimSmearVec.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	m_AnimSmearDuration = 0.0f;
}

/**
*	CannonPlayerComponent::OnAnimEvent
*/
void CannonPlayerComponent::OnAnimEvent( const kbAnimEvent & animEvent ) {

	const static kbString CannonBallImpact = "CannonBall_Impact";
	const static kbString CannonBallVO = "CannonBall_VO";
	const static kbString CannonBallJumpSmear = "CannonBall_JumpSmear";

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
		m_AnimSmearVec.Set( 0.0f, 1.0f, 0.0f, 0.0f );
		m_AnimSmearDuration = animEventVal;
	}
}

 /**
  *	CannonPlayerComponent::HandleInput
  */
 void CannonPlayerComponent::HandleInput( const kbInput_t & input, const float DT ) {

	static kbVec3 lastMove( 0.0f, 0.0f, -1.0f );
	static const kbString Run_Anim( "Run_Basic" );
	static const kbString IdleL_Anim( "IdleLeft_Basic" );
	static const kbString IdleR_Anim( "IdleRight_Basic" );
	static const kbString PunchL_Anim( "PunchLeft_Basic" );
	static const kbString KickL_Anim( "KickLeft_Basic" );
	static const kbString PunchR_Anim( "PunchRight_Basic" );
	static const kbString KickR_Anim( "KickRight_Basic" );
	static const kbString CannonBall_Anim( "CannonBall" );

	static bool bIsPunching = false;
	static bool bIsCannonBalling = false;

	if ( bIsCannonBalling ) {
		kbMat4 facingMat;
		facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -1.0f, 0.0f, 0.0f ), kbVec3::up );		

		const kbQuat curRot = GetOwnerRotation();
		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

		if ( m_SkelModelsList[0]->IsPlaying( CannonBall_Anim ) == true ) {
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
			pSkelModel->PlayAnimation( CannonBall_Anim, 0.08f, false, returnIdle, 0.01f );
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

	// Anim Smear
	if ( m_AnimSmearStartTime > 0.0f ) {
		const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_AnimSmearStartTime;
		if ( elapsedTime > m_AnimSmearDuration ) {
			m_AnimSmearStartTime = -1.0f;
				const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), kbVec4::zero );
		} else {
			const float strength = 1.0f - kbClamp( elapsedTime / m_AnimSmearDuration, 0.0f, 1.0f );
			const static kbString smearParam = "smearParams";
			kbVec4 smearVec = m_AnimSmearVec;
			smearVec.x *= strength;
			smearVec.y *= strength;
			smearVec.z *= strength;

			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), smearVec );
		}
	}
	//		m_AnimSmearStartTime = g_GlobalTimer.TimeElapsedSeconds();
	//	m_MaxAnimSmearStrength = 1.0f;
	//	m_AnimSmearDuration = animEventVal;
 }

/**
 *	CannonPlayerComponent::SetEnable_Internal
 */
void CannonPlayerComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		m_SkelModelsList.clear();
		const int NumComponents = (int)GetOwner()->NumComponents();
		for ( int i = 0; i < NumComponents; i++ ) {
			kbComponent *const pComponent = GetOwner()->GetComponent(i);
			if ( pComponent->IsA( kbSkeletalModelComponent::GetType() ) == false ) {
				continue;
			}
			m_SkelModelsList.push_back( (kbSkeletalModelComponent*)pComponent );
		}

		if ( m_SkelModelsList.size() > 0 ) {
			for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
				m_SkelModelsList[i]->RegisterAnimEventListener( this );
			}

			if ( m_SkelModelsList.size() > 1 ) {
				const static kbString smearParam = "smearParams";
				m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), kbVec4::zero );
			}
		}

		g_ResourceManager.GetPackage( "./assets/Packages/Sheep.kbPkg" );
	} else {
		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			m_SkelModelsList[i]->UnregisterAnimEventListener( this );
		}

		m_SkelModelsList.clear();
	}
}

/**
 *	CannonPlayerComponent::Update_Internal
 */
void CannonPlayerComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );
}

/**
 *	CannonCameraComponent::Constructor
 */
void CannonCameraComponent::Constructor() {

	// Editor
	m_MoveMode = MoveMode_Follow;
	m_Offset.Set( 0.0f, 0.0f, 0.0f );
	m_pTarget = nullptr;

	// Game
	m_NearPlane = 1.0f;
	m_FarPlane = 20000.0f;

	m_CameraShakeStartTime = -1.0f;
	m_CameraShakeDuration = 0.0f;
	m_CameraShakeAmplitude.Set( 0.0f, 0.0f );
	m_CameraShakeFrequency.Set( 0.0f, 0.0f );
}

/**
 *	CannonCameraComponent::SetEnable_Internal
 */
void CannonCameraComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	m_pTarget = nullptr;
	g_pRenderer->SetNearFarPlane( nullptr, m_NearPlane, m_FarPlane );
	if ( bEnable ) {
		switch( m_MoveMode ) {
			case MoveMode_None : {
			}
			break;

			case MoveMode_Follow : {
			}
			break;
		}
	}
}

/**
 *	CannonCameraComponent::FindTarget
 */
void CannonCameraComponent::FindTarget() {

	if ( g_UseEditor ) {
		extern kbEditor * g_Editor;
		std::vector<kbEditorEntity *> &	gameEnts = g_Editor->GetGameEntities();
		for ( int i = 0; i < gameEnts.size(); i++ ) {
			const kbGameEntity *const pEnt = gameEnts[i]->GetGameEntity();
			const kbComponent *const pComp = pEnt->GetComponentByType( CannonPlayerComponent::GetType() );
			if ( pComp != nullptr ) {
				m_pTarget = (kbGameEntity*)pComp->GetOwner();
				break;
			}
		}
	} else {
		const std::vector<kbGameEntity*> & GameEnts = g_pGame->GetGameEntities();
		for ( int i = 0; i < (int) GameEnts.size(); i++ ) {
			const kbGameEntity *const pEnt = GameEnts[i];
			const kbComponent *const pComp = pEnt->GetComponentByType( CannonPlayerComponent::GetType() );
			if ( pComp != nullptr ) {
				m_pTarget = (kbGameEntity*)pComp->GetOwner();
				break;
			}
		}
	}
}

/**
 *	CannonCameraComponent::StartCameraShake
 */
void CannonCameraComponent::StartCameraShake( const CannonCameraShakeComponent *const pCameraShakeComponent ) {

	m_CameraShakeStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_CameraShakeStartingOffset = kbVec2Rand( -m_CameraShakeAmplitude, m_CameraShakeAmplitude );
	m_CameraShakeDuration = pCameraShakeComponent->GetDuration();
	m_CameraShakeAmplitude = pCameraShakeComponent->GetAmplitude();
	m_CameraShakeFrequency = pCameraShakeComponent->GetFrequency();
}

/**
 *	CannonCameraComponent::Update_Internal
 */
void CannonCameraComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	kbVec2 camShakeOffset( 0.0f, 0.0f );
	if ( m_CameraShakeStartTime > 0.0f ) {
		const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_CameraShakeStartTime;
		if ( elapsedTime > m_CameraShakeDuration ) {
			m_CameraShakeStartTime = -1.0f;
		} else {
			const float fallOff = 1.0f - kbClamp( ( elapsedTime / m_CameraShakeDuration ), 0.0f, 1.0f );
			camShakeOffset.x = sin( m_CameraShakeStartingOffset.x + ( g_GlobalTimer.TimeElapsedSeconds() * m_CameraShakeFrequency.x ) ) * m_CameraShakeAmplitude.x * fallOff;
			camShakeOffset.y = sin( m_CameraShakeStartingOffset.y + ( g_GlobalTimer.TimeElapsedSeconds() * m_CameraShakeFrequency.y ) ) * m_CameraShakeAmplitude.y * fallOff;
		}
	}

	switch( m_MoveMode ) {
		case MoveMode_None : {
				
		}
		break;

		case MoveMode_Follow : {
			if ( m_pTarget == nullptr ) {
				FindTarget();
				if ( m_pTarget == nullptr ) {
					break;
				}
			}

			kbMat4 cameraDestRot;
			cameraDestRot.LookAt( GetOwner()->GetPosition(), m_pTarget->GetPosition(), kbVec3::up );
			cameraDestRot.InvertFast();
			GetOwner()->SetOrientation( kbQuatFromMatrix( cameraDestRot ) );

			const kbVec3 cameraDestPos = m_pTarget->GetPosition() + m_Offset;
			GetOwner()->SetPosition( cameraDestPos + cameraDestRot[0].ToVec3() * camShakeOffset.x + cameraDestRot[1].ToVec3() * camShakeOffset.y );
		}
		break;
	}
}

/**
 *	CannonCameraShakeComponent::Constructor
 */
void CannonCameraShakeComponent::Constructor() {
	m_Duration = 1.0f;
	m_AmplitudeX = 0.025f;
	m_FrequencyX = 15.0f;

	m_AmplitudeY = 0.019f;
	m_FrequencyY = 10.0f;
}