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
class CannonPlayerComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( CannonPlayerComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
public:
	void										HandleInput( const kbInput_t & input, const float DT );

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:
	int											m_Dummy;

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
 *	CannonCameraComponent
 */
class CannonCameraComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( CannonCameraComponent, kbActorComponent );

//---------------------------------------------------------------------------------------------------
protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	void										FindTarget();

private:

	float										m_NearPlane;
	float										m_FarPlane;

	ECameraMoveMode								m_MoveMode;
	kbVec3										m_Offset;
	const kbGameEntity *						m_pTarget;
};

#endif