//===================================================================================================
// OxiGame.h
//
//
// 2020 kbEngine 2.0
//===================================================================================================
#ifndef _OxiGame_H_
#define _OxiGame_H_

#include "kbGame.h"
#include "kbJobManager.h"
#include "kbRenderer.h"

class kbLevelComponent;


/**
 *	OxiLevelComponent
 */
class OxiLevelComponent : public kbLevelComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( OxiLevelComponent, kbLevelComponent );

private:
	int											m_Dummy2;
};

/**
 *	ECameraMoveMode
 */
enum ECameraMoveMode {
	MoveMode_None,
	MoveMode_Follow,
};

/**
 *	OxiCameraShakeComponent
 */
class OxiCameraShakeComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( OxiCameraShakeComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:

	float										GetDuration() const { return m_Duration; }
	kbVec2										GetAmplitude() const { return kbVec2( m_AmplitudeX, m_AmplitudeY ); }
	kbVec2										GetFrequency() const { return kbVec2( m_FrequencyX, m_FrequencyY ); }

	void										SetEnable_Internal( const bool bEnable );

	float										m_Duration;
	float										m_AmplitudeX;
	float										m_AmplitudeY;

	float										m_FrequencyX;
	float										m_FrequencyY;

private:

	bool										m_bActivateOnEnable;
};

/**
 *	OxiCameraComponent
 */
class OxiCameraComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( OxiCameraComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:
	
	void										StartCameraShake( const OxiCameraShakeComponent *const pCameraShakeComponent );

	void										SetTarget( const kbGameEntity *const pTarget, const float blendRate );
	void										SetPositionOffset( const kbVec3 & posOffset, const float blendRate );
	void										SetLookAtOffset( const kbVec3 & lookAtOffset, const float blendRate );

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	float										m_NearPlane;
	float										m_FarPlane;
	kbVec3										m_PositionOffset;
	kbVec3										m_LookAtOffset;

	// Game
	ECameraMoveMode								m_MoveMode;
	const kbGameEntity *						m_pTarget;
	float										m_SwitchTargetBlendSpeed;
	float										m_SwitchTargetCurT;
	kbVec3										m_SwitchTargetStartPos;

	float										m_SwitchPosOffsetBlendSpeed;
	float										m_SwitchPosOffsetCurT;
	kbVec3										m_PosOffsetTarget;

	float										m_SwitchLookAtOffsetBlendSpeed;
	float										m_SwitchLookAtOffsetCurT;
	kbVec3										m_LookAtOffsetTarget;

	float										m_CameraShakeStartTime;
	kbVec2										m_CameraShakeStartingOffset;
	float										m_CameraShakeDuration;
	kbVec2										m_CameraShakeAmplitude;
	kbVec2										m_CameraShakeFrequency;
};

/**
 *	OxiGame
 */
class OxiGame : public kbGame, public kbRenderHook {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												OxiGame();
	virtual										~OxiGame();

	OxiCameraComponent *						GetMainCamera() const { return m_pMainCamera; }

protected:

	virtual void								InitGame_Internal() override;
	virtual void								PlayGame_Internal() override;
	virtual void								StopGame_Internal() override;
	virtual void								LevelLoaded_Internal() override;

	virtual void								AddGameEntity_Internal( kbGameEntity *const pEntity ) override;
	virtual void								RemoveGameEntity_Internal( kbGameEntity *const pEntity ) override;


	virtual void								PreUpdate_Internal() override;
	virtual void								PostUpdate_Internal() override;

	virtual kbGameEntity *						CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & desiredLocation ) override;

protected:

	virtual void								RenderSync() override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) override;

	kbCamera									m_Camera;

	kbTimer										m_GameStartTimer;

	OxiCameraComponent *						m_pMainCamera;

private:

	void										ProcessInput( const float deltaTimeSec );
};


#endif