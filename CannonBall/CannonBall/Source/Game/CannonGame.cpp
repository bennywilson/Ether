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
#include "CannonPlayer.h"
#include "CannonGame.h"
#include <directxpackedvector.h>

CannonGame * g_pCannonGame = nullptr;

/**
 *	CannonGame::CannonGame
 */
CannonGame::CannonGame() :
	kbRenderHook( RP_FirstPerson ),
	m_pLevelComp( nullptr ),
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

kbRenderObject crossHair;

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
}

/**
 *	CannonGame::StopGame_Internal
 */
void CannonGame::StopGame_Internal() {

	m_pLocalPlayer = nullptr;
	m_pLevelComp = nullptr;

	g_pRenderer->UnregisterRenderHook( this );
}

/**
 *	CannonGame::LevelLoaded_Internal
 */
void CannonGame::LevelLoaded_Internal() {

	m_pLevelComp = nullptr;
	m_pMainCamera = nullptr;

	const std::vector<kbGameEntity*> & GameEnts = GetGameEntities();
	for ( int i = 0; i < GameEnts.size(); i++ ) {
		kbGameEntity *const pCurEnt = GameEnts[i];
		if ( m_pLevelComp == nullptr ) {
			m_pLevelComp = (kbLevelComponent*)pCurEnt->GetComponentByType( kbLevelComponent::GetType() );
		}

		if ( m_pMainCamera == nullptr ) {
			m_pMainCamera = (CannonCameraComponent*)pCurEnt->GetComponentByType( CannonCameraComponent::GetType() );
		}

		if ( m_pPlayerComp == nullptr ) {
			m_pPlayerComp = (CannonPlayerComponent*)pCurEnt->GetComponentByType( CannonPlayerComponent::GetType() );
		}
	}

	kbWarningCheck( m_pLevelComp != nullptr, "CannonGame::LevelLoaded_Internal() - No level component found.");
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
		kbLog( "CannonGame::AddGameEntity_Internal() - nullptr Entity" );
		return;
	}

	if ( pEntity == m_pLocalPlayer ) {

		m_Camera.m_Position.y = 256.0f;
		m_pLocalPlayer->SetPosition( m_Camera.m_Position );

		const kbMat4 orientation( kbVec4( 0.0f, 0.0f, -1.0f, 0.0f ), kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ), kbVec4( -1.0f, 0.0f, 0.0f, 0.0f ), kbVec3::zero );
		m_pLocalPlayer->SetOrientation( kbQuatFromMatrix( orientation ) );
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

	if ( m_pPlayerComp != nullptr ) {
		m_pPlayerComp->HandleInput( GetInput(), DT );
	}
}

/**
 *	CannonGame::AddPrefabToEntity
 */
void CannonGame::AddPrefabToEntity( const kbPackage *const pPrefabPackage, const std::string & prefabName, kbGameEntity *const pEntity, const bool bComponentsOnly ) {
	
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