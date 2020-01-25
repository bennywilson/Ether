//==============================================================================
// CannonUI.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _CANNONUI_H_
#define _CANNONUI_H_

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
 *	CannonBallPauseMenuUIComponent
 */
class CannonBallPauseMenuUIComponent : public kbUIComponent, public IUIWidgetListener {

	KB_DECLARE_COMPONENT( CannonBallPauseMenuUIComponent, kbUIComponent );

//---------------------------------------------------------------------------------------------------
public:
	enum ePauseMenuOptions {
		BackToGame,
		MasterVolume,
		VideoQuality,
		Brightness,
		ExitToMainMenu,
		NumOptions
	};

	bool									CloseRequested() const { return m_bRequestClose; }
	int										GetSelectedWidgetIdx() const { return m_SelectedWidgetIdx; }


protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	virtual void							WidgetEventCB( kbUIWidgetComponent *const pWidget, const kbInput_t *const pInput );

private:

	void									RecalculateChildrenTransform();

	virtual void							InputCB( const kbInput_t & input ) override;

	// Editor
	std::vector<kbUISlider>					m_SliderWidgets;
	std::vector<kbUIWidgetComponent>					m_Widgets;
	kbVec3									m_WidgetSize;
	kbVec3									m_StartingWidgetAnchorPt;
	float									m_SpaceBetweenWidgets;
	std::vector<kbSoundData>				m_VolumeSliderTestWav;

	// Runtime
	std::vector<kbUIWidgetComponent*>				m_WidgetList;
	int										m_SelectedWidgetIdx;
	bool									m_bHackSlidersInit;

	bool									m_bRequestClose;

	kbGameEntity							m_Entity;
};

/**
 *	CannonBallMainMenuComponent
 */
class CannonBallMainMenuComponent : public kbUIWidgetComponent {

	KB_DECLARE_COMPONENT( CannonBallMainMenuComponent, kbUIWidgetComponent );

//---------------------------------------------------------------------------------------------------
public:

	enum eMainMenuOptions {
		PlayGame,
		Settings,
		Quit
	};

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	virtual void							WidgetEventCB( kbUIWidgetComponent *const pWidget );

	void									SetAnimationFrame( const int idx );

	int										GetSelectedIndex() const { return m_MainMenuIdx; }

protected:

	// Editor
	std::vector<kbSoundData>				m_ActionVO;


	// Runtime
	int										m_AnimationState;
	float									m_TimeAnimStateBegan;
	kbVec3									m_StartRelativePos;

	int										m_MainMenuIdx;

private:
		
	virtual void							InputCB( const kbInput_t & input ) override;

};

/**
 *	CannonBallYesNoPromptComponent
 */
class CannonBallYesNoPromptComponent : public kbUIWidgetComponent {

	KB_DECLARE_COMPONENT( CannonBallYesNoPromptComponent, kbUIWidgetComponent );

};

/**
 *	CannonBallGameSettingsComponent
 */
class CannonBallGameSettingsComponent : public kbGameComponent, public ISingleton<CannonBallGameSettingsComponent> {

	KB_DECLARE_COMPONENT( CannonBallGameSettingsComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	void									SaveSettings();

	static CannonBallGameSettingsComponent* Get();

	// Editor
	int										m_Volume;
	int										m_Brightness;
	int										m_VisualQuality;

private:

	inline static CannonBallGameSettingsComponent *	s_pInstance = nullptr;
};

#endif