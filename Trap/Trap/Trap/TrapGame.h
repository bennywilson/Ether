//===================================================================================================
// TrapGame.h
//
//
// 2019 kbEngine 2.0
//===================================================================================================
#ifndef _TrapGame_H_
#define _TrapGame_H_

#include "kbGame.h"
#include "kbJobManager.h"
#include "kbRenderer.h"


/**
 *	TrapGame
 */
class TrapGame : public kbGame, public kbRenderHook {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												TrapGame();
	virtual										~TrapGame();



protected:

	virtual void								InitGame_Internal();
	virtual void								PlayGame_Internal();
	virtual void								StopGame_Internal();
	virtual void								LevelLoaded_Internal();

	virtual void								AddGameEntity_Internal( kbGameEntity *const pEntity );
	virtual void								Update_Internal( const float deltaTimeSec );

	virtual kbGameEntity *						CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & desiredLocation );

	void										ProcessInput( const float deltaTimeSec );
	

	void										AddPrefabToEntity( const kbPackage *const pPrefab, const std::string & prefabName, kbGameEntity *const pEntity, 
																   const bool bComponentsOnly );

protected:

	virtual void								RenderSync() override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst );

	kbCamera									m_Camera;
	
	kbTimer										m_GameStartTimer;
};


extern TrapGame * g_pTrapGame;

#endif