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

	void									SetTargetHealth( const float meterFill );

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

	KB_DECLARE_COMPONENT( CannonUIWidget, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual void							RecalculateOld( const kbUIComponent *const pParent );
	virtual void							Recalculate( const CannonUIWidget *const pParent );

	void									SetRelativePosition( const kbVec3 & newPos );
	void									SetRelativeSize( const kbVec3 & newSize );

	const kbVec3 &							GetRelativePosition() const { return m_RelativePosition; }
	const kbVec3 &							GetRelativeSize() const { return m_RelativeSize; }

	const kbVec3 &							GetAbsolutePosition() const { return m_AbsolutePosition; }
	const kbVec3 &							GetAbsoluteSize() const { return m_AbsoluteSize; }

	const kbVec3 &							GetStartingPosition() const { return m_StartingPosition; }
	const kbVec3 &							GetStartingSize() const { return m_StartingSize; }

	kbVec2i									GetBaseTextureDimensions() const;

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;


	// Editor
protected:

	std::vector<kbMaterialComponent> 		m_Materials;
	std::vector<CannonUIWidget>				m_ChildWidgets;

private:

	// Editor
	kbVec3									m_StartingPosition;
	kbVec3									m_StartingSize;


	// Runtime
	kbVec3									m_RelativePosition;
	kbVec3									m_RelativeSize;
	kbVec3									m_AbsolutePosition;
	kbVec3									m_AbsoluteSize;
	kbStaticModelComponent *				m_pModel;

	kbVec3									m_CachedParentPosition;
	kbVec3									m_CachedParentSize;
};

/**
 *	CannonUISlider
 */
class CannonUISlider : public CannonUIWidget {

	KB_DECLARE_COMPONENT( CannonUISlider, CannonUIWidget );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	
	virtual void							RecalculateOld( const kbUIComponent *const pParent ) override;
	virtual void							Recalculate( const CannonUIWidget *const pParent ) override;

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	// Editor
	kbVec3									m_SliderBoundsMin;
	kbVec3									m_SliderBoundsMax;

	// Runtime
	kbVec3									m_CalculatedSliderBoundsMin;
	kbVec3									m_CalculatedSliderBoundsMax;
};

/**
 *	CannonBallPauseMenuUIComponent
 */
class CannonBallPauseMenuUIComponent : public kbUIComponent {

	KB_DECLARE_COMPONENT( CannonBallPauseMenuUIComponent, kbUIComponent );

//---------------------------------------------------------------------------------------------------
public:
	enum ePauseMenuOptions {
		BackToGame,
		MasterVolume,
		Brightness,
		VideoQuality,
		ExitToMainMenu,
		NumOptions
	};

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	std::vector<CannonUISlider>				m_SliderWidgets;
	std::vector<CannonUIWidget>				m_Widgets;
	kbVec3									m_WidgetSize;
	kbVec3									m_StartingWidgetAnchorPt;
	float									m_SpaceBetweenWidgets;

	kbGameEntity							m_Entity;
};


#endif