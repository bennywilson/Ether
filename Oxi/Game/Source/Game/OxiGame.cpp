//===================================================================================================
// OxiGame.cpp
//
//
// 2020 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbTypeInfo.h"
#include "kbIntersectionTests.h"
#include "kbLevelComponent.h"
#include "DX11/kbRenderer_DX11.h"
#include "OxiGame.h"
#include <directxpackedvector.h>
#include "kbEditorEntity.h"

OxiGame * g_pOxiGame = nullptr;

/**
 *	OxiGame::OxiGame
 */
OxiGame::OxiGame() :
	kbRenderHook( RP_FirstPerson ),
	m_pMainCamera( nullptr ) {

	m_Camera.m_Position.Set( 0.0f, 2600.0f, 0.0f );

	kbErrorCheck( g_pOxiGame == nullptr, "OxiGame::OxiGame() - g_pOxiGame is not nullptr" );
	g_pOxiGame = this;
}

/**
 *	OxiGame::~OxiGame
 */
OxiGame::~OxiGame() {

	kbErrorCheck( g_pOxiGame != nullptr, "OxiGame::~OxiGame() - g_pOxiGame is nullptr" );
	g_pOxiGame = nullptr;
}

/**
 *	OxiGame::PlayGame_Internal
 */
void OxiGame::PlayGame_Internal() {
	g_pRenderer->RegisterRenderHook( this );
}

/**
 *	OxiGame::InitGame_Internal
 */
void OxiGame::InitGame_Internal() {

	m_GameStartTimer.Reset();
}

/**
 *	OxiGame::StopGame_Internal
 */
void OxiGame::StopGame_Internal() {

	m_pLocalPlayer = nullptr;

	g_pRenderer->UnregisterRenderHook( this );
}

/**
 *	OxiGame::LevelLoaded_Internal
 */
void OxiGame::LevelLoaded_Internal() {

	m_pMainCamera = nullptr;

	int cameraIdx = -1;
	const std::vector<kbGameEntity*> & GameEnts = GetGameEntities();
	for ( int i = 0; i < GameEnts.size(); i++ ) {
		kbGameEntity *const pCurEnt = GameEnts[i];

		if ( m_pMainCamera == nullptr ) {
			m_pMainCamera = pCurEnt->GetComponent<OxiCameraComponent>();
			if ( m_pMainCamera != nullptr ) {
				cameraIdx = i;
			}
		}

		/*if ( m_pPlayerComp == nullptr ) {
			CannonActorComponent *const pActor = pCurEnt->GetComponent<CannonActorComponent>();
			if ( pActor != nullptr && pActor->IsPlayer() ) {
				m_pPlayerComp = pActor;
			}
		}*/
	}

	if ( cameraIdx >= 0 ) {
		SwapEntitiesByIdx( cameraIdx, GameEnts.size() - 1 );
	}

	kbWarningCheck( m_pMainCamera != nullptr, "OxiGame::LevelLoaded_Internal() - No camera found.");
}

/**
 *	OxiGame::PreUpdate_Internal
 */
void OxiGame::PreUpdate_Internal() {

	const float frameDT = GetFrameDT();
	if ( IsConsoleActive() == false ) {
		ProcessInput( frameDT );
	}
}

/**
 *	OxiGame::PostUpdate_Internal
 */
void OxiGame::PostUpdate_Internal() {

	// Update renderer cam
	if ( m_pMainCamera != nullptr && m_pMainCamera->GetOwner() != nullptr ) {
		g_pD3D11Renderer->SetRenderViewTransform( nullptr, m_pMainCamera->GetOwner()->GetPosition(), m_pMainCamera->GetOwner()->GetOrientation() );
	}
}

/**
 *	OxiGame::AddGameEntity_Internal
 */
void OxiGame::AddGameEntity_Internal( kbGameEntity *const pEntity ) {

	if ( pEntity == nullptr ) {
		kbWarning( "OxiGame::AddGameEntity_Internal() - nullptr Entity" );
		return;
	}
	/*
	if ( m_pLocalPlayer == nullptr || m_pPlayerComp == nullptr ) {
		CannonActorComponent *const pActor = pEntity->GetComponent<CannonActorComponent>();
		if ( pActor != nullptr && pActor->IsPlayer() ) {
			m_pLocalPlayer = pEntity;
			m_pPlayerComp = pActor;
		}
	}*/
}

/**
 *	OxiGame::RemoveGameEntity_Internal
 */
