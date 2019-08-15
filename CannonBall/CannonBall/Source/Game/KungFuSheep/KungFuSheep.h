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

	kbVec3										GetTargetFacingDirection() const { return m_TargetFacingDirection; }
	void										SetTargetFacingDirection( const kbVec3 & targetDir ) { m_TargetFacingDirection = targetDir; }

	bool										IsPlayingAnim( const kbString animName ) const;

	void										PlayAnimation( const kbString animName, const float animBlendInLen, const bool bRestartIfAlreadyPlaying = false, const kbString nextAnim = kbString::EmptyString, const float nextAnimBlendInLen = 0.0f );
	bool										HasFinishedAnim() const;

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
	float										m_AnimSmearStartTime;
	float										m_AnimSmearDuration;
	kbVec4										m_AnimSmearVec;
	kbVec3										m_TargetFacingDirection;

	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEvent & animEvent ) override;
};

template<typename T>
class KungFuSheepStateBase : public StateMachineNode<T> {

//---------------------------------------------------------------------------------------------------'
public:

	KungFuSheepStateBase( KungFuSheepComponent *const pPlayerComponent ) : m_pPlayerComponent( pPlayerComponent ) { }

protected:

	void PlayAnimation( const kbString animationName, const float BlendInLength, const bool bRestartIfAlreadyPlaying = false, const kbString nextAnim = kbString::EmptyString, const float nextAnimBlendInLen = 0.0f ) {
		kbErrorCheck( m_pPlayerComponent != nullptr, "KungFuSheepStateBase::PlayAnimation() - NULL player component" );
		m_pPlayerComponent->PlayAnimation( animationName, BlendInLength, bRestartIfAlreadyPlaying, nextAnim, nextAnimBlendInLen );
	}
	
	KungFuSheepComponent * m_pPlayerComponent;

};


#endif