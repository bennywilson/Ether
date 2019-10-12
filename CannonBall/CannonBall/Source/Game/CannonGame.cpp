//===================================================================================================
// CannonGame.cpp
//
//
// 2019 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbTypeInfo.h"
#include "kbIntersectionTests.h"
#include "kbLevelComponent.h"
#include "DX11/kbRenderer_DX11.h"
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "UI/CannonUI.h"
#include <directxpackedvector.h>
#include "kbEditorEntity.h"

CannonGame * g_pCannonGame = nullptr;

/**
 *	CannonGame::CannonGame
 */
CannonGame::CannonGame() :
	kbRenderHook( RP_FirstPerson ),
	m_pMainCamera( nullptr ),
	m_pPlayerComp( nullptr ) {

	m_Camera.m_Position.Set( 0.0f, 2600.0f, 0.0f );

	kbErrorCheck( g_pCannonGame == nullptr, "CannonGame::CannonGame() - g_pCannonGame is not nullptr" );
	g_pCannonGame = this;
}

/**
 *	CannonGame::~CannonGame
 */
CannonGame::~CannonGame() {

	kbErrorCheck( g_pCannonGame != nullptr, "CannonGame::~CannonGame() - g_pCannonGame is nullptr" );
	g_pCannonGame = nullptr;
}

/**
 *	CannonGame::PlayGame_Internal
 */
void CannonGame::PlayGame_Internal() {
	g_pRenderer->RegisterRenderHook( this );
}

/**
 *	CannonGame::InitGame_Internal
 */
void CannonGame::InitGame_Internal() {

	m_GameStartTimer.Reset();

	GetSoundManager().SetMasterVolume( CannonBallGameSettingsComponent::Get()->m_Volume / 100.0f );

	kbShaderParamOverrides_t shaderParam;
	shaderParam.SetVec4( "globalTint", kbVec4( 0.0f, 0.0f, 0.0f, 1.0f - ( CannonBallGameSettingsComponent::Get()->m_Brightness / 100.0f ) ) );
	g_pRenderer->SetGlobalShaderParam( shaderParam );
}

/**
 *	CannonGame::StopGame_Internal
 */
void CannonGame::StopGame_Internal() {

	m_pLocalPlayer = nullptr;

	g_pRenderer->UnregisterRenderHook( this );
}

/**
 *	CannonGame::LevelLoaded_Internal
 */
void CannonGame::LevelLoaded_Internal() {

	m_pMainCamera = nullptr;

	int cameraIdx = -1;
	const std::vector<kbGameEntity*> & GameEnts = GetGameEntities();
	for ( int i = 0; i < GameEnts.size(); i++ ) {
		kbGameEntity *const pCurEnt = GameEnts[i];

		if ( m_pMainCamera == nullptr ) {
			m_pMainCamera = pCurEnt->GetComponent<CannonCameraComponent>();
			if ( m_pMainCamera != nullptr ) {
				cameraIdx = i;
			}
		}

		if ( m_pPlayerComp == nullptr ) {
			CannonActorComponent *const pActor = pCurEnt->GetComponent<CannonActorComponent>();
			if ( pActor != nullptr && pActor->IsPlayer() ) {
				m_pPlayerComp = pActor;
			}
		}
	}

	if ( cameraIdx >= 0 ) {
		SwapEntitiesByIdx( cameraIdx, GameEnts.size() - 1 );
	}

	kbWarningCheck( m_pMainCamera != nullptr, "CannonGame::LevelLoaded_Internal() - No camera found.");
	kbWarningCheck( m_pPlayerComp != nullptr, "CannonGame::LevelLoaded_Internal() - No player found.");
}

/**
 *	CannonGame::PreUpdate_Internal
 */
void CannonGame::PreUpdate_Internal() {

	const float frameDT = GetFrameDT();
	if ( IsConsoleActive() == false ) {
		ProcessInput( frameDT );
	}
}

/**
 *	CannonGame::PostUpdate_Internal
 */
void CannonGame::PostUpdate_Internal() {

	// Update renderer cam
	if ( m_pMainCamera != nullptr ) {
		g_pD3D11Renderer->SetRenderViewTransform( nullptr, m_pMainCamera->GetOwner()->GetPosition(), m_pMainCamera->GetOwner()->GetOrientation() );
	}
}

/**
 *	CannonGame::AddGameEntity_Internal
 */
void CannonGame::AddGameEntity_Internal( kbGameEntity *const pEntity ) {

	if ( pEntity == nullptr ) {
		kbWarning( "CannonGame::AddGameEntity_Internal() - nullptr Entity" );
		return;
	}

	if ( m_pLocalPlayer == nullptr || m_pPlayerComp == nullptr ) {
		CannonActorComponent *const pActor = pEntity->GetComponent<CannonActorComponent>();
		if ( pActor != nullptr && pActor->IsPlayer() ) {
			m_pLocalPlayer = pEntity;
			m_pPlayerComp = pActor;
		}
	}
}

