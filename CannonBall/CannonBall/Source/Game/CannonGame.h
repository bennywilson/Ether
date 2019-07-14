//===================================================================================================
// CannonGame.h
//
//
// 2019 kbEngine 2.0
//===================================================================================================
#ifndef _CannonGame_H_
#define _CannonGame_H_

#include "kbGame.h"
#include "kbJobManager.h"
#include "kbRenderer.h"


/**
 *	CannonGame
 */
class CannonGame : public kbGame, public kbRenderHook {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												CannonGame();
	virtual										~CannonGame();



protected:

	virtual void								InitGame_Internal() override;
	virtual void								PlayGame_Internal() override;
	virtual void								StopGame_Internal() override;
	virtual void								LevelLoaded_Internal() override;

	virtual void								AddGameEntity_Internal( kbGameEntity *const pEntity ) override;
	virtual void								PreUpdate_Internal() override;
	virtual void								PostUpdate_Internal() override;

	virtual kbGameEntity *						CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & desiredLocation );

	void										ProcessInput( const float deltaTimeSec );
	

	void										AddPrefabToEntity( const kbPackage *const pPrefab, const std::string & prefabName, kbGameEntity *const pEntity, 
																   const bool bComponentsOnly );

protected:

	virtual void								RenderSync() override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst );

	kbCamera									m_Camera;
	
	kbTimer										m_GameStartTimer;

	class kbLevelComponent *					m_pLevelComp;
	class CannonCameraComponent *				m_pMainCamera;
	class CannonPlayerComponent *				m_pPlayerComp;
};

extern CannonGame * g_pCannonGame;

#endif