//===================================================================================================
// ELGame.cpp
//
//
// 2019 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbTypeInfo.h"
#include "ELGame.h"
#include "kbIntersectionTests.h"
#include "DX11/kbRenderer_DX11.h"
#include <directxpackedvector.h>

ELGame * g_pELGame = nullptr;


/**
 *	ELGame::ELGame
 */
ELGame::ELGame() :
	kbRenderHook( RP_FirstPerson ) {


	m_Camera.m_Position.Set( 0.0f, 2600.0f, 0.0f );

	kbErrorCheck( g_pELGame == nullptr, "ELGame::ELGame() - g_pELGame is not nullptr" );
	g_pELGame = this;
}

/**
 *	ELGame::~ELGame
 */
ELGame::~ELGame() {

	kbErrorCheck( g_pELGame != nullptr, "ELGame::~ELGame() - g_pELGame is nullptr" );
	g_pELGame = nullptr;
}

kbRenderObject crossHair;

/**
 *	ELGame::PlayGame_Internal
 */
void ELGame::PlayGame_Internal() {
	g_pRenderer->RegisterRenderHook( this );
}

/**
 *	ELGame::InitGame_Internal
 */
void ELGame::InitGame_Internal() {

	m_GameStartTimer.Reset();
}

/**
 *	ELGame::StopGame_Internal
 */
void ELGame::StopGame_Internal() {

	m_pLocalPlayer = nullptr;

	g_pRenderer->UnregisterRenderHook( this );
}

/**
 *	ELGame::LevelLoaded_Internal
 */
void ELGame::LevelLoaded_Internal() {

}

/**
 *	ELGame::Update_Internal
 */
void ELGame::Update_Internal( float DT ) {

	if ( IsConsoleActive() == false ) {
		ProcessInput( DT );
	}
	
	/*if ( GetAsyncKeyState( VK_LSHIFT ) && GetAsyncKeyState( 'P' ) ) {
	    kbCamera & playerCamera = GetCamera();

		for ( int i = 0; i < GetGameEntities().size();  i++ ) {

			const kbGameEntity *const pCurEntity = GetGameEntities()[i];
			kbPlayerStartComponent *const pStart = (kbPlayerStartComponent*)pCurEntity->GetComponentByType( kbPlayerStartComponent::GetType() );
			if ( pStart == nullptr ) {
				continue;
			}

			const kbVec3 newPos = pCurEntity->GetPosition();
			const kbQuat newRotation = pCurEntity->GetOrientation();

			playerCamera.m_Position = newPos;
			playerCamera.m_Rotation = newRotation;
			playerCamera.m_RotationTarget = newRotation;

			m_pLocalPlayer->SetPosition( newPos );
			m_pLocalPlayer->SetOrientation( newRotation );
			break;
		}
	}*/


	// Update renderer cam
	g_pD3D11Renderer->SetRenderViewTransform( nullptr, m_Camera.m_Position, m_Camera.m_Rotation );

	/*if ( g_ShowPos.GetBool() ) {
		std::string PlayerPos;
		PlayerPos += "x:";
		PlayerPos += std::to_string( m_Camera.m_Position.x );
		PlayerPos += " y:";
		PlayerPos += std::to_string( m_Camera.m_Position.y );
		PlayerPos += " z:";
		PlayerPos += std::to_string( m_Camera.m_Position.z );
	
		g_pD3D11Renderer->DrawDebugText( PlayerPos, 0, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );
	}*/

}

/**
 *	ELGame::AddGameEntity_Internal
 */
void ELGame::AddGameEntity_Internal( kbGameEntity *const pEntity ) {
	if ( pEntity == nullptr ) {
		kbLog( "ELGame::AddGameEntity_Internal() - nullptr Entity" );
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
 *	ELGame::CreatePlayer
 */
kbGameEntity * ELGame::CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & DesiredLocation ) {

	return nullptr;
}

/**
 *	ELGame::ProcessInput
 */
void ELGame::ProcessInput( const float DT ) {

	if ( m_pLocalPlayer == nullptr || m_pLocalPlayer->GetActorComponent()->IsDead() ) {
		return;
	}

	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if ( bFirstRun ) {
		ShowCursor( false );
		bFirstRun = false;
	}

	if ( GetAsyncKeyState( VK_SPACE ) ) {
		const kbMat4 orientation( kbVec4( 0.0f, 0.0f, -1.0f, 0.0f ), kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ), kbVec4( -1.0f, 0.0f, 0.0f, 0.0f ), kbVec3::zero );
		m_pLocalPlayer->SetOrientation( kbQuatFromMatrix( orientation ) );
		m_Camera.m_Rotation = kbQuatFromMatrix( orientation );
		m_Camera.m_RotationTarget = kbQuatFromMatrix( orientation );

		POINT CursorPos;
		GetCursorPos( &CursorPos );
		RECT rc;
 		GetClientRect( m_Hwnd, &rc );
		SetCursorPos( (rc.right - rc.left ) / 2, (rc.bottom - rc.top) / 2 );
	}
}

/**
 *	ELGame::AddPrefabToEntity
 */
void ELGame::AddPrefabToEntity( const kbPackage *const pPrefabPackage, const std::string & prefabName, kbGameEntity *const pEntity, const bool bComponentsOnly ) {
	
}

/**
 *	ELGame::RenderSync
 */
void ELGame::RenderSync() {

	
}

/**
 *	ELGame::RenderHookCallBack
 */
static float g_TimeMultiplier = 0.95f / 0.016f;
void ELGame::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {

	if ( pSrc == nullptr || pDst == nullptr ) {
		return;
	}

	g_pRenderer->RT_CopyRenderTarget( pSrc, pDst );
}