/**
 *	CannonGame::RemoveGameEntity_Internal
 */
void CannonGame::RemoveGameEntity_Internal( kbGameEntity *const pEntity ) {

	if ( pEntity == nullptr ) {
		kbWarning( "CannonGame::RemoveGameEntity_Internal() - nullptr Entity" );
		return;
	}

	if ( pEntity == m_pLocalPlayer ) {
		m_pLocalPlayer = nullptr;
		m_pPlayerComp = nullptr;
	}
}

/**
 *	CannonGame::CreatePlayer
 */
kbGameEntity * CannonGame::CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & DesiredLocation ) {

	return nullptr;
}

/**
 *	CannonGame::ProcessInput
 */
void CannonGame::ProcessInput( const float DT ) {

	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if ( bFirstRun ) {
		ShowCursor( false );
		bFirstRun = false;
	}
}

/**
 *	CannonGame::RenderSync
 */
void CannonGame::RenderSync() {

	
}

/**
 *	CannonGame::RenderHookCallBack
 */
static float g_TimeMultiplier = 0.95f / 0.016f;
void CannonGame::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {

	if ( pSrc != nullptr && pDst != nullptr ) {
		g_pRenderer->RT_CopyRenderTarget( pSrc, pDst );
	}
}

/**
 *	CannonGame::HackEditorInit
 */
void CannonGame::HackEditorInit( HWND hwnd, std::vector<class kbEditorEntity *> & editorEntities ) {

	for ( int i = 0; i < editorEntities.size(); i++ ) {
		kbGameEntity *const pCurEnt = editorEntities[i]->GetGameEntity();

		if ( m_pPlayerComp == nullptr ) {
			m_pPlayerComp = (CannonActorComponent*)pCurEnt->GetComponentByType( CannonActorComponent::GetType() );
		}

		if ( m_pMainCamera == nullptr ) {
			m_pMainCamera = (CannonCameraComponent*)pCurEnt->GetComponentByType( CannonCameraComponent::GetType() );
		}
	}

	m_InputManager.Init( hwnd );
}


/**
 *	CannonGame::HackEditorUpdate
 */
void CannonGame::HackEditorUpdate( const float DT, kbCamera *const pEditorCam ) {

	m_InputManager.Update( DT );

	/*if ( m_pPlayerComp != nullptr ) {
		m_pPlayerComp->HandleInput( m_InputManager.GetInput(), DT );
	}*/

	if ( m_pMainCamera != nullptr && pEditorCam != nullptr ) {
		pEditorCam->m_Position = m_pMainCamera->GetOwnerPosition();
		pEditorCam->m_Rotation = pEditorCam->m_RotationTarget = m_pMainCamera->GetOwnerRotation();
	}
}

/**
 *	CannonGame::HackEditorShutdown
 */
void CannonGame::HackEditorShutdown() {
	m_pPlayerComp = nullptr;
	m_pMainCamera = nullptr;
}

/**
 *	CannonLevelComponent::Constructor
 */
void CannonLevelComponent::Constructor() {
	m_Dummy2 = -1;
}

/*
 *	CannonFogComponent::Constructor
 */
void CannonFogComponent::Constructor() {
	SetRenderPass( RP_Translucent );
	m_pShader = nullptr;
	m_FogStartDist = 300;
	m_FogEndDist = 3000;
	m_FogClamp = 1.0f;
	m_FogColor = kbColor::white;
}

/**
 *	CannonFogComponent::RenderHookCallBack
 */
void CannonFogComponent::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {
	//g_pRenderer->RT_ClearRenderTarget( pDst, kbColor::white );

	if ( m_pShader == nullptr ) {
		m_pShader = (kbShader *) g_ResourceManager.GetResource( "./assets/shaders/PostProcess/Fog.kbshader", true, true );
	}

	g_pRenderer->RT_SetRenderTarget( pDst );
	kbShaderParamOverrides_t shaderParams;
	shaderParams.SetVec4( "fog_Start_End_Clamp", kbVec4( m_FogStartDist, m_FogEndDist, m_FogClamp, 0.0f ) );
	shaderParams.SetVec4( "fogColor", m_FogColor );

	kbVec3 position;
	kbQuat orientation;
	g_pRenderer->GetRenderViewTransform( nullptr, position, orientation );

	shaderParams.SetVec4( "cameraPosition", position );
	g_pRenderer->RT_Render2DQuad( kbVec2( 0.5f, 0.5f ), kbVec2( 1.0f, 1.0f ), kbColor::white, m_pShader, &shaderParams );
}

/**
 *	CannonFogComponent::SetEnable_Internal
 */
void CannonFogComponent::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		g_pRenderer->RegisterRenderHook( this );
	} else {
		g_pRenderer->UnregisterRenderHook( this );
	}
}
