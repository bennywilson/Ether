//==============================================================================
// KungFuLevelComponent.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KUNGFULEVELCOMPONENT_H_
#define _KUNGFULEVELCOMPONENT_H_


/**
 *	KungFuLevelComponent
 */
class KungFuLevelComponent : public CannonLevelComponent {

	KB_DECLARE_COMPONENT( KungFuLevelComponent, CannonLevelComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	struct AttackInfo_t {
		AttackInfo_t() : m_pHitComponent( nullptr ), m_bHit( false ) { }
		kbGameComponent * m_pHitComponent;
		bool m_bHit;
	};

	AttackInfo_t								PerformAttack( CannonActorComponent *const pAttacker );

	void										SpawnEnemy();

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	kbGameEntityPtr								m_SnolafPrefab;
	kbGameEntityPtr								m_SheepPrefab;
};


#endif