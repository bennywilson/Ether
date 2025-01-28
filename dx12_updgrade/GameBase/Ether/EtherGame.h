//===================================================================================================
// EtherGame.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERGAME_H_
#define _ETHERGAME_H_

#include "kbGame.h"
#include "kbJobManager.h"
#include "kbRenderer.h"

enum eCameraMode_t {
	Cam_FirstPerson,
	Cam_ThirdPerson,
	Cam_Free,
};

class EtherFireEntity {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												EtherFireEntity( const kbVec3 & position, const kbPrefab *const pFirePrefab, const kbPrefab *const pSmokePrefab, const kbPrefab *const pParticlePrefab, const kbPrefab *const pFireLightPrefab );

	void										Update( const float DeltaTime );
	void										Destroy();

	kbVec3										GetPosition() const { return m_Position; }

	float										GetScorchRadius() const { return m_ScorchRadius; }
	kbVec3										GetScorchOffset() const { return m_ScorchOffset; }

	bool										IsFinished() const { return m_bIsFinished; }

protected:
	kbGameEntity *								m_pFireEntity;
	kbGameEntity *								m_pSmokeEntity;
	kbGameEntity *								m_pEmberEntity;
	kbGameEntity *								m_pFireLight;

	kbVec3										m_Position;

	float										m_StartingTimeSeconds;
	float										m_FadeOutStartTime;

	kbVec3										m_FireStartPos;
	kbVec3										m_SmokeStartPos;
	kbVec3										m_EmberStartPos;
	kbVec3										m_StartFireLightPos;
	kbColor										m_StartFireColor;
	float										m_StartingScrollSpeed;

	kbVec3										m_ScorchOffset;
	float										m_ScorchRadius;
	float										m_NextStateChangeTime;
	int											m_ScorchState;

	float										m_RandomScroller;
	kbVec3										m_ScrollRate;
	bool										m_bIsFinished;
};

/**
 *	EtherGame
 */
class EtherGame : public kbGame, public kbRenderHook {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												EtherGame();
	virtual										~EtherGame();

	virtual kbGameEntity *						CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & desiredLocation );

	kbCamera &									GetCamera() { return m_Camera; }
	const eCameraMode_t &						GetCameraMode() const { return m_CameraMode; }

protected:

	virtual void								InitGame_Internal() override;
	virtual void								PlayGame_Internal() override;
	virtual void								StopGame_Internal() override;
	virtual void								LevelLoaded_Internal() override;

	virtual void								AddGameEntity_Internal( kbGameEntity *const pEntity ) override;
	virtual void								RemoveGameEntity_Internal( kbGameEntity *const pEntity ) override;

	virtual void								PreUpdate_Internal() override;

	void										ProcessInput( const float deltaTimeSec );
	void										UpdateWorld( const float deltaTimeSec );

protected:

	virtual void								RenderSync() override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst );

	kbCamera									m_Camera;
	eCameraMode_t								m_CameraMode;
	class EtherPlayerComponent *				m_pPlayerComponent;

	kbTimer										m_GameStartTimer;
};


extern EtherGame * g_pEtherGame;

#endif