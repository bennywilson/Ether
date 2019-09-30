//==============================================================================
// CannonUI.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KBCANNONUI_H_
#define _KBCANNONUI_H_

/**
 *	CannonHealthBarUIComponent
 */
class CannonHealthBarUIComponent : public kbUIComponent {

	KB_DECLARE_COMPONENT( CannonHealthBarUIComponent, kbUIComponent );

//---------------------------------------------------------------------------------------------------
public:

	void									SetTargetHealth( const float newHealth );

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	float									m_HealthBarWarningFlashThreshold;
	float									m_HealthBarWarningFlashSpeed;

	// Runtime
	float									m_TargetNormalizedHealth;
	float									m_CurrentNormalizedHealth;
	float									m_StartFlashTime;
};

/**
 *	CannonBallUIComponent
 */
class CannonBallUIComponent : public kbUIComponent {

	KB_DECLARE_COMPONENT( CannonBallUIComponent, kbUIComponent );

//---------------------------------------------------------------------------------------------------
public:

	void									SetFill( const float newHealth );

	void									CannonBallActivatedCB();

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	kbVec3									m_SparkRelativePosition;
	kbVec3									m_SparkRelativeSize;

	kbVec3									m_BoomRelativePosition;
	kbVec3									m_BoomRelativeSize;

	kbVec3									m_SmokeRelativePosition;
	kbVec3									m_SmokeRelativeSize;

	// Runtime
	kbStaticModelComponent *				m_pSparkModel;
	kbStaticModelComponent *				m_pBoomModel;
	kbStaticModelComponent *				m_pSmokeModel;

	float									m_CurrentFill;
	float									m_TargetFill;

	float									m_NextSparkAnimUpdateTime;
	float									m_CannonBallActivatedStartTime;
	float									m_NextSmokeCloudUpdateTime;
};


#endif