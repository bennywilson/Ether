/// EtherGame.h
///
/// 2025 blk 1.0

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

	virtual kbGameEntity* CreatePlayer(const int netId, const kbGUID& prefabGUID, const Vec3& desiredLocation);

	kbCamera& GetCamera() { return m_Camera; }
	const eCameraMode_t& GetCameraMode() const { return m_CameraMode; }

protected:
	virtual void init_internal() override;
	virtual void play_internal() override;
	virtual void stop_internal() override;
	virtual void level_loaded_internal() override;

	virtual void add_entity_internal(kbGameEntity* const pEntity) override;
	virtual void remove_entity_internal(kbGameEntity* const pEntity) override;

	virtual void preupdate_internal() override;

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
