//==============================================================================
// KungFuLevelComponent.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KUNGFULEVELCOMPONENT_H_
#define _KUNGFULEVELCOMPONENT_H_



namespace KungFuGame {

	enum eKungFuGame_State {
		MainMenu = 0,
		Intro,
		Gameplay,
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

	AttackHitInfo_t								DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo );

	void										SpawnEnemy();

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	kbGameEntityPtr								m_SnolafPrefab;
	kbGameEntityPtr								m_SheepPrefab;
};


#endif