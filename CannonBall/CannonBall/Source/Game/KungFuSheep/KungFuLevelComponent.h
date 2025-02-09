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
		Outro,
		NumStates
	 };

	enum eAttackType {
		Punch_Kick,
		Hug,
		Shake,
		Cannonball,
		DebugDeath
	};

	// TODO - Optimize - Move static kbStrings here

	// Camera
	const Vec3 kCameraStartPos = Vec3( 66.170448f, -49.412132f, -391.558990f );
	const Quat4 kCameraRotation = Quat4( -0.023375f, -0.706720f, 0.023375f, 0.706720f );
	const float kPrehugLengthSec = 0.067f;
	const float kDistToHugSheep = 0.75f;
	const float kDistToHugSnolaf = 0.5f;
	const float kDistToChase = kDistToHugSheep + 0.05f;
	const float kMeterFillPerSnolafKill = 0.125f;

	// Bonus
	const float kShakeNBakeCannonBallBonus = 0.125f;

	// Sheep Attack
	const float kShakeNBakeRadius = 3.0f;
	const float kSheepAttackDist = 0.75f;
	const int kMaxNumRequiredShakeBakeTurns = 4;
	const float kSheepPunchAnimTimeScale = 15.0f;
	const float kSheepKickAnimTimeScale = 8.9999f;

	// Snolaf Attack
	const int kMaxHuggerDamageMultiplier = 4;

	// Wave
	const int kSnolafPoolSize = 150;
	const int kMaxSnolafWaveSize = 35;
	const float kTimeBetweenSnolafWaves = 6.0f;
	const float kDistBetweenSnolafs = 1.0f;
	const float kSubWaveDirChangeInitialOffset = 2.f;
	const float kSubWaveDirChangePerSnolafOffset = 0.25f;

	// Movement	
	const Vec3 kSheepStartPos( 77.10445f, -52.6362f, -398.0f );
	const Quat4 kSheepStartRot( 0.0f, 1.0f, 0.0f, 0.0f );
	
	const float kLevelLength = 165.0f;

	// Cinema
	const float kOutroStartZ = -297.0f;
	const Vec3 kFoxPos = Vec3( 77.10445f, -52.6362f, -278.628601f );
	const Vec3 kTreyTonOffScreenPos = Vec3( 76.992683f, -52.599991f, -270.628479f );
	const Vec3 kTreyTonJump1StartPos = Vec3( 76.992683f, -52.599991f, -272.828479f );
	const Vec3 kTreyTonJump2StartPos = Vec3( 76.992683f, -52.599991f, -280.828479f );
	const Vec3 kFoxFinalPos = Vec3( 77.10445f, -52.6362f, -281.45f );
	const Vec3 kSheepFinalPos = kFoxFinalPos + Vec3( 0.0f, 0.0f, -2.0f );
	const Vec3 kFinalSnolafEntryPos = KungFuGame::kFoxFinalPos + Vec3( 0.0f, 0.0f, 8.25f );
};


/**
 * 	KungFuLevelComponent
 */
class KungFuLevelComponent : public CannonLevelComponent {

