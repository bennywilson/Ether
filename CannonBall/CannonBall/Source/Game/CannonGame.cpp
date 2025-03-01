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

CannonGame* g_pCannonGame = nullptr;

/// CannonGame::CannonGame
CannonGame::CannonGame() :
	kbRenderHook(RP_FirstPerson),
	m_pMainCamera(nullptr),
	m_pPlayerComp(nullptr) {

	m_Camera.m_position.set(0.0f, 2600.0f, 0.0f);

	blk::error_check(g_pCannonGame == nullptr, "CannonGame::CannonGame() - g_pCannonGame is not nullptr");
	g_pCannonGame = this;
}

/// CannonGame::~CannonGame
CannonGame::~CannonGame() {

	blk::error_check(g_pCannonGame != nullptr, "CannonGame::~CannonGame() - g_pCannonGame is nullptr");
	g_pCannonGame = nullptr;
}

/// CannonGame::play_internal
void CannonGame::play_internal() {
	g_pRenderer->RegisterRenderHook(this);
}

/// CannonGame::init_internal
void CannonGame::init_internal() {
	m_GameStartTimer.Reset();

	CannonBallGameSettingsComponent* const pGameSettings = CannonBallGameSettingsComponent::Get();

	GetSoundManager().SetMasterVolume(pGameSettings->m_Volume / 100.0f);

	kbShaderParamOverrides_t shaderParam;

	float brightness = (pGameSettings->m_Brightness / 100.0f);
	brightness = (brightness * 0.5f) + 0.5f;

	shaderParam.SetVec4("globalTint", Vec4(0.0f, 0.0f, 0.0f, 1.0f - brightness));
	g_pRenderer->SetGlobalShaderParam(shaderParam);

	const float LOD = (float)pGameSettings->m_VisualQuality / 100.0f;
	kbTerrainComponent::SetTerrainLOD(LOD);
}

/// CannonGame::stop_internal
void CannonGame::stop_internal() {
	m_pLocalPlayer = nullptr;

	g_pRenderer->UnregisterRenderHook(this);
}

/// CannonGame::level_loaded_internal
void CannonGame::level_loaded_internal() {
	m_pMainCamera = nullptr;

	int cameraIdx = -1;
	const std::vector<kbGameEntity*>& GameEnts = GetGameEntities();
	for (int i = 0; i < GameEnts.size(); i++) {
		kbGameEntity* const pCurEnt = GameEnts[i];

		if (m_pMainCamera == nullptr) {
			m_pMainCamera = pCurEnt->GetComponent<CannonCameraComponent>();
			if (m_pMainCamera != nullptr) {
				cameraIdx = i;
			}
		}

		if (m_pPlayerComp == nullptr) {
			CannonActorComponent* const pActor = pCurEnt->GetComponent<CannonActorComponent>();
			if (pActor != nullptr && pActor->IsPlayer()) {
				m_pPlayerComp = pActor;
			}
		}
	}

	if (cameraIdx >= 0) {
		swap_entities_by_idx(cameraIdx, GameEnts.size() - 1);
	}

	blk::warn_check(m_pMainCamera != nullptr, "CannonGame::LevelLoaded_Internal() - No camera found.");
	blk::warn_check(m_pPlayerComp != nullptr, "CannonGame::LevelLoaded_Internal() - No player found.");
}

/// CannonGame::preupdate_internal
void CannonGame::preupdate_internal() {
	const float frameDT = GetFrameDT();
	if (is_console_active() == false) {
		ProcessInput(frameDT);
	}
}

/// CannonGame::postupdate_internal
void CannonGame::postupdate_internal() {
	// Update renderer cam
	if (m_pMainCamera != nullptr && m_pMainCamera->GetOwner() != nullptr) {
		g_pD3D11Renderer->SetRenderViewTransform(nullptr, m_pMainCamera->GetOwner()->GetPosition(), m_pMainCamera->GetOwner()->GetOrientation());
	}
}

/// CannonGame::add_entity_internal
void CannonGame::add_entity_internal(kbGameEntity* const pEntity) {
	if (pEntity == nullptr) {
		blk::warn("CannonGame::AddGameEntity_Internal() - nullptr Entity");
		return;
	}

	if (m_pLocalPlayer == nullptr || m_pPlayerComp == nullptr) {
		CannonActorComponent* const pActor = pEntity->GetComponent<CannonActorComponent>();
		if (pActor != nullptr && pActor->IsPlayer()) {
			m_pLocalPlayer = pEntity;
			m_pPlayerComp = pActor;
		}
	}
}

