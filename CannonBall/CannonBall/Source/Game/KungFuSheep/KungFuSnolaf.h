//==============================================================================
// KungFuSnolaf.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KUNGFUSNOLAF_H_
#define _KUNGFUSNOLAF_H_


/**
 *	KungFuSheepState
 */
namespace KungFuSnolafState {
	enum SnolafState_t {
		Idle = 0,
		Run,
		Hug,
		Dead,
		WatchCannonBall,
		NumStates
	};
}

/**
 *	KungFuSnolafComponent
 */

template<typename T>
class KungFuSnolafStateBase;

class KungFuSnolafComponent : public CannonActorComponent, IStateMachine<KungFuSnolafStateBase<KungFuSnolafState::SnolafState_t>, KungFuSnolafState::SnolafState_t> {
	KB_DECLARE_COMPONENT( KungFuSnolafComponent, CannonActorComponent );

//---------------------------------------------------------------------------------------------------'
public:

	void										EnableSmallLoveHearts( const bool bEnable );
	void										EnableLargeLoveHearts( const bool bEnable );

	void										TakeDamage( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo );

	void										DoPoofDeath();
	void										SpawnAndFlingDecapHead();
	void										SpawnAndFlingTopAndBottomHalf();
	void										SpawnSplash();

	KungFuSnolafState::SnolafState_t			GetState() const { return m_CurrentState; }
	const DealAttackInfo_t<KungFuGame::eAttackType>	& GetLastAttackInfo() const { return m_LastAttackInfo; }

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Data
	kbGameEntityPtr								m_FootStepImpactFX;
	kbGameEntityPtr								m_PoofDeathFX;
	kbGameEntityPtr								m_DecapitatedHead;
	kbGameEntityPtr								m_TopHalfOfBody;
	kbGameEntityPtr								m_BottomHalfOfBody;
	kbGameEntityPtr								m_SplashFX;

	// Game
	kbParticleComponent *						m_pSmallLoveHearts;
	kbParticleComponent *						m_pLargeLoveHearts;

	DealAttackInfo_t<KungFuGame::eAttackType>	m_LastAttackInfo;

//---------------------------------------------------------------------------------------------------
	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEventInfo_t & animEvent ) override;
};

template<typename T>
class KungFuSnolafStateBase : public CannonBallCharacterState<T> {

//---------------------------------------------------------------------------------------------------'
public:

	KungFuSnolafStateBase( CannonActorComponent *const pPlayerComponent ) : CannonBallCharacterState( pPlayerComponent ) { }

protected:

	KungFuSnolafComponent *	GetSnolaf() const { return (KungFuSnolafComponent*)m_pActorComponent; }
	CannonActorComponent *	GetTarget() const { return g_pCannonGame->GetPlayer(); }

	float GetDistanceToTarget() {

		if ( GetTarget() == nullptr ) {
			return -1.0f;
		}

		const kbVec3 targetPos = GetTarget()->GetOwnerPosition();
		const kbVec3 snolafPos = m_pActorComponent->GetOwnerPosition();

		if ( targetPos.Compare( snolafPos ) == true ) {
			return 0.0f;
		}

		return ( targetPos - snolafPos ).Length();
	}

	void RotateTowardTarget() {
		if ( GetTarget() == nullptr ) {
			return;
		}

		const kbVec3 targetPos = GetTarget()->GetOwnerPosition();
		const kbVec3 snolafPos = m_pActorComponent->GetOwnerPosition();

		m_pActorComponent->SetTargetFacingDirection( ( snolafPos - targetPos ).Normalized() );
	}

	bool IsTargetOnLeft() const {
		if ( GetTarget() == nullptr ) {
			return false;
		}

		const kbVec3 targetPos = GetTarget()->GetOwnerPosition();
		const kbVec3 snolafPos = m_pActorComponent->GetOwnerPosition();
		return ( targetPos.z > snolafPos.z );
	}
};


#endif