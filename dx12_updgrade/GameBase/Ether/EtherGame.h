/// EtherGame.h
///
/// 2025 kbEngine 2.0

#pragma once

#include "kbGame.h"
#include "kbRenderer.h"

enum eCameraMode_t {
	Cam_FirstPerson,
	Cam_ThirdPerson,
	Cam_Free,
};

/// EtherGame
class EtherGame : public kbGame, public kbRenderHook {
public:
	EtherGame();
	virtual	~EtherGame();

	virtual kbGameEntity* CreatePlayer(const int netId, const kbGUID& prefabGUID, const kbVec3& desiredLocation);

	kbCamera& GetCamera() { return m_Camera; }
	const eCameraMode_t& GetCameraMode() const { return m_CameraMode; }

protected:
	virtual void InitGame_Internal() override;
	virtual void PlayGame_Internal() override;
	virtual void StopGame_Internal() override;
	virtual void LevelLoaded_Internal() override;

	virtual void AddGameEntity_Internal(kbGameEntity* const pEntity) override;
	virtual void RemoveGameEntity_Internal(kbGameEntity* const pEntity) override;

	virtual void PreUpdate_Internal() override;

	void ProcessInput(const float deltaTimeSec);
	void UpdateWorld(const float deltaTimeSec);

protected:
	virtual void RenderSync() override;
	virtual void RenderHookCallBack(kbRenderTexture* const pSrc, kbRenderTexture* const pDst);

	kbCamera m_Camera;
	eCameraMode_t m_CameraMode;
	class EtherPlayerComponent* m_pPlayerComponent;

	kbTimer	m_GameStartTimer;
};

extern EtherGame* g_pEtherGame;
