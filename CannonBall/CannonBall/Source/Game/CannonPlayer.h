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

	Vec3									GetTargetFacingDirection() const { return m_TargetFacingDirection; }
	void										SetTargetFacingDirection( const Vec3& targetDir ) { m_TargetFacingDirection = targetDir; }

	bool										IsPlayingAnim( const kbString animName ) const;
	void										PlayAnimation( const kbString animName, const float animBlendInLen, const bool bRestartIfAlreadyPlaying = false, const kbString nextAnim = kbString::EmptyString, const float nextAnimBlendInLen = 0.0f );
	bool										HasFinishedAnim( const kbString animName = kbString::EmptyString ) const;
	void										SetAnimationTimeScaleMultiplier( const kbString animName, const float multiplier );

	void										ApplyAnimSmear( const Vec3 smearVec, const float durationSec );

	void										SetOverrideFXMaskParameters( const Vec4& fxParams );

	void										PlayAttackVO( const int pref );

	bool										IsPlayer() const { return m_bIsPlayer; }
	virtual bool						IsDead() const { return m_Health <= 0.0f; }

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	// Data
	float										m_MaxRunSpeed;
	float										m_MaxRotateSpeed;
	float										m_Health;

	std::vector<kbSoundData>	m_AttackVO;

	// Game
	std::vector<kbSkeletalModelComponent *>		m_SkelModelsList;
	Vec3									m_TargetFacingDirection;

	float										m_AnimSmearDuration;
	Vec4									m_AnimSmearVec;
	float										m_AnimSmearStartTime;

	Vec4									m_OverridenFXMaskParams;

	float										m_LastVOTime;

	bool										m_bIsPlayer;

//---------------------------------------------------------------------------------------------------
	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEventInfo_t& animEvent ) override { }
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
	Vec2										GetAmplitude() const { return Vec2( m_AmplitudeX, m_AmplitudeY ); }
	Vec2										GetFrequency() const { return Vec2( m_FrequencyX, m_FrequencyY ); }

	void										SetEnable_Internal( const bool bEnable ) override;
	void										Update_Internal( const float deltaTime ) override;

	float										m_Duration;
	float										m_AmplitudeX;
	float										m_AmplitudeY;

	float										m_FrequencyX;
	float										m_FrequencyY;

private:

	float										m_ActivationDelaySeconds;
	float										m_ShakeStartTime;
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

	void										SetTarget( const kbGameEntity *const pTarget, const float blendRate );
	void										SetPositionOffset( const Vec3 & posOffset, const float blendRate );
	void										SetLookAtOffset( const Vec3 & lookAtOffset, const float blendRate );

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	float										m_NearPlane;
	float										m_FarPlane;
	Vec3										m_PositionOffset;
	Vec3										m_LookAtOffset;

	// Game
	ECameraMoveMode								m_MoveMode;
	const kbGameEntity *						m_pTarget;
	float										m_SwitchTargetBlendSpeed;
	float										m_SwitchTargetCurT;
	Vec3										m_SwitchTargetStartPos;

	float										m_SwitchPosOffsetBlendSpeed;
	float										m_SwitchPosOffsetCurT;
	Vec3										m_PosOffsetTarget;

	float										m_SwitchLookAtOffsetBlendSpeed;
	float										m_SwitchLookAtOffsetCurT;
	Vec3										m_LookAtOffsetTarget;

	float										m_CameraShakeStartTime;
	Vec2										m_CameraShakeStartingOffset;
	float										m_CameraShakeDuration;
	Vec2										m_CameraShakeAmplitude;
	Vec2										m_CameraShakeFrequency;
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