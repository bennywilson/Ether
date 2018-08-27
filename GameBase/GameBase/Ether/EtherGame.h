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

	void										RegisterHit( kbComponent *const pComponent, const kbVec3 & hitLoc, const kbVec3 & hitDir );

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
	struct hits {
		kbComponent * pHitComponent;
		kbVec3 hitLocation;
		kbVec3 hitDirection;
	};
	std::vector<hits>							m_Hits;
	virtual void								RenderSync() override;
	virtual void								RenderThreadCallBack() override;
	kbRenderTexture *							m_pBulletHoleRenderTexture;
	kbRenderTexture *							m_pBulletTraceRenderTexture;
};


extern EtherGame * g_pEtherGame;

#endif