	KB_DECLARE_COMPONENT( KungFuLevelComponent, CannonLevelComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	void										SpawnEnemy( const bool bSpawnLeft, const int waveSize );
	class KungFuSheepComponent* 				SpawnSheep();
	KungFuSheepComponent* 						GetSheep() const { return m_pSheep; }
	CannonActorComponent* 						Get3000Ton() const { return m_p3000Ton; }
	CannonActorComponent* 						GetFox() const { return m_pFox; }

	void										UpdateSheepHealthBar( const float healthVal );
	void										UpdateCannonBallMeter( const float value, const bool bActivated );

	void										DoWaterDropletScreenFX();
	void										DoSplashSound();
	kbGameEntityPtr					GetPresent( const int idx ) { return m_PresentsEnt[idx]; }
	void										DoBreakBridgeEffect( const bool bBreakIt );

	void										SetPlayLevelMusic( const int idx, const bool bPlay );

	void										ShowBLM( const bool bShow );
	void										ShowCredits( const bool bShowCredits );
	
	void										ShowHealthBar( const bool bShow );

	float										GetPlayerTravelDistance();

	class KungFuSnolafComponent* 				GetSnolafFromPool();
	void										ReturnSnolafToPool( class KungFuSnolafComponent* const pSnolaf );
	void										RemoveSheep();

	static KungFuLevelComponent* 				Get() { return s_Inst; }

protected:

	virtual void								enable_internal( const bool bEnable ) override;
	virtual void								update_internal( const float DeltaTime ) override;

private:

	virtual void								UpdateDebugAndCheats() override;

	// Data
	float										m_LevelLength;

	std::vector<kbSoundData>					m_LevelMusic;

	kbGameEntityPtr								m_SnolafPrefab;
	kbGameEntityPtr								m_SheepPrefab;

	kbGameEntityPtr								m_WaterDropletScreenFX;
	std::vector<kbSoundData>					m_WaterSplashSound;

	kbTexture*									m_pBasePortraitTexture;
	kbTexture*									m_pHuggedPortraitTexture;
	kbTexture*									m_pDeadPortriatTexture;

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

	CannonHealthBarUIComponent* 				m_pHealthBarUI;
	CannonBallUIComponent* 						m_pCannonBallUI;

	kbGameEntityPtr								m_BLM;
	kbGameEntityPtr								m_Credits;

	std::vector<kbGameEntity*>					m_SnolafPool;
	KungFuSnolafComponent* 						m_EndSnolafs[2];
	KungFuSheepComponent* 						m_pSheep;
	CannonActorComponent* 						m_p3000Ton;
	CannonActorComponent* 						m_pFox;

	kbGameEntityPtr								m_PresentsEnt[2];
	kbGameEntityPtr								m_BridgeBreakDecal;
	kbGameEntityPtr								m_BridgeExplosionFX;

	static KungFuLevelComponent* 				s_Inst;
};

/**
 * 	KungFuGame_BaseState
 */
class KungFuGame_BaseState : public StateMachineNode<KungFuGame::eKungFuGame_State> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_BaseState( KungFuLevelComponent* const pLevelComponent ) : m_pLevelComponent( pLevelComponent ) { }

	virtual AttackHitInfo_t DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo ) { AttackHitInfo_t ret; return ret; }

	virtual void WidgetEventCB( kbUIWidgetComponent* const pWidget, const kbInput_t* const pInput ) { }

protected:
	KungFuLevelComponent* m_pLevelComponent;
};


/**
 * 	KungFuSheepDirector
 */
class KungFuSheepDirector : public kbLevelDirector<KungFuGame_BaseState, KungFuGame::eKungFuGame_State>, public ISingleton<KungFuSheepDirector>, public IUIWidgetListener {

//---------------------------------------------------------------------------------------------------
public:

												KungFuSheepDirector();
	virtual										~KungFuSheepDirector();

	virtual void								UpdateStateMachine() override;

	virtual void								StateChangeCB( const KungFuGame::eKungFuGame_State previousState, const KungFuGame::eKungFuGame_State nextState );
	virtual void								WidgetEventCB( kbUIWidgetComponent* const pWidget, const kbInput_t* const pInput );

	AttackHitInfo_t								DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> attackInfo );

	CannonBallPauseMenuUIComponent* 			GetPauseMenu() const { return m_pPauseMenuUI; }

	int											GetNumHuggers() const { return m_NumHuggers; }
	int											GetNumPrehuggers() const { return m_NumPrehuggers; }
	int											GetNumHuggersAndPrehuggers() const { return m_NumHuggers + m_NumPrehuggers; }

protected:

	virtual void								InitializeStateMachine_Internal();
	virtual void								ShutdownStateMachine_Internal();

private:

	void										CollectUIElements();

	CannonHealthBarUIComponent* 				m_pHealthBarUI;
	CannonBallUIComponent* 						m_pCannonBallUI;
	CannonBallMainMenuComponent* 				m_pMainMenuUI;
	CannonBallPauseMenuUIComponent* 			m_pPauseMenuUI;

	int											m_NumHuggers;
	int											m_NumPrehuggers;
};

#endif