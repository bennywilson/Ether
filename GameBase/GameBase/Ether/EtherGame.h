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

/**
 *	EtherFoliageManager
 */
class EtherFoliageManager : public kbRenderHook {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												EtherFoliageManager();
												~EtherFoliageManager();

private:

	virtual void								RenderThreadCallBack() override;
};

/**
 *	EtherGame
 */
class EtherGame : public kbGame {

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

	bool										IsInSlomoMode() const { return m_SlomoStartTime > 0.0f; }
	bool										IsOLCFiring() const { return m_OLCTimer > 0.0f; }
	bool										IsAirstrikeInProgress() const { return  m_AirstrikeTimeLeft > 0.0f; }

	const kbVec3 &								GetHMDWorldOffset() const { return m_HMDWorldOffset; }

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

	// Power ups
	void										ActivateStimPack();
	void										ActivateAirstrike();
	void										ActivateOLC();
	
	void										UpdateStimPack( const float deltaTimeSec );
	void										UpdateAirstrike( const float deltaTimeSec );
	void										UpdateOLC( const float deltaTimeSec );

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

	enum EtherGameState_t {
		TitleScreen,
		VerseScreen,
		GamePlay,
	}											m_CurrentGameState;
	int											m_VerseIdx;

	// Stimpack
	float										m_SlomoStartTime;
	class kbWaveFile *							m_pSlomoSound;
	
	// Airstrike
	float										m_AirstrikeTimeLeft;
	int											m_BombersLeft;
	float										m_NextBomberSpawnTime;
	kbModel *									m_pELBomberModel;
	kbGameEntity *								m_ELBomberEntity[3];
	float										m_BombTimer[3];
	kbWaveFile *								m_pAirstrikeFlybyWave;
	
	// OLC
	float										m_OLCTimer;
	float										m_OLCPostProcess;
	kbWaveFile *								m_pOLCWindupWave;
	kbWaveFile *								m_pOLCExplosion;
	kbVec3										m_OLCTint;
};


extern EtherGame * g_pEtherGame;

#endif