/// CannonGame::remove_entity_internal
void CannonGame::remove_entity_internal(kbGameEntity* const pEntity) {
	if (pEntity == nullptr) {
		blk::warn("CannonGame::RemoveGameEntity_Internal() - nullptr Entity");
		return;
	}

	if (pEntity == m_pLocalPlayer) {
		m_pLocalPlayer = nullptr;
		m_pPlayerComp = nullptr;
	}
}

/// CannonGame::CreatePlayer
kbGameEntity* CannonGame::CreatePlayer(const int netId, const kbGUID& prefabGUID, const Vec3& DesiredLocation) {

	return nullptr;
}

/// CannonGame::ProcessInput
void CannonGame::ProcessInput(const float DT) {

	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if (bFirstRun) {
		ShowCursor(false);
		bFirstRun = false;
	}
}

/// CannonGame::RenderSync
void CannonGame::RenderSync() {


}

/// CannonGame::RenderHookCallBack
static float g_TimeMultiplier = 0.95f / 0.016f;
void CannonGame::RenderHookCallBack(kbRenderTexture* const pSrc, kbRenderTexture* const pDst) {

	if (pSrc != nullptr && pDst != nullptr) {
		g_pRenderer->RT_CopyRenderTarget(pSrc, pDst);
	}
}

/// CannonGame::HackEditorInit
void CannonGame::HackEditorInit(HWND hwnd, std::vector<class kbEditorEntity*>& editorEntities) {

	for (int i = 0; i < editorEntities.size(); i++) {
		kbGameEntity* const pCurEnt = editorEntities[i]->GetGameEntity();

		if (m_pPlayerComp == nullptr) {
			m_pPlayerComp = (CannonActorComponent*)pCurEnt->GetComponentByType(CannonActorComponent::GetType());
		}

		if (m_pMainCamera == nullptr) {
			m_pMainCamera = (CannonCameraComponent*)pCurEnt->GetComponentByType(CannonCameraComponent::GetType());
		}
	}

	m_InputManager.Init(hwnd);
}


/// CannonGame::HackEditorUpdate
void CannonGame::HackEditorUpdate(const float DT, kbCamera* const pEditorCam) {

	m_InputManager.Update(DT);

	/*if ( m_pPlayerComp != nullptr ) {
		m_pPlayerComp->HandleInput( m_InputManager.GetInput(), DT );
	}*/

	if (m_pMainCamera != nullptr && pEditorCam != nullptr) {
		pEditorCam->m_position = m_pMainCamera->owner_position();
		pEditorCam->m_Rotation = pEditorCam->m_RotationTarget = m_pMainCamera->owner_rotation();
	}
}

/// CannonGame::HackEditorShutdown
void CannonGame::HackEditorShutdown() {
	m_pPlayerComp = nullptr;
	m_pMainCamera = nullptr;
}

/// CannonLevelComponent::Constructor
void CannonLevelComponent::Constructor() {
	m_Dummy2 = -1;
}

/*
 *	CannonFogComponent::Constructor
 */
void CannonFogComponent::Constructor() {
	set_render_pass(RP_Translucent);
	m_shader = nullptr;
	m_FogStartDist = 300;
	m_FogEndDist = 3000;
	m_FogClamp = 1.0f;
	m_FogColor = kbColor::white;
}

/// CannonFogComponent::RenderHookCallBack
void CannonFogComponent::RenderHookCallBack(kbRenderTexture* const pSrc, kbRenderTexture* const pDst) {
	//g_pRenderer->RT_ClearRenderTarget( pDst, kbColor::white );

	if (m_shader == nullptr) {
		m_shader = (kbShader*)g_ResourceManager.GetResource("./assets/shaders/PostProcess/Fog.kbshader", true, true);
	}

	g_pRenderer->RT_SetRenderTarget(pDst);
	kbShaderParamOverrides_t shaderParams;
	shaderParams.SetVec4("fog_Start_End_Clamp", Vec4(m_FogStartDist, m_FogEndDist, m_FogClamp, 0.0f));
	shaderParams.SetVec4("fogColor", m_FogColor);

	Vec3 position;
	Quat4 orientation;
	g_pRenderer->GetRenderViewTransform(nullptr, position, orientation);

	shaderParams.SetVec4("cameraPosition", position);
	g_pRenderer->RT_Render2DQuad(Vec2(0.5f, 0.5f), Vec2(1.0f, 1.0f), kbColor::white, m_shader, &shaderParams);
}

/// CannonFogComponent::enable_internal
void CannonFogComponent::enable_internal(const bool bEnable) {

	Super::enable_internal(bEnable);

	if (bEnable) {
		g_pRenderer->RegisterRenderHook(this);
	} else {
		g_pRenderer->UnregisterRenderHook(this);
	}
}
