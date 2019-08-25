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


protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Data
	int											m_Dummy;

	// Game
	kbParticleComponent *						m_pSmallLoveHearts;
	kbParticleComponent *						m_pLargeLoveHearts;

//---------------------------------------------------------------------------------------------------
	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEvent & animEvent ) override;
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