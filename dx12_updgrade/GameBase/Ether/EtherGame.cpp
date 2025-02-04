/// EtherGame.cpp
///
/// 2025 kbEngine 2.0

#include "kbGame.h"
#include "EtherGame.h"

kbConsoleVariable g_NoEnemies( "noenemies", false, kbConsoleVariable::Console_Bool, "Remove enemies", "" );
kbConsoleVariable g_LockMouse( "lockmouse", true, kbConsoleVariable::Console_Int, "Locks mouse", "" );
kbConsoleVariable g_ShowPos( "showpos", false, kbConsoleVariable::Console_Bool, "Displays player position", "" );

EtherGame * g_pEtherGame = nullptr;


/**
 *	EtherGame::EtherGame
 */
EtherGame::EtherGame() :
	kbRenderHook( RP_FirstPerson ),
	m_CameraMode( Cam_FirstPerson ),
	m_pPlayerComponent( nullptr ) {
	m_Camera.m_Position.Set( 0.0f, 0.0, 10.0f );
	kbErrorCheck( g_pEtherGame == nullptr, "EtherGame::EtherGame() - g_pEtherGame is not nullptr" );
	g_pEtherGame = this;
}

/**
 *	EtherGame::~EtherGame
 */
EtherGame::~EtherGame() {

	kbErrorCheck( g_pEtherGame != nullptr, "EtherGame::~EtherGame() - g_pEtherGame is nullptr" );
	g_pEtherGame = nullptr;
}

kbRenderObject crossHair;

void EtherGame::PlayGame_Internal() {}

void EtherGame::InitGame_Internal() {}

void EtherGame::StopGame_Internal() {}

void EtherGame::LevelLoaded_Internal() {}

void EtherGame::PreUpdate_Internal() {
	ProcessInput(0.0166f);
	// Set render view
}

void EtherGame::AddGameEntity_Internal( kbGameEntity *const pEntity ) {}

void EtherGame::RemoveGameEntity_Internal( kbGameEntity *const pEntity ) {}

void EtherGame::ProcessInput( const float DT ) {

	m_Camera.m_Position = kbVec3(0.0f, 0.0f, 0.0f);
	m_Camera.m_Rotation = kbQuat(0.0f, 0.0f, 0.0f, -1.0f);
	m_Camera.Update();

}

void EtherGame::UpdateWorld( const float DT ) {}

kbGameEntity * EtherGame::CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & DesiredLocation ) {
	return nullptr;
}

void EtherGame::RenderSync() {

	if ( HasFirstSyncCompleted() == false ) {
	}
}

static float g_TimeMultiplier = 0.95f / 0.016f;
void EtherGame::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {
}
