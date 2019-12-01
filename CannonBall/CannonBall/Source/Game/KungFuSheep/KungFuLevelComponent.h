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

	const float kPrehugLengthSec = 0.067f;
	const float kDistToHugSheep = 0.75f;
	const float kDistToHugSnolaf = 0.5f;
	const float kDistToChase = kDistToHugSheep + 0.05f;
	const float kTimeUntilRequeueReady = 0.0f;
	const float kMeterFillPerSnolafKill = 0.125f;

	// Bonus
	const float kShakeNBakeCannonBallBonus = 0.125f;

	// Sheep Attack
	const float kShakeNBakeRadius = 3.0f;
	const float kSheepAttackDist = 0.75f;
	const int kMaxNumRequiredShakeBakeTurns = 4;
	const float kSheepAttackAnimTimeScale = 15.0f;

	// Wave
	const int kSnolafPoolSize = 100;
	const int kMaxSnolafWaveSize = 20;
	const float kTimeBetweenSnolafWaves = 6.0f;
	const float kDistBetweenSnolafs = 1.0f;
	const float kSubWaveDirChangeOffsetMult = 3.f;

	// Movement
	const kbVec3 kSheepStartPos( 77.10445f, -52.6362f, -396.559f );
	const kbQuat kSheepStartRot( 0.0f, 1.0f, 0.0f, 0.0f );
	
	const float kLevelLength = 165.0f;
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

	void										UpdateSheepHealthBar( const float healthVal );
	void										UpdateCannonBallMeter( const float value, const bool bActivated );

	void										DoWaterDropletScreenFX();
	void										DoSplashSound();

	void										SetPlayLevelMusic( const int idx, const bool bPlay );

	float										GetDistancePlayerHasTraveled();

	void										ReturnSnolafToPool( class KungFuSnolafComponent *const pSnolaf );
	void										RemoveSheep();

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
	KungFuSnolafComponent *						m_EndSnolafs[2];
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

	virtual void WidgetEventCB( kbUIWidget *const pWidget, const kbInput_t *const pInput ) { }

protected:
	KungFuLevelComponent * m_pLevelComponent;
};


/**
 *	KungFuSheepDirector
 */
class KungFuSheepDirector : public kbLevelDirector<KungFuGame_BaseState, KungFuGame::eKungFuGame_State>, public ISingleton<KungFuSheepDirector>, public IUIWidgetListener {

//---------------------------------------------------------------------------------------------------
public:

												KungFuSheepDirector();
	virtual										~KungFuSheepDirector();

	virtual void								UpdateStateMachine() override;

	virtual void								StateChangeCB( const KungFuGame::eKungFuGame_State previousState, const KungFuGame::eKungFuGame_State nextState );
	virtual void								WidgetEventCB( kbUIWidget *const pWidget, const kbInput_t *const pInput );

	AttackHitInfo_t								DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> attackInfo );

	CannonBallPauseMenuUIComponent *			GetPauseMenu() const { return m_pPauseMenuUI; }

	int											GetNumHuggers() const { return m_NumHuggers; }
	int											GetNumPrehuggers() const { return m_NumPrehuggers; }
	int											GetNumHuggersAndPrehuggers() const { return m_NumHuggers + m_NumPrehuggers; }
protected:

	virtual void								InitializeStateMachine_Internal();
	virtual void								ShutdownStateMachine_Internal();

private:

	CannonHealthBarUIComponent *				m_pHealthBarUI;
	CannonBallUIComponent *						m_pCannonBallUI;
	CannonBallMainMenuComponent *				m_pMainMenuUI;
	CannonBallPauseMenuUIComponent *			m_pPauseMenuUI;

	int											m_NumHuggers;
	int											m_NumPrehuggers;
};

#endif