//==============================================================================
// CannonPlayer.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KBCANNONPLAYER_H_
#define _KBCANNONPLAYER_H_

/**
 *	CannonActorComponent
 */
class CannonActorComponent : public kbActorComponent, IAnimEventListener {

	KB_DECLARE_COMPONENT( CannonActorComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:

	float										GetMaxRunSpeed() const { return m_MaxRunSpeed; }
	float										GetMaxRotateSpeed() const { return m_MaxRotateSpeed; }

	kbVec3										GetTargetFacingDirection() const { return m_TargetFacingDirection; }
	void										SetTargetFacingDirection( const kbVec3 & targetDir ) { m_TargetFacingDirection = targetDir; }

	bool										IsPlayingAnim( const kbString animName ) const;
	void										PlayAnimation( const kbString animName, const float animBlendInLen, const bool bRestartIfAlreadyPlaying = false, const kbString nextAnim = kbString::EmptyString, const float nextAnimBlendInLen = 0.0f );
	bool										HasFinishedAnim( const kbString animName = kbString::EmptyString ) const;
	void										SetAnimationTimeScaleMultiplier( const kbString animName, const float multiplier );

	void										ApplyAnimSmear( const kbVec3 smearVec, const float durationSec );

	void										PlayAttackVO( const int pref );

	bool										IsPlayer() const { return m_bIsPlayer; }
	virtual bool								IsDead() const { return m_Health <= 0.0f; }

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	// Data
	float										m_MaxRunSpeed;
	float										m_MaxRotateSpeed;
	float										m_Health;

	std::vector<kbSoundData>					m_AttackVO;

	// Game
	std::vector<kbSkeletalModelComponent *>		m_SkelModelsList;
	kbVec3										m_TargetFacingDirection;

	float										m_AnimSmearDuration;
	kbVec4										m_AnimSmearVec;
	float										m_AnimSmearStartTime;

	float										m_LastVOTime;

	bool										m_bIsPlayer;

//---------------------------------------------------------------------------------------------------
	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEventInfo_t & animEvent ) override { }
};

/**
 *	ECameraMoveMode
 */
enum ECameraMoveMode {
	MoveMode_None,
	MoveMode_Follow,
};

/**
 *	CannonCameraShakeComponent
 */
class CannonCameraShakeComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( CannonCameraShakeComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:

	float										GetDuration() const { return m_Duration; }
	kbVec2										GetAmplitude() const { return kbVec2( m_AmplitudeX, m_AmplitudeY ); }
	kbVec2										GetFrequency() const { return kbVec2( m_FrequencyX, m_FrequencyY ); }

	void										SetEnable_Internal( const bool bEnable );

private:

	float										m_Duration;
	float										m_AmplitudeX;
	float										m_AmplitudeY;

	float										m_FrequencyX;
	float										m_FrequencyY;

	bool										m_bActivateOnEnable;
};

/**
 *	CannonCameraComponent
 */
class CannonCameraComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( CannonCameraComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:
	
	void										StartCameraShake( const CannonCameraShakeComponent *const pCameraShakeComponent );

	void										SetTarget( const kbGameEntity *const pTarget );

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	float										m_NearPlane;
	float										m_FarPlane;
	kbVec3										m_PositionOffset;
	kbVec3										m_LookAtOffset;

	// Game
	ECameraMoveMode								m_MoveMode;
	const kbGameEntity *						m_pTarget;

	float										m_CameraShakeStartTime;
	kbVec2										m_CameraShakeStartingOffset;
	float										m_CameraShakeDuration;
	kbVec2										m_CameraShakeAmplitude;
	kbVec2										m_CameraShakeFrequency;
};

template<typename T>
class CannonBallCharacterState : public StateMachineNode<T> {

//---------------------------------------------------------------------------------------------------'
public:

	CannonBallCharacterState( CannonActorComponent *const pActorComponent ) : m_pActorComponent( pActorComponent ) { }

protected:

	CannonActorComponent * m_pActorComponent;

};

#endif