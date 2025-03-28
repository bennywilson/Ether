/// EtherGame.cpp
///
/// 2025 blk 1.0

#include "kbGame.h"
#include "EtherGame.h"

kbConsoleVariable g_NoEnemies("noenemies", false, kbConsoleVariable::Console_Bool, "Remove enemies", "");
kbConsoleVariable g_LockMouse("lockmouse", true, kbConsoleVariable::Console_Int, "Locks mouse", "");
kbConsoleVariable g_ShowPos("showpos", false, kbConsoleVariable::Console_Bool, "Displays player position", "");

EtherGame* g_pEtherGame = nullptr;


/// EtherGame::EtherGame
EtherGame::EtherGame() :
	kbRenderHook(RP_FirstPerson),
	m_CameraMode(Cam_FirstPerson),
	m_pPlayerComponent(nullptr) {
	m_Camera.m_Position.set(0.0f, 0.0, 10.0f);
	blk::error_check(g_pEtherGame == nullptr, "EtherGame::EtherGame() - g_pEtherGame is not nullptr");
	g_pEtherGame = this;
}

/// EtherGame::~EtherGame
EtherGame::~EtherGame() {

	blk::error_check(g_pEtherGame != nullptr, "EtherGame::~EtherGame() - g_pEtherGame is nullptr");
	g_pEtherGame = nullptr;
}

kbRenderObject crossHair;

void EtherGame::play_internal() {}

void EtherGame::init_internal() {}

void EtherGame::stop_internal() {}

void EtherGame::level_loaded_internal() {}

void EtherGame::preupdate_internal() {
	ProcessInput(0.0166f);
	// Set render view
}

void EtherGame::add_entity_internal(kbGameEntity* const pEntity) {}

void EtherGame::remove_entity_internal(kbGameEntity* const pEntity) {}

void EtherGame::ProcessInput(const float DT) {

	m_Camera.m_Position = Vec3(0.0f, 0.0f, 0.0f);
	m_Camera.m_Rotation = Quat4(0.0f, 0.0f, 0.0f, -1.0f);
	m_Camera.Update();

}

void EtherGame::UpdateWorld(const float DT) {}

kbGameEntity* EtherGame::CreatePlayer(const int netId, const kbGUID& prefabGUID, const Vec3& DesiredLocation) {
	return nullptr;
}

void EtherGame::RenderSync() {

	if (HasFirstSyncCompleted() == false) {
	}
}

static float g_TimeMultiplier = 0.95f / 0.016f;
void EtherGame::RenderHookCallBack(kbRenderTexture* const pSrc, kbRenderTexture* const pDst) {
}
