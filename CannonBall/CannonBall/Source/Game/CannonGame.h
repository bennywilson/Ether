//===================================================================================================
// CannonGame.h
//
//
// 2019-2025 kbEngine 2.0
//===================================================================================================
#pragma once

#include "kbGame.h"
#include "kbJobManager.h"
#include "kbRenderer.h"


class CannonCameraComponent;
class CannonActorComponent;
class kbLevelComponent;


/// DealAttackInfo_t
template<typename T>
struct DealAttackInfo_t {
	CannonActorComponent * m_pAttacker = nullptr;
	float m_BaseDamage = 1.0f;
	float m_Radius = 0.0f;
	T m_AttackType = (T)0;
};

/// AttackHitInfo_t
struct AttackHitInfo_t {
	kbGameComponent * m_pHitComponent = nullptr;
	bool m_bHit = false;
};



/// CannonLevelComponent
class CannonLevelComponent : public kbLevelComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( CannonLevelComponent, kbLevelComponent );

private:
	int											m_Dummy2;
};

/// CannonGame
class CannonGame : public kbGame, public kbRenderHook {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												CannonGame();
	virtual										~CannonGame();

	CannonCameraComponent *						GetMainCamera() const { return m_pMainCamera; }

	CannonActorComponent *						GetPlayer() const { return m_pPlayerComp; }

protected:

	virtual void								init_internal() override;
	virtual void								play_internal() override;
	virtual void								stop_internal() override;
	virtual void								level_loaded_internal() override;

	virtual void								add_entity_internal( kbGameEntity *const pEntity ) override;
	virtual void								remove_entity_internal( kbGameEntity *const pEntity ) override;


	virtual void								preupdate_internal() override;
	virtual void								postupdate_internal() override;

	virtual kbGameEntity *						CreatePlayer( const int netId, const kbGUID & prefabGUID, const Vec3 & desiredLocation ) override;

	virtual void								HackEditorInit( HWND hwnd, std::vector<class kbEditorEntity *> & editorEntities ) override;
	virtual void								HackEditorUpdate( const float DT, kbCamera *const pCamera ) override;
	virtual void								HackEditorShutdown() override;

protected:

	virtual void								RenderSync() override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) override;

	kbCamera									m_Camera;

	kbTimer										m_GameStartTimer;

	CannonCameraComponent *						m_pMainCamera;
	CannonActorComponent *						m_pPlayerComp;

private:

	void										ProcessInput( const float deltaTimeSec );
};

/// CannonFogComponent
class CannonFogComponent : public kbGameComponent, kbRenderHook {

	KB_DECLARE_COMPONENT( CannonFogComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
protected:

	virtual void								enable_internal( const bool bEnable ) override;
	virtual void								RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) override;

private:

	kbShader *									m_shader;
	float										m_FogStartDist;
	float										m_FogEndDist;
	float										m_FogClamp;
	kbColor										m_FogColor;					
};


extern CannonGame * g_pCannonGame;

inline bool WasAttackJustPressed( const kbInput_t *const pInput = nullptr ) {
	const kbInput_t & input = ( pInput == nullptr )?( g_pInputManager->get_input() ) : ( *pInput );
	return input.WasKeyJustPressed( 'K' ) || input.GamepadButtonStates[12].m_Action == kbInput_t::KA_JustPressed;
}

inline bool WasSpecialAttackPressed( const kbInput_t *const pInput = nullptr ) {
	const kbInput_t & input = ( pInput == nullptr )?( g_pInputManager->get_input() ) : ( *pInput );
	return input.WasKeyJustPressed( 'J' ) || input.LeftTrigger > 0.1f || input.RightTrigger > 0.1f;
}

inline bool WasStartButtonPressed( const kbInput_t *const pInput = nullptr ) {
	const kbInput_t & input = ( pInput == nullptr )?( g_pInputManager->get_input() ) : ( *pInput );
	return input.GamepadButtonStates[4].m_Action == kbInput_t::KA_JustPressed;
}

inline bool WasBackButtonPressed( const kbInput_t *const pInput = nullptr ) {
	const kbInput_t & input = ( pInput == nullptr )?( g_pInputManager->get_input() ) : ( *pInput );
	return input.WasNonCharKeyJustPressed( kbInput_t::Escape ) || input.GamepadButtonStates[5].m_Action == kbInput_t::KA_JustPressed;
}

inline bool WasConfirmationButtonPressed( const kbInput_t *const pInput = nullptr ) {
	if ( WasStartButtonPressed( pInput ) || WasAttackJustPressed( pInput ) ) {
		return true;
	}

	const kbInput_t & input = ( pInput == nullptr )?( g_pInputManager->get_input() ) : ( *pInput );
	if ( input.WasNonCharKeyJustPressed( kbInput_t::Return ) ) {
		return true;
	}

	return false;
}

inline Vec2 GetLeftStick( const kbInput_t *const pInput = nullptr ) {
	const kbInput_t & input = ( pInput == nullptr )?( g_pInputManager->get_input() ) : ( *pInput );
	Vec2 retLeftStick = Vec2::zero;

	if ( input.IsKeyPressedOrDown( 'A' ) ) {
		retLeftStick.x = -1.0f;
	} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
		retLeftStick.x = 1.0f;
	} else {
		retLeftStick.x = input.m_LeftStick.x;
	}

	if ( input.IsKeyPressedOrDown( 'W' ) ) {
		retLeftStick.y = 1.0f;
	} else if ( input.IsKeyPressedOrDown( 'S' ) ) {
		retLeftStick.y = -1.0f;
	} else {
		retLeftStick.y = input.m_LeftStick.y;
	}

	return retLeftStick;
}

inline Vec2 GetPrevLeftStick( const kbInput_t *const pInput = nullptr ) {
	const kbInput_t & input = ( pInput == nullptr )?( g_pInputManager->get_input() ) : ( *pInput );
	Vec2 leftStick = Vec2::zero;

	if ( input.IsKeyPressedOrDown( 'A' ) ) {
		leftStick.x = -1.0f;
	} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
		leftStick.x = 1.0f;
	} else {
		leftStick.x = input.m_PrevLeftStick.x;
	}

	if ( input.IsKeyPressedOrDown( 'W' ) ) {
		leftStick.y = 1.0f;
	} else if ( input.IsKeyPressedOrDown( 'S' ) ) {
		leftStick.y = -1.0f;
	} else {
		leftStick.y = input.m_PrevLeftStick.y;
	}

	return leftStick;
}
