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

class KungFuSheepComponent : public CannonPlayerComponent, IStateMachine<KungFuSheepStateBase<KungFuSheepState::SheepStates_t>, KungFuSheepState::SheepStates_t> {
	KB_DECLARE_COMPONENT( KungFuSheepComponent, CannonPlayerComponent );

	friend class KungFuSheepStateBase<KungFuSheepState::SheepStates_t>;

//---------------------------------------------------------------------------------------------------'
public:

	virtual void								HandleInput(const kbInput_t & input, const float DT) override;

	// IAnimEventListener
	virtual void								OnAnimEvent(const kbAnimEvent & animEvent) override;

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	void										PlayAnimation() {

	}

	// Data
	kbGameEntityPtr								m_CannonBallImpactFX;
	std::vector<kbSoundData>					m_CannonBallVO;
	std::vector<kbSoundData>					m_CannonBallImpactSound;

	float										m_JumpSmearMagnitude;
	float										m_DropSmearMagnitude;

	// Game
	float										m_AnimSmearStartTime;
	float										m_AnimSmearDuration;
	kbVec4										m_AnimSmearVec;
};

template<typename T>
class KungFuSheepStateBase : public StateMachineNode<T> {

	template<KungFuSheepState::SheepStates_t>
	class KungFuSheepComponent;

//---------------------------------------------------------------------------------------------------'
public:

	
	KungFuSheepStateBase( const KungFuSheepComponent *const pPlayerComponent ) : m_pPlayerComponent( pPlayerComponent ) { }

	void PlayAnimation() {
		kbErrorCheck( m_pPlayerComponent != null, "KungFuSheepStateBase::PlayAnimation() - NULL player component" );
		m_pPlayerComponent->PlayAnimation();
	}

private:

	//template<KungFuSheepState::SheepStates_t T>
	KungFuSheepComponent * m_pPlayerComponent;
};


#endif