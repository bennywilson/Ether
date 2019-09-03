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


class CannonCameraComponent;
class CannonActorComponent;
class kbLevelComponent;


/**
 *	DealAttackInfo_t
 */
template<typename T>
struct DealAttackInfo_t {
	CannonActorComponent * m_pAttacker = nullptr;
	float m_BaseDamage = 1.0f;
	float m_Radius = 0.0f;
	T m_AttackType = (T)0;
};

/**
 *	AttackHitInfo_t
 */
struct AttackHitInfo_t {
	kbGameComponent * m_pHitComponent = nullptr;
	bool m_bHit = false;
};



/**
 *	CannonLevelComponent
 */
class CannonLevelComponent : public kbLevelComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( CannonLevelComponent, kbLevelComponent );

private:
	int											m_Dummy2;
};

/**
 *	CannonGame
 */
class CannonGame : public kbGame, public kbRenderHook {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												CannonGame();
	virtual										~CannonGame();

	CannonCameraComponent *						GetMainCamera() const { return m_pMainCamera; }

	CannonActorComponent *						GetPlayer() const { return m_pPlayerComp; }

	template<typename T>
	T *	GetLevelComponent() const {
			kbErrorCheck( m_pLevelComp != nullptr && m_pLevelComp->IsA( T::GetType() ), "GetLevelComponent<T>() - Incorrect level component type" );
			return (T*) m_pLevelComp;
	}

protected:

	virtual void								InitGame_Internal() override;
	virtual void								PlayGame_Internal() override;
	virtual void								StopGame_Internal() override;
	virtual void								LevelLoaded_Internal() override;

	virtual void								AddGameEntity_Internal( kbGameEntity *const pEntity ) override;
	virtual void								PreUpdate_Internal() override;
	virtual void								PostUpdate_Internal() override;

	virtual kbGameEntity *						CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & desiredLocation ) override;

	virtual void								HackEditorInit( HWND hwnd, std::vector<class kbEditorEntity *> & editorEntities ) override;
	virtual void								HackEditorUpdate( const float DT, kbCamera *const pCamera ) override;
	virtual void								HackEditorShutdown() override;

protected:

	virtual void								RenderSync() override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) override;

	kbCamera									m_Camera;

	kbTimer										m_GameStartTimer;

	kbLevelComponent *							m_pLevelComp;
	CannonCameraComponent *						m_pMainCamera;
	CannonActorComponent *						m_pPlayerComp;

private:

	void										ProcessInput( const float deltaTimeSec );
};

extern CannonGame * g_pCannonGame;

#endif