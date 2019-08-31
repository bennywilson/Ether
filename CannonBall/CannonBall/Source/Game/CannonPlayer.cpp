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
 *	CannonActorComponent::Constructor
 */
 void CannonActorComponent::Constructor() {
	m_MaxRunSpeed = 3.0f;
	m_MaxRotateSpeed = 15.0f;
	m_Health = 100.0f;

	m_TargetFacingDirection.Set( 0.0f, 0.0f, -1.0f );

	m_AnimSmearDuration = 0.1f;
	m_AnimSmearVec.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	m_AnimSmearStartTime = -1.0f;

	m_bIsPlayer = false;
}

/**
 *	CannonActorComponent::SetEnable_Internal
 */
void CannonActorComponent::SetEnable_Internal( const bool bEnable ) {
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

			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), kbVec4::zero );
		}
	} else {
		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			m_SkelModelsList[i]->UnregisterAnimEventListener( this );
		}

		m_SkelModelsList.clear();
	}
}

/**
 *	CannonActorComponent::Update_Internal
 */
void CannonActorComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	const kbQuat curRot = GetOwnerRotation();

	kbMat4 facingMat;
	facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + m_TargetFacingDirection, kbVec3::up );

	const kbQuat targetRot = kbQuatFromMatrix( facingMat );
	GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

	
	// Anim Smear
	if ( m_AnimSmearStartTime > 0.0f ) {
		const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_AnimSmearStartTime;
		if ( elapsedTime > m_AnimSmearDuration ) {
			m_AnimSmearStartTime = -1.0f;
			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector(0, smearParam.stl_str(), kbVec4::zero );
		}
		else {
			const float strength = 1.0f - kbClamp( elapsedTime / m_AnimSmearDuration, 0.0f, 1.0f );
			const static kbString smearParam = "smearParams";
			kbVec4 smearVec = m_AnimSmearVec;
			smearVec.x *= strength;
			smearVec.y *= strength;
			smearVec.z *= strength;

			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), smearVec );
		}
	}
}

/**
 *	CannonActorComponent::PlayAnimation
 */
void CannonActorComponent::PlayAnimation( const kbString animName, const float animBlendInLen, const bool bRestartIfAlreadyPlaying, const kbString nextAnimName, const float nextAnimBlendInLen ) {
	for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
		kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
		pSkelModel->PlayAnimation( animName, animBlendInLen, bRestartIfAlreadyPlaying, nextAnimName, nextAnimBlendInLen );
	}
}

/**
 *	CannonActorComponent::HasFinishedAnim
 */
bool CannonActorComponent::HasFinishedAnim() const {
	if ( m_SkelModelsList.size() == 0 ) {
		kbWarning( "KungFuSheepComponent::HasFinishedAnim() - Called with empty m_SkelModels list" );
		return true;
	}

	return m_SkelModelsList[0]->HasFinishedAnimation();
}

/**
 *	CannonActorComponent::ApplyAnimSmear
 */
void CannonActorComponent::ApplyAnimSmear( const kbVec3 smearVec, const float durationSec ) {
	m_AnimSmearStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_AnimSmearVec = smearVec;
	m_AnimSmearDuration = durationSec;
}

/**
 *	CannonActorComponent::IsPlayingAnim
 */
bool CannonActorComponent::IsPlayingAnim( const kbString animName ) const {
	if ( m_SkelModelsList.size() == 0 ) {
		return false;
	}

	return m_SkelModelsList[0]->IsPlaying( animName );
}

/**
 *	CannonCameraComponent::Constructor
 */
void CannonCameraComponent::Constructor() {

	// Editor
	m_NearPlane = 1.0f;
	m_FarPlane = 20000.0f;		// TODO - NEAR/FAR PLANE - Tie into renderer properly
	m_PositionOffset.Set( 0.0f, 0.0f, 0.0f );
	m_LookAtOffset.Set( 0.0f, 0.0f, 0.0f );

	m_MoveMode = MoveMode_Follow;
	m_pTarget = nullptr;

	// Game
	m_CameraShakeStartTime = -1.0f;
	m_CameraShakeStartingOffset.Set( 0.0f, 0.0f );
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
			const kbComponent *const pComp = pEnt->GetComponentByType( CannonActorComponent::GetType() );
			if ( pComp != nullptr ) {
				m_pTarget = (kbGameEntity*)pComp->GetOwner();
				break;
			}
		}
	} else {
		const std::vector<kbGameEntity*> & GameEnts = g_pGame->GetGameEntities();
		for ( int i = 0; i < (int) GameEnts.size(); i++ ) {
			const kbGameEntity *const pEnt = GameEnts[i];
			const kbComponent *const pComp = pEnt->GetComponentByType( CannonActorComponent::GetType() );
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
			cameraDestRot.LookAt( GetOwner()->GetPosition(), m_pTarget->GetPosition() + m_LookAtOffset, kbVec3::up );
			cameraDestRot.InvertFast();
			GetOwner()->SetOrientation( kbQuatFromMatrix( cameraDestRot ) );

			const kbVec3 cameraDestPos = m_pTarget->GetPosition() + m_PositionOffset;
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
	m_AmplitudeY = 0.019f;

	m_FrequencyX = 15.0f;
	m_FrequencyY = 10.0f;
}
