//===================================================================================================
// DQuadGame.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _DQUADGAME_H_
#define _DQUADGAME_H_

#include "kbGame.h"
#include "kbJobManager.h"
#include "kbWorldGen.h"

enum eCameraMode_t {
	Cam_FirstPerson,
	Cam_ThirdPerson,
	Cam_Free,
};

enum DQuadCustomNetMessageType_t {
	DQuadMsg_SpawnActor,
	DQuadMsg_DamageActor,
};

struct DQuadNetMsg_t {
	DQuadCustomNetMessageType_t m_MsgType;
};

struct DQuadSpawnActorNetMsg_t : public DQuadNetMsg_t {
	DQuadSpawnActorNetMsg_t() { m_MsgType = DQuadMsg_SpawnActor; }

	kbGUID						m_PrefabGUID;
	kbVec3						m_Position;
	kbQuat						m_Orientation;
};

struct DQuadDamageActorNetMsg_t : public DQuadNetMsg_t {
	DQuadDamageActorNetMsg_t() : 
		m_TargetNetId( -1 ),
		m_SourceNetId( - 1 ),
		m_DamageAmt( 0 ),
		m_bKilled( true ) { m_MsgType = DQuadMsg_DamageActor; }

	int							m_TargetNetId;
	int							m_SourceNetId;
	float						m_DamageAmt;
	bool						m_bKilled;
};

/**
 *	DQuadGame
 */
class DQuadGame : public kbGame {
public:
											DQuadGame();
	virtual									~DQuadGame();

	// Hack.  The Component->IsA( kbDQuadWorldGenComponent::GetType() ) was failing, so we'll set it this way
	void									SetWorldGenComponent( const kbDQuadWorldGenComponent *const pWorldGen ) { m_pWorldGenComponent = pWorldGen; }

	bool									TraceAgainstWorld( const kbVec3 & startPt, const kbVec3 & endPt, kbVec3 & collisionPt, const bool bTraceAgainstDynamicCollision );

	virtual kbGameEntity *					CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & desiredLocation );

	kbCamera &								GetCamera() { return m_Camera; }
	const eCameraMode_t &					GetCameraMode() const { return m_CameraMode; }

	// Networking
	virtual void							ServerNewPlayerJoined( kbCreateActorNetMsg_t & NetMsg );

	virtual void							ClientNetMsgNotify( const kbNetMsg_t *const NetMsg );
	virtual void							ServerNetMsgNotify( const kbNetMsg_t *const NetMsg );

protected:

	virtual void							InitGame_Internal();
	virtual void							PlayGame_Internal();
	virtual void							StopGame_Internal();
	virtual void							LevelLoaded_Internal();
	virtual void							CustomNetPackageReceived_Internal( const kbCustomNetMsg_t & netMsg );

	virtual void							AddGameEntity_Internal( kbGameEntity *const pEntity );
	virtual void							Update_Internal( const float DT );

	void									ProcessInput( const float DT );

	void									UpdateWorld( const float DT );

	void									AddPrefabToEntity( const kbPackage *const pPrefab, const std::string & prefabName, kbGameEntity *const pEntity, const bool bComponentsOnly );

private:

	kbCamera								m_Camera;
	eCameraMode_t							m_CameraMode;
	class kbDQuadPlayerComponent *			m_pPlayerComponent;

	kbTimer									m_GameStartTimer;
	const kbDQuadWorldGenComponent *		m_pWorldGenComponent;

	const kbPackage *						m_pCharacterPackage;
	const kbPackage *						m_pWeaponsPackage;
	const kbPackage *						m_pFXPackage;
};

#endif