//==============================================================================
// KungFuLevelComponent.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KUNGFULEVELCOMPONENT_H_
#define _KUNGFULEVELCOMPONENT_H_

#include "UI/CannonUI.h"
#include "kbLevelDirector.h"
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
		DebugDeath
	};
};


/**
 *	KungFuLevelComponent
 */
class KungFuLevelComponent : public CannonLevelComponent {

	KB_DECLARE_COMPONENT( KungFuLevelComponent, CannonLevelComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:


	void										SpawnEnemy( const bool bSpawnLeft, const int waveSize );
	class KungFuSheepComponent *				SpawnSheep();
	KungFuSheepComponent *						GetSheep() const { return m_pSheep; }

	AttackHitInfo_t								DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo );
	void										UpdateSheepHealthBar( const float healthVal );
	void										UpdateCannonBallMeter( const float value, const bool bActivated );

	void										DoWaterDropletScreenFX();
	void										DoSplashSound();

	void										SetPlayLevelMusic( const bool bPlay );

	float										GetDistancePlayerHasTraveled();

	void										ReturnSnolafToPool( class KungFuSnolafComponent *const pSnolaf );

	static KungFuLevelComponent *				Get() { return s_Inst; }

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	virtual void								UpdateDebugAndCheats() override;

	// Data
	float										m_LevelLength;

	std::vector<kbSoundData>					m_LevelMusic;

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

	std::vector<kbGameEntity*>					m_SnolafPool;
	KungFuSheepComponent *						m_pSheep;

	static KungFuLevelComponent *				s_Inst;
};

/**
 *	KungFuGame_BaseState
 */
class KungFuGame_BaseState : public StateMachineNode<KungFuGame::eKungFuGame_State> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_BaseState( KungFuLevelComponent *const pLevelComponent ) : m_pLevelComponent( pLevelComponent ) { }

	virtual AttackHitInfo_t DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo ) { AttackHitInfo_t ret; return ret; }

protected:
	KungFuLevelComponent * m_pLevelComponent;
};


/**
 *	KungFuSheepDirector
 */
class KungFuSheepDirector : public kbLevelDirector<KungFuGame_BaseState, KungFuGame::eKungFuGame_State>, public ISingleton<KungFuSheepDirector>  {

//---------------------------------------------------------------------------------------------------
public:

												KungFuSheepDirector();
	virtual										~KungFuSheepDirector();

	virtual void								UpdateStateMachine() override;

	virtual void								StateChangeCallback( const KungFuGame::eKungFuGame_State previousState, const KungFuGame::eKungFuGame_State nextState );

	AttackHitInfo_t								DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> attackInfo );

private:

	CannonHealthBarUIComponent *				m_pHealthBarUI;
	CannonBallUIComponent *						m_pCannonBallUI;

};

#endif