void OxiGame::RemoveGameEntity_Internal( kbGameEntity *const pEntity ) {

	if ( pEntity == nullptr ) {
		kbWarning( "OxiGame::RemoveGameEntity_Internal() - nullptr Entity" );
		return;
	}

	if ( pEntity == m_pLocalPlayer ) {
		m_pLocalPlayer = nullptr;
	}
}

/**
 *	OxiGame::CreatePlayer
 */
kbGameEntity * OxiGame::CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & DesiredLocation ) {

	return nullptr;
}

/**
 *	OxiGame::ProcessInput
 */
void OxiGame::ProcessInput( const float DT ) {

	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if ( bFirstRun ) {
		ShowCursor( false );
		bFirstRun = false;
	}
}

/**
 *	OxiGame::RenderSync
 */
void OxiGame::RenderSync() {

	
}

/**
 *	OxiGame::RenderHookCallBack
 */
static float g_TimeMultiplier = 0.95f / 0.016f;
void OxiGame::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {

	if ( pSrc != nullptr && pDst != nullptr ) {
		g_pRenderer->RT_CopyRenderTarget( pSrc, pDst );
	}
}

/**
 *	CannonLevelComponent::Constructor
 */
void OxiLevelComponent::Constructor() {
	m_Dummy2 = -1;
}

/**
 *	OxiCameraComponent::Constructor
 */
void OxiCameraComponent::Constructor() {

	// Editor
	m_NearPlane = 1.0f;
	m_FarPlane = 20000.0f;		// TODO - NEAR/FAR PLANE - Tie into renderer properly
	m_PositionOffset.Set( 0.0f, 0.0f, 0.0f );
	m_LookAtOffset.Set( 0.0f, 0.0f, 0.0f );

	m_MoveMode = MoveMode_Follow;
	m_pTarget = nullptr;

	// Game
	m_SwitchTargetBlendSpeed = 1.0f;
	m_SwitchTargetCurT = 1.0f;
	m_SwitchTargetStartPos.Set( 0.0f, 0.0f, 0.0f );

	m_SwitchPosOffsetBlendSpeed = 1.0f;
	m_SwitchPosOffsetCurT = 1.0f;
	m_PosOffsetTarget.Set( 0.0f, 0.0f, 0.0f );

	m_SwitchLookAtOffsetBlendSpeed = 1.0f;
	m_SwitchLookAtOffsetCurT = 1.0f;
	m_LookAtOffsetTarget.Set( 0.0f, 0.0f, 0.0f );

	m_CameraShakeStartTime = -1.0f;
	m_CameraShakeStartingOffset.Set( 0.0f, 0.0f );
	m_CameraShakeDuration = 0.0f;
	m_CameraShakeAmplitude.Set( 0.0f, 0.0f );
	m_CameraShakeFrequency.Set( 0.0f, 0.0f );
}

/**
 *	OxiCameraComponent::SetEnable_Internal
 */
void OxiCameraComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	m_pTarget = nullptr;
	g_pRenderer->SetNearFarPlane( nullptr, m_NearPlane, m_FarPlane );
}

/**
 *	OxiCameraComponent::SetTarget
 */
void OxiCameraComponent::SetTarget( const kbGameEntity* const pTarget, const float blendRate ) {
	m_SwitchTargetBlendSpeed = blendRate;

	if ( m_SwitchTargetBlendSpeed > 0 ) {
		if ( m_pTarget != nullptr ) {
			m_SwitchTargetStartPos = m_pTarget->GetPosition();
			m_SwitchTargetCurT = 0.0f;

		} else {
			m_SwitchTargetBlendSpeed = -1.0f;
		}
	}

	m_pTarget = pTarget;
}

/**
 *	OxiCameraComponent::SetPositionOffset
 */
void OxiCameraComponent::SetPositionOffset( const kbVec3& posOffset, const float blendRate ) {
	
	if ( blendRate < 0.0f ) {
		m_SwitchPosOffsetCurT = 1.0f;
		m_PositionOffset = posOffset;
	} else {
		m_SwitchPosOffsetCurT = 0.0f;
		m_SwitchPosOffsetBlendSpeed = blendRate;
		m_PosOffsetTarget = posOffset;
	}
}

/**
 *	OxiCameraComponent::SetLookAtOffset
 */
void OxiCameraComponent::SetLookAtOffset( const kbVec3& lookAtOffset, const float blendRate ) {

	if ( blendRate < 0.0f ) {
		m_SwitchLookAtOffsetCurT = 1.0f;
		m_LookAtOffset = lookAtOffset;
	} else {
		m_SwitchLookAtOffsetBlendSpeed = blendRate;
		m_SwitchLookAtOffsetCurT = 0.0f;
		m_LookAtOffsetTarget = lookAtOffset;
	}
}

/**
 *	OxiCameraComponent::StartCameraShake
 */
void OxiCameraComponent::StartCameraShake( const OxiCameraShakeComponent* const pCameraShakeComponent ) {

	m_CameraShakeStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_CameraShakeStartingOffset = kbVec2Rand( -m_CameraShakeAmplitude, m_CameraShakeAmplitude );
	m_CameraShakeDuration = pCameraShakeComponent->GetDuration();
	m_CameraShakeAmplitude = pCameraShakeComponent->GetAmplitude();
	m_CameraShakeFrequency = pCameraShakeComponent->GetFrequency();
}

/**
 *	OxiCameraComponent::Update_Internal
 */
void OxiCameraComponent::Update_Internal( const float DeltaTime ) {
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
			if ( m_pTarget != nullptr ) {

				// Target blend to
				kbVec3 targetPosition = m_pTarget->GetPosition();
				if ( m_SwitchTargetCurT < 1.0f ) {
					m_SwitchTargetCurT += m_SwitchTargetBlendSpeed * g_pGame->GetFrameDT();
					targetPosition = kbLerp( m_SwitchTargetStartPos, targetPosition, kbSaturate( m_SwitchTargetCurT ) );
				}

				// LookAt offset blend
				kbVec3 lookAtOffset = m_LookAtOffset;
				if ( m_SwitchLookAtOffsetCurT < 1.0f ) {
					m_SwitchLookAtOffsetCurT += m_SwitchLookAtOffsetBlendSpeed * g_pGame->GetFrameDT();
					lookAtOffset = kbLerp( m_LookAtOffset, m_LookAtOffsetTarget, kbSaturate( m_SwitchLookAtOffsetCurT ) );
					if ( m_SwitchLookAtOffsetCurT > 1.0f ) {
						m_LookAtOffset = m_LookAtOffsetTarget;
					}
				}

				// Position offset blend
				kbVec3 positionOffset = m_PositionOffset;
				if ( m_SwitchPosOffsetCurT < 1.0f ) {
					m_SwitchPosOffsetCurT += m_SwitchPosOffsetBlendSpeed * g_pGame->GetFrameDT();
					positionOffset = kbLerp( m_PositionOffset, m_PosOffsetTarget, kbSaturate( m_SwitchPosOffsetCurT ) );
					if ( m_SwitchPosOffsetCurT >= 1.0f ) {
						m_PositionOffset = m_PosOffsetTarget;
					}
				}

				GetOwner()->SetPosition( targetPosition + positionOffset );

				kbMat4 cameraDestRot;
				cameraDestRot.LookAt( GetOwner()->GetPosition(), targetPosition + lookAtOffset, kbVec3::up );
				cameraDestRot.InvertFast();
				GetOwner()->SetOrientation( kbQuatFromMatrix( cameraDestRot ) );

				const kbVec3 cameraDestPos = targetPosition + positionOffset;
				GetOwner()->SetPosition( cameraDestPos + cameraDestRot[0].ToVec3() * camShakeOffset.x + cameraDestRot[1].ToVec3() * camShakeOffset.y );
				GetOwner()->SetPosition( cameraDestPos + cameraDestRot[0].ToVec3() * camShakeOffset.x + cameraDestRot[1].ToVec3() * camShakeOffset.y );
			}
		}
		break;
	}
}

/**
 *	OxiCameraShakeComponent::Constructor
 */
void OxiCameraShakeComponent::Constructor() {
	m_Duration = 1.0f;
	m_AmplitudeX = 0.025f;
	m_AmplitudeY = 0.019f;

	m_FrequencyX = 15.0f;
	m_FrequencyY = 10.0f;

	m_bActivateOnEnable = false;
}

/**
 *	OxiCameraShakeComponent::SetEnable_Internal
 */
void OxiCameraShakeComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable ); 
	
	if ( bEnable ) {
		// Disable so that this component doesn't prevent it's owning entity to linger past it's life time
		Enable( false );

		if ( m_bActivateOnEnable )  {
			OxiCameraComponent *const pCam = (OxiCameraComponent*)g_pOxiGame->GetMainCamera();
			if ( pCam != nullptr ) {
				pCam->StartCameraShake( this );
			}
		}
	} 
}
