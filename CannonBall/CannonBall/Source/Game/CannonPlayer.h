//==============================================================================
// CannonPlayer.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KBCANNONPLAYER_H_
#define _KBCANNONPLAYER_H_

/**
 *	CannonPlayerComponent
 */
class CannonPlayerComponent : public kbActorComponent, IAnimEventListener {

	KB_DECLARE_COMPONENT( CannonPlayerComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual void								HandleInput( const kbInput_t & input, const float DT ) { }

	// IAnimEventListener
	virtual void								OnAnimEvent( const kbAnimEvent & animEvent ) { }

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	// Data
	float										m_MaxRunSpeed;
	float										m_MaxRotateSpeed;

	// Game
	std::vector<kbSkeletalModelComponent *>		m_SkelModelsList;
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

private:
	float										m_Duration;
	float										m_AmplitudeX;
	float										m_FrequencyX;

	float										m_AmplitudeY;
	float										m_FrequencyY;
};

/**
 *	CannonCameraComponent
 */
class CannonCameraComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( CannonCameraComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:
	
	void										StartCameraShake( const CannonCameraShakeComponent *const pCameraShakeComponent );

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	void										FindTarget();

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


#endif