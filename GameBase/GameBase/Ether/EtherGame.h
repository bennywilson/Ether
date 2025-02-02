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
#include "EtherWorldGen.h"
#include "EtherAI.h"
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

	// Hack.  The Component->IsA( EtherWorldGenComponent::GetType() ) was failing, so we'll set it this way
	void										SetWorldGenComponent( EtherWorldGenComponent *const pWorldGen ) { m_pWorldGenComponent = pWorldGen; }

	bool										TraceAgainstWorld( const kbVec3 & startPt, const kbVec3 & endPt, kbVec3 & collisionPt, 
																   const bool bTraceAgainstDynamicCollision );

	bool										CoverObjectsPointTest( const EtherCoverObject *& pCoverObject, const kbVec3 & startPt ) const;
	void										MoveActorAlongGround( class EtherActorComponent *const pActor, const kbVec3 & startPt, const kbVec3 & endPt );

	virtual kbGameEntity *						CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & desiredLocation );

	kbCamera &									GetCamera() { return m_Camera; }
	const eCameraMode_t &						GetCameraMode() const { return m_CameraMode; }

	EtherAIManager &							GetAIManager() { return m_AIManager; }

	const kbVec3 &								GetHMDWorldOffset() const { return m_HMDWorldOffset; }
	
	struct frameBulletShots {
		kbComponent * pHitComponent;
		kbVec3 shotStart;
		kbVec3 shotEnd;
	};
	void										RegisterBulletShot( kbComponent *const pComponent, const kbVec3 & shotStart, const kbVec3 & shotEnd );
	const std::vector<frameBulletShots> &		GetShotsThisFrame() const { if ( m_ShotsThisFrame[0].size() > 0 ) return m_ShotsThisFrame[0]; else return m_ShotsThisFrame[1]; }

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

	void										UpdateFires_GameThread( const float DeltaTime );
	void										UpdateFires_RenderSync();
	void										UpdateFires_RenderHook( const kbTerrainComponent *const pTerrain );

	void										AddPrefabToEntity( const kbPackage *const pPrefab, const std::string & prefabName, kbGameEntity *const pEntity, 
																   const bool bComponentsOnly );

protected:

	virtual void								RenderSync() override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst );

	kbCamera									m_Camera;
	eCameraMode_t								m_CameraMode;
	class EtherPlayerComponent *				m_pPlayerComponent;

	EtherAIManager								m_AIManager;

	kbTimer										m_GameStartTimer;
	EtherWorldGenComponent *					m_pWorldGenComponent;

	const kbPackage *							m_pCharacterPackage;
	const kbPackage *							m_pWeaponsPackage;
	const kbPackage *							m_pFXPackage;
	kbShader *									m_pTranslucentShader;

	kbVec3										m_HMDWorldOffset;

	int											m_ShotFrame;
	std::vector<frameBulletShots>				m_ShotsThisFrame[2];
	std::vector<frameBulletShots>				m_RenderThreadShotsThisFrame;

	// kbRenderHook
	kbRenderTexture *							m_pBulletHoleRenderTexture;
	kbRenderTexture *							m_pGrassCollisionTexture;
	kbRenderTexture *							m_pGrassCollisionReadBackTexture;
	kbRenderTexture *							mww_DynamicLightMapColors;
	kbRenderTexture *							m_DynamicLightMapDirections;

	kbShader *									m_pCollisionMapScorchGenShader;
	kbShader *									m_pCollisionMapDamageGenShader;
	kbShader *									m_pCollisionMapTimeGenShader;

	kbShader *									m_pBulletHoleUpdateShader;
	kbShaderParamOverrides_t					m_ShaderParamOverrides;

	static const int NUM_FIRE_PREFABS = 16;
	const kbPrefab *							m_FirePrefabs[NUM_FIRE_PREFABS];
	const kbPrefab *							m_SmokePrefabs[NUM_FIRE_PREFABS];
	const kbPrefab *							m_EmberPrefabs[NUM_FIRE_PREFABS];
	const kbPrefab *							m_FireLightPrefabs[NUM_FIRE_PREFABS];

	std::vector<EtherFireEntity>				m_FireEntities;

	struct RenderThreadScorch {
		kbVec3									m_Position;
		kbVec3									m_Size;
	};
	std::vector<RenderThreadScorch>				m_RenderThreadScorch;
};


extern EtherGame * g_pEtherGame;

#endif