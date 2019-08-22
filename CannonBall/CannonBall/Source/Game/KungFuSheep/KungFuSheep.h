//==============================================================================
// KungFuSheep.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KBKUNGFUSHEEP_H_
#define _KBKUNGFUSHEEP_H_


/**
 *	KungFuSheepStateBase
 */
namespace KungFuSheepState {
	enum SheepStates_t {
		Idle = 0,
		Run,
		Attack,
		Hugged,
		Dead,
		CannonBall,
		NumStates
	};
}

/**
 *	KungFuSheepComponent
 */

template<typename T>
class KungFuSheepStateBase;

class KungFuSheepComponent : public CannonActorComponent, IStateMachine<KungFuSheepStateBase<KungFuSheepState::SheepStates_t>, KungFuSheepState::SheepStates_t> {
	KB_DECLARE_COMPONENT( KungFuSheepComponent, CannonActorComponent );

	friend class KungFuSheepStateBase<KungFuSheepState::SheepStates_t>;

//---------------------------------------------------------------------------------------------------'
public:



protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Data
	kbGameEntityPtr								m_CannonBallImpactFX;
	std::vector<kbSoundData>					m_CannonBallVO;
	std::vector<kbSoundData>					m_CannonBallImpactSound;

	float										m_JumpSmearMagnitude;
	float										m_DropSmearMagnitude;

	// Game

//---------------------------------------------------------------------------------------------------

	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEvent & animEvent ) override;
};

template<typename T>
class KungFuSheepStateBase : public CannonBallCharacterState<T> {

//---------------------------------------------------------------------------------------------------'
public:

	KungFuSheepStateBase( CannonActorComponent *const pPlayerComponent ) : CannonBallCharacterState( pPlayerComponent ) { }
};


#endif