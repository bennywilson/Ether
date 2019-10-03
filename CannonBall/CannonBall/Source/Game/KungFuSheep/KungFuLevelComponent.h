//==============================================================================
// KungFuLevelComponent.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KUNGFULEVELCOMPONENT_H_
#define _KUNGFULEVELCOMPONENT_H_

#include "UI/CannonUI.h"

namespace KungFuGame {

	enum eKungFuGame_State {
		MainMenu = 0,
		Intro,
		Gameplay,
		PlayerDead,
		Paused,
		NumStates
	 };

	enum eAttackType {
		Punch_Kick,
		Hug,
		Shake,
		Cannonball,
	};
};




/**
 *	KungFuLevelComponent
 */
class KungFuLevelComponent : public CannonLevelComponent {

	KB_DECLARE_COMPONENT( KungFuLevelComponent, CannonLevelComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	void										SpawnEnemy();
	void										SpawnSheep();

	AttackHitInfo_t								DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo );
	void										UpdateSheepHealthBar( const float healthVal );
	void										UpdateCannonBallMeter( const float value, const bool bActivated );

	void										DoWaterDropletScreenFX();
	void										DoSplashSound();

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Data
	kbGameEntityPtr								m_SnolafPrefab;
	kbGameEntityPtr								m_SheepPrefab;

	kbGameEntityPtr								m_WaterDropletScreenFX;
	std::vector<kbSoundData>					m_WaterSplashSound;

	// Runtime
	float										m_WaterDropletFXStartTime;

	const static int NumWaterSplashes = 2;
	struct waterSplashFX_t {
		kbGameEntityPtr							m_Entity;
		float									m_Duration;
		float									m_InitialDelay;
	};
	waterSplashFX_t								m_WaterSplashFXInst[NumWaterSplashes];

	float										m_LastWaterSplashSoundTime;

	CannonHealthBarUIComponent *				m_pHealthBarUI;
	CannonBallUIComponent *						m_pCannonBallUI;
};


#endif