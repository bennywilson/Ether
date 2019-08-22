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
	enum SnolafState_T {
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

class KungFuSnolafComponent : public CannonActorComponent, IStateMachine<KungFuSnolafStateBase<KungFuSnolafState::SnolafState_T>, KungFuSnolafState::SnolafState_T> {
	KB_DECLARE_COMPONENT( KungFuSnolafComponent, CannonActorComponent );

//---------------------------------------------------------------------------------------------------'
public:

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Data
	int											m_Dummy;

	// Game


//---------------------------------------------------------------------------------------------------
	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEvent & animEvent ) override;
};

template<typename T>
class KungFuSnolafStateBase : public CannonBallCharacterState<T> {

//---------------------------------------------------------------------------------------------------'
public:

	KungFuSnolafStateBase( CannonActorComponent *const pPlayerComponent ) : CannonBallCharacterState( pPlayerComponent ) { }

};


#endif