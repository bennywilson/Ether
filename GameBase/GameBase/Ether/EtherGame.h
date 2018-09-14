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
												EtherFireEntity( const kbVec3 & position, const kbPrefab *const pFirePrefab, const kbPrefab *const pSmokePrefab, const kbPrefab *const pParticlePrefab );

	void										Update();
	void										Destroy();

	kbVec3										GetPosition() const { return m_Position; }

	float										GetScorchRadius() const { return m_ScorchRadius; }

protected:
	kbGameEntity *								m_pFireEntity;
	kbGameEntity *								m_pSmokeEntity;
	kbGameEntity *								m_pEmberEntity;
	kbVec3										m_Position;

	float										m_StartingTimeSeconds;
	kbVec3										m_FireScale;
	kbVec3										m_SmokeScale;
	kbVec3										m_EmberScale;

	float										m_ScorchRadius;
	int											m_ScorchState;
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

	void										RegisterBulletShot( kbComponent *const pComponent, const kbVec3 & shotStart, const kbVec3 & shotEnd );

protected:

	virtual void								InitGame_Internal();
	virtual void								PlayGame_Internal();
	virtual void								StopGame_Internal();
	virtual void								LevelLoaded_Internal();

	virtual void								AddGameEntity_Internal( kbGameEntity *const pEntity );
	virtual void								Update_Internal( const float deltaTimeSec );

	void										ProcessInput( const float deltaTimeSec );
	void										UpdateWorld( const float deltaTimeSec );

	void										AddPrefabToEntity( const kbPackage *const pPrefab, const std::string & prefabName, kbGameEntity *const pEntity, 
																   const bool bComponentsOnly );

protected:

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

	// kbRenderHook
	struct frameBulletShots {
		kbComponent * pHitComponent;
		kbVec3 shotStart;
		kbVec3 shotEnd;
	};
	std::vector<frameBulletShots>				m_ShotsThisFrame;
	std::vector<frameBulletShots>				m_RenderThreadShotsThisFrame;
	virtual void								RenderSync() override;
	virtual void								RenderThreadCallBack() override;
	kbRenderTexture *							m_pBulletHoleRenderTexture;
	kbRenderTexture *							m_pGrassCollisionTexture;
	kbRenderTexture *							m_pGrassCollisionReadBackTexture;

	kbShader *									m_pCollisionMapScorchGenShader;
	kbShader *									m_pCollisionMapPushGenShader;
	kbShader *									m_pCollisionMapDamageGenShader;
	kbShader *									m_pCollisionMapTimeGenShader;
	kbShader *									m_pCollisionMapUpdateTimeShader;
	kbShader *									m_pBulletHoleUpdateShader;
	kbShaderParamOverrides_t					m_ShaderParamOverrides;

	static const int NUM_FIRE_PREFABS = 16;
	const kbPrefab *							m_FirePrefabs[16];
	const kbPrefab *							m_SmokePrefabs[16];
	const kbPrefab *							m_EmberPrefabs[16];
	std::vector<EtherFireEntity>				m_FireEntities;

	struct RenderThreadScorch {
		kbVec3									m_Position;
		kbVec3									m_Size;
	};
	std::vector<RenderThreadScorch>				m_RenderThreadScorch;
};


extern EtherGame * g_pEtherGame;

#endif