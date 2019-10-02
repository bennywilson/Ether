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


/**
 *	CannonUIWidget
 */
class CannonUIWidget : public kbGameComponent {
public:

	KB_DECLARE_COMPONENT( CannonUIWidget, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	void									SetParent( const kbUIComponent *const pParent );

	void									SetRelativePosition( const kbVec3 & newPos ) { m_RelativePosition = newPos; }
	void									SetRelativeSize( const kbVec3 & newSize ) { m_RelativeSize = newSize; }

	kbVec2i									GetBaseTextureDimensions() const;

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	kbVec3									m_RelativePosition;
	kbVec3									m_RelativeSize;
	std::vector<kbMaterialComponent> 		m_Materials;

	// Runtime
	kbStaticModelComponent *				m_pModel;
	const kbUIComponent *					m_pParent;
	kbGameEntity							m_GameEntity;
};

/**
 *	CannonBallPauseMenuUIComponent
 */
class CannonBallPauseMenuUIComponent : public kbUIComponent {

	KB_DECLARE_COMPONENT( CannonBallPauseMenuUIComponent, kbUIComponent );

//---------------------------------------------------------------------------------------------------
public:
	enum ePauseMenuOptions {
		MasterVolume,
		Brightness,
		VideoQuality,
		BackToGame,
		ExitToMainMenu,
		NumOptions
	};

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	std::vector<CannonUIWidget>				m_Widgets;
	kbVec3									m_WidgetSize;
	kbVec3									m_StartingWidgetAnchorPt;
	float									m_SpaceBetweenWidgets;

	kbGameEntity							m_Entity;
};


#endif