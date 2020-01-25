//===================================================================================================
// CannonUI.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonUI.h"


/**
 *	CannonHealthBarUIComponent::Constructor
 */
 void CannonHealthBarUIComponent::Constructor() {

	m_HealthBarWarningFlashThreshold = -1.0f;
	m_HealthBarWarningFlashSpeed = 1.0f;

	m_TargetNormalizedHealth = 1.0f;
	m_CurrentNormalizedHealth = 1.0f;
	m_StartFlashTime = -1.0f;
}

/**
 *	CannonHealthBarUIComponent::SetEnable_Internal
 */
void CannonHealthBarUIComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );
}

/**
 *	CannonHealthBarUIComponent::Update_Internal
 */
void CannonHealthBarUIComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	if ( m_pStaticModelComponent != nullptr ) {
		static kbString normalizedHealth( "normalizedHealth" );
		m_pStaticModelComponent->SetMaterialParamVector( 0, normalizedHealth.stl_str(), kbVec4( m_TargetNormalizedHealth, 0.0f, 0.0f, 0.0f ) );

		static kbString barTextureWeights( "barTextureWeights" );
		kbVec4 weights( 1.0f, 0.0f, 0.0f, 0.0f );
		if ( m_StartFlashTime > 0.0f ) {
			weights.x = ( g_GlobalTimer.TimeElapsedSeconds() - m_StartFlashTime ) * m_HealthBarWarningFlashSpeed;
			weights.x = sin( ( weights.x ) - ( kbPI * 0.5f ) ) * 0.5f + 0.5f;
			weights.y = 1.0f - weights.x;
		}
		m_pStaticModelComponent->SetMaterialParamVector( 0, barTextureWeights.stl_str(), weights );
		
	}
}

/**
 *	CannonHealthBarUIComponent::SetTargetHealth
 */
void CannonHealthBarUIComponent::SetTargetHealth( const float newHealth ) {
	m_TargetNormalizedHealth = newHealth;

	if ( newHealth < m_HealthBarWarningFlashThreshold ) {
		if ( m_StartFlashTime < 0.0f ) {
			m_StartFlashTime = g_GlobalTimer.TimeElapsedSeconds();
		}
	} else {
		m_StartFlashTime = -1.0f;
	}
}

/**
 *	CannonBallUIComponent::Constructor
 */
void CannonBallUIComponent::Constructor() {

	m_SparkRelativePosition.Set( 0.5f, 0.5f, 0.f );
	m_SparkRelativeSize.Set( 0.25f, 0.25f, 0.25f );

	m_BoomRelativePosition.Set( 0.5f, 0.5f, 0.f );
	m_BoomRelativeSize.Set( 0.25f, 0.25f, 0.25f );

	m_SmokeRelativePosition.Set( 0.5f, 0.5f, 0.f );
	m_SparkRelativeSize.Set( 0.25f, 0.25f, 0.25f );

	m_CurrentFill = 0.0f;
	m_TargetFill = 0.0f;
	m_pSparkModel = nullptr;
	m_pBoomModel = nullptr;
	m_pSmokeModel = nullptr;

	m_NextSparkAnimUpdateTime = -1.0f;
	m_CannonBallActivatedStartTime = -1.0f;
	m_NextSmokeCloudUpdateTime = -1.0f;
}

/**
 *	CannonBallUIComponent::SetEnable_Internal
 */
void CannonBallUIComponent::SetEnable_Internal( const bool bEnable ) {
	
	Super::SetEnable_Internal( bEnable );

	m_CannonBallActivatedStartTime = -1.0f;
	if ( bEnable ) {
		m_pSparkModel = GetOwner()->GetComponent(3)->GetAs<kbStaticModelComponent>();
		m_pBoomModel = GetOwner()->GetComponent(4)->GetAs<kbStaticModelComponent>();
		m_pSmokeModel = GetOwner()->GetComponent(5)->GetAs<kbStaticModelComponent>();

		m_pBoomModel->Enable( false );
		m_pSmokeModel->Enable( false );
	}
}

/**
 *	CannonBallUIComponent::Update_Internal
 */
void CannonBallUIComponent::Update_Internal( const float dt ) {
	
	Super::Update_Internal( dt );

	if ( m_pStaticModelComponent != nullptr ) {
		static kbString meterFill( "meterFill" );
		m_pStaticModelComponent->SetMaterialParamVector( 0, meterFill.stl_str(), kbVec4( m_CurrentFill, 0.0f, 0.0f, 0.0f ) );		
	}

	const float ScreenPixelWidth = (float)g_pRenderer->GetBackBufferWidth();
	const float ScreenPixelHeight = (float)g_pRenderer->GetBackBufferHeight();

	const kbVec3 normalizedScreenSize = GetNormalizedScreenSize();
	const kbVec2 normalizedAnchorPt( GetNormalizedAnchorPt().x, GetNormalizedAnchorPt().y );

	static const kbString normalizedScreenSize_Anchor( "normalizedScreenSize_Anchor" );
	static const kbString colorFactor( "colorFactor" );
	static const kbString rotationAngle( "rotationAngle" );

	if ( m_pSparkModel != nullptr ) {

		static kbVec2 randomScaleFactor( 1.0f, 1.0f );

		if ( m_CurrentFill >= 1.0f ) {
			m_pSparkModel->SetMaterialParamVector( 0, colorFactor.stl_str(), kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

			if ( m_NextSparkAnimUpdateTime > 0.0f && g_GlobalTimer.TimeElapsedSeconds() > m_NextSparkAnimUpdateTime ) {
				m_NextSparkAnimUpdateTime = g_GlobalTimer.TimeElapsedSeconds() + 0.25f;
				m_pSparkModel->SetMaterialParamVector( 0, rotationAngle.stl_str(), kbVec4( kbfrand() * kbPI * 2.0f, 0.0f, 0.0f, 0.0f ) );

				const float randSize = 0.5f + 0.5f * kbfrand();
				randomScaleFactor.Set( randSize, randSize );
			}
		} else {
			m_pSparkModel->SetMaterialParamVector( 0, colorFactor.stl_str(), kbVec4( 0.0f, 0.0f, 0.0f, 0.25f ) );
		}

		const kbVec2 screenSize = kbVec2( normalizedScreenSize.x * m_SparkRelativeSize.x, normalizedScreenSize.y * m_SparkRelativeSize.y );
		const kbVec2 screenPos = normalizedAnchorPt + kbVec2( GetNormalizedScreenSize().x * m_SparkRelativePosition.x, GetNormalizedScreenSize().y * m_SparkRelativePosition.y );

		m_pSparkModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
			kbVec4( screenSize.x * randomScaleFactor.x,
					screenSize.y * randomScaleFactor.y,
					screenPos.x + 0.5f * screenSize.x + screenSize.x * randomScaleFactor.x * 0.05f,
					screenPos.y + 0.5f * screenSize.y + screenSize.y * randomScaleFactor.y * 0.05f ) );
	}

	if ( m_pSmokeModel != nullptr ) {

		const kbVec2 screenSize = kbVec2( normalizedScreenSize.x * m_SmokeRelativeSize.x, normalizedScreenSize.y * m_SmokeRelativeSize.y );
		const kbVec2 screenPos = normalizedAnchorPt + kbVec2( GetNormalizedScreenSize().x * m_SmokeRelativePosition.x, GetNormalizedScreenSize().y * m_SmokeRelativePosition.y );
		m_pSmokeModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
							kbVec4( screenSize.x, 
									screenSize.y, 
									screenPos.x + 0.5f * screenSize.x,
									screenPos.y + 0.5f * screenSize.y ) );
	}

	if ( m_CannonBallActivatedStartTime > 0.0f ) {
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();

		static kbVec2 randomOffset( 0.0f, 0.0f );

		if ( curTime > m_CannonBallActivatedStartTime + 1.0f ) {
			m_CannonBallActivatedStartTime = -1.0f;

			m_pSparkModel->Enable( true );
			m_pStaticModelComponent->Enable( true );
		} else {
			m_pBoomModel->Enable( true );
			m_pSmokeModel->Enable( true );
			m_pSparkModel->Enable( false );
			m_pStaticModelComponent->Enable( false );

			static int idx = 0;
			if ( curTime > m_NextSmokeCloudUpdateTime ) {
				m_NextSmokeCloudUpdateTime = curTime + 0.2f;
				idx = ( idx + 1 ) % 4;
				randomOffset.Set( ( kbfrand() - 0.5f ) * normalizedScreenSize.x * 7.0f, ( kbfrand() - 0.5f ) * normalizedScreenSize.x * 14.0f );
			}
			kbVec4 textureMask = kbVec4::zero;
			textureMask[idx] = 1.0f;

			static kbString channelMask( "channelMask" );
			m_pSmokeModel->SetMaterialParamVector( 0, channelMask.stl_str(), textureMask );
		}

		const kbVec2 screenSize = kbVec2( normalizedScreenSize.x * m_BoomRelativeSize.x, normalizedScreenSize.y * m_BoomRelativeSize.y );
		const kbVec2 screenPos = normalizedAnchorPt + kbVec2( GetNormalizedScreenSize().x * m_BoomRelativePosition.x, GetNormalizedScreenSize().y * m_BoomRelativePosition.y );
		m_pBoomModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
							kbVec4( screenSize.x, screenSize.y, 
								   screenPos.x + 0.5f * screenSize.x + screenSize.x * randomOffset.x,
								   screenPos.y + 0.5f * screenSize.y + screenSize.y * randomOffset.y ) );
	} else {
		m_pBoomModel->Enable( false );
		m_pSmokeModel->Enable( false );
		m_pSparkModel->Enable( true );
		m_pStaticModelComponent->Enable( true );
	}
}

/**
 *	CannonBallUIComponent::SetFill
 */
void CannonBallUIComponent::SetFill( const float fill ) {
	
	if ( m_CurrentFill >= 1.0f && fill >= 1.0f ) {
		return;
	}

	m_CurrentFill = m_TargetFill = kbSaturate( fill );

	if ( fill >= 1.0f ) {
		m_pSparkModel->Enable( true );
		m_NextSparkAnimUpdateTime = g_GlobalTimer.TimeElapsedSeconds() + 0.25f;
	}
}

/**
 *	CannonBallUIComponent::CannonBallActivatedCB
 */
void CannonBallUIComponent::CannonBallActivatedCB() {
	m_CurrentFill = m_TargetFill = 0.0f;

	m_CannonBallActivatedStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_NextSmokeCloudUpdateTime = m_CannonBallActivatedStartTime + 0.2f;
}

/**
 *	CannonBallPauseMenuUIComponent::Constructor
 */
void CannonBallPauseMenuUIComponent::Constructor() {

	// Editor
	m_WidgetSize.Set( 0.1f, 0.1f, 1.0f );
	m_StartingWidgetAnchorPt.Set( 0.0f, 0.0f, 0.0f );
	m_SpaceBetweenWidgets = 0.05f;

	// Runtime
	m_SelectedWidgetIdx = 0;

	m_bHackSlidersInit = false;

	m_bRequestClose = false;
}

const kbVec3 g_CheckMarkPos[] = {  kbVec3( 0.400000f, 0.287480f, 0.000000f ), kbVec3( 0.3600704f, 0.353173f, 0.000000f ), kbVec3(0.325628f, 0.415369f, 0.000000f ), kbVec3( 0.322864f, 0.484983f, 0.000000f ), kbVec3( 0.400000f, 0.557629f, 0.000000f ) }; 
kbVec3 g_Offsets[] = { kbVec3::zero, kbVec3::zero, kbVec3::zero, kbVec3::zero, kbVec3::zero, kbVec3::zero };
/*

/**
 *	CannonBallPauseMenuUIComponent::SetEnable_Internal
 */
void CannonBallPauseMenuUIComponent::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		m_pStaticModelComponent->Enable( true );
		for ( int i = 0; i < m_Widgets.size(); i++ ) {
			m_Entity.AddComponent( &m_Widgets[i] );
			m_Widgets[i].Enable( false );
			m_Widgets[i].Enable( true );
			m_Widgets[i].RegisterEventListener( this );
		}

		for ( int i = 0; i < m_SliderWidgets.size(); i++ ) {
			m_Entity.AddComponent( &m_SliderWidgets[i] );
			m_SliderWidgets[i].Enable( false );
			m_SliderWidgets[i].Enable( true );
			m_SliderWidgets[i].RegisterEventListener( this );
		}

		m_WidgetList.clear();
		m_WidgetList.push_back( &m_Widgets[0] );
		m_WidgetList.push_back( &m_SliderWidgets[0] );
		m_WidgetList.push_back( &m_SliderWidgets[1] );
		m_WidgetList.push_back( &m_SliderWidgets[2] );
		m_WidgetList.push_back( &m_Widgets[1] );

		m_SelectedWidgetIdx = 0;
		m_WidgetList[m_SelectedWidgetIdx]->SetFocus( true );

		RecalculateChildrenTransform();

		m_SliderWidgets[0].SetNormalizedValue( g_pCannonGame->GetSoundManager().GetMasterVolume() );
		m_SliderWidgets[1].SetNormalizedValue( CannonBallGameSettingsComponent::Get()->m_VisualQuality / 100.0f );
		m_SliderWidgets[2].SetNormalizedValue( CannonBallGameSettingsComponent::Get()->m_Brightness / 100.0f );
		m_bHackSlidersInit = true;

		g_pInputManager->RegisterInputListener( this );

		m_Widgets.back().SetRelativePosition( g_CheckMarkPos[m_SelectedWidgetIdx] );
		m_Widgets.back().SetRenderOrderBias( -5.0f );
	} else {
		for ( int i = 0; i < m_Widgets.size(); i++ ) {
			m_Entity.RemoveComponent( &m_Widgets[i] );
			m_Widgets[i].Enable( false );
			m_Widgets[i].UnregisterEventListener( this );
		}

		for ( int i = 0; i < m_SliderWidgets.size(); i++ ) {
			m_Entity.RemoveComponent( &m_SliderWidgets[i] );
			m_SliderWidgets[i].Enable( false );
			m_SliderWidgets[i].UnregisterEventListener( this );
		}
		if ( m_pStaticModelComponent != nullptr ) {
			m_pStaticModelComponent->Enable( false );
		}

		CannonBallGameSettingsComponent *const pGameSettings = CannonBallGameSettingsComponent::Get();
		const int prevQuality = pGameSettings->m_VisualQuality;
		if ( m_SliderWidgets.size() > 0 && m_bHackSlidersInit == true ) {
			pGameSettings->m_Volume = (int)kbClamp( m_SliderWidgets[0].GetNormalizedValue() * 100.0f, 0.0f, 100.0f );
			pGameSettings->m_VisualQuality = (int)kbClamp( m_SliderWidgets[1].GetNormalizedValue() * 100.0f, 0.0f, 100.0f );
			pGameSettings->m_Brightness = (int)kbClamp( m_SliderWidgets[2].GetNormalizedValue() * 100.0f, 0.0f, 100.0f );
			pGameSettings->SaveSettings();
		}

		if ( prevQuality != pGameSettings->m_VisualQuality ) {
			const float LOD = (float)pGameSettings->m_VisualQuality / 100.0f;
			kbTerrainComponent::SetTerrainLOD( LOD );
		}

		if ( m_WidgetList.size() > 0 ) {
			m_WidgetList[m_SelectedWidgetIdx]->SetFocus( false );
		}

		for ( int i = 0; i < m_VolumeSliderTestWav.size(); i++ ) {
			m_VolumeSliderTestWav[i].StopSound();
		}

		g_pInputManager->UnregisterInputListener( this );
	}

	m_bRequestClose = false;
}

/**
 *	CannonBallPauseMenuUIComponent::RecalculateChildrenTransform
 */
void CannonBallPauseMenuUIComponent::RecalculateChildrenTransform() {

	const float ScreenPixelWidth = (float)g_pRenderer->GetBackBufferWidth();
	const float ScreenPixelHeight = (float)g_pRenderer->GetBackBufferHeight();

	static float sliderOffset = 0.03f;

	kbVec3 nextPos = m_StartingWidgetAnchorPt;
	nextPos.x -= sliderOffset;	// hack!
	for ( size_t i = 0; i < m_WidgetList.size(); i++ ) {
		kbUIWidgetComponent & widget = *m_WidgetList[i];
		const kbVec2i textureDim = widget.GetBaseTextureDimensions();
		kbVec3 targetWidgetSize = m_WidgetSize;
		kbVec3 targetWidgetPos = nextPos;

		if ( textureDim.x > 0 ) {
			// Height is fixed by design, so calculate Width
			const float baseTextureAspectRatio = (float)textureDim.x / (float)textureDim.y;
			const float pixelHeight = targetWidgetSize.y * ScreenPixelHeight;
			const float targetPixelWidth = pixelHeight * baseTextureAspectRatio;
			targetWidgetSize.x = (float)targetPixelWidth / ScreenPixelWidth;
		}	

		widget.SetRelativeSize( targetWidgetSize );
		widget.SetRelativePosition( targetWidgetPos );
		nextPos.y += m_SpaceBetweenWidgets;

		widget.RecalculateOld( this, false );
	}
}

/**
 *	CannonBallPauseMenuUIComponent::InputCB
 */
void CannonBallPauseMenuUIComponent::InputCB( const kbInput_t & input ) {

	bool bNewOptionSelected = false;
	int prevSelected = m_SelectedWidgetIdx;
	const kbVec2 LeftStick = GetLeftStick( &input );
	const kbVec2 PrevLeftStick = GetPrevLeftStick( &input );

	if ( LeftStick.y > 0.5f && PrevLeftStick.y <= 0.5f ) {
		m_WidgetList[m_SelectedWidgetIdx]->SetFocus( false );
		m_SelectedWidgetIdx--;
		if ( m_SelectedWidgetIdx < 0 ) {
			m_SelectedWidgetIdx = (int)m_WidgetList.size() - 1;
		}
		m_WidgetList[m_SelectedWidgetIdx]->SetFocus( true );
		bNewOptionSelected = true;
	} else if ( LeftStick.y < -0.5f && PrevLeftStick.y >= -0.5f ) {
		m_WidgetList[m_SelectedWidgetIdx]->SetFocus( false );
		m_SelectedWidgetIdx++;
		if ( m_SelectedWidgetIdx >= m_WidgetList.size() ) {
			m_SelectedWidgetIdx = 0;
		}
		m_WidgetList[m_SelectedWidgetIdx]->SetFocus( true );
		bNewOptionSelected = true;
	}

	if ( bNewOptionSelected ) {

		if ( m_WidgetList[m_SelectedWidgetIdx] == &m_SliderWidgets[0] && m_VolumeSliderTestWav.size() > 0 ) {
			m_VolumeSliderTestWav[rand() % m_VolumeSliderTestWav.size()].PlaySoundAtPosition( kbVec3::zero );
		} else if ( m_WidgetList[prevSelected] == &m_SliderWidgets[0] ) {
			for ( int i = 0; i < m_VolumeSliderTestWav.size(); i++ ) {
				m_VolumeSliderTestWav[i].StopSound();
			}
		}

		m_Widgets.back().SetRelativePosition( g_CheckMarkPos[m_SelectedWidgetIdx] + g_Offsets[m_SelectedWidgetIdx] );
	}

	if ( input.IsNonCharKeyPressedOrDown( kbInput_t::Return ) || WasAttackJustPressed() || WasSpecialAttackPressed() || WasStartButtonPressed() ) {

		FireEvent();
	}
}

/**
 *	CannonBallPauseMenuUIComponent::Update_Internal
 */
void CannonBallPauseMenuUIComponent::Update_Internal( const float DT ) {

	Super::Update_Internal( DT );

	RecalculateChildrenTransform();
/*
	static int offsetIdx = 0;
	if ( GetAsyncKeyState( '0' ) ) {
		offsetIdx = 0;
	} else if ( GetAsyncKeyState( '1' ) ) {
		offsetIdx = 1;
	} else if ( GetAsyncKeyState( '2' ) ) {
		offsetIdx = 2;
	} else if ( GetAsyncKeyState( '3' ) ) {
		offsetIdx = 3;
	} else if ( GetAsyncKeyState( '4' ) ) {
		offsetIdx = 4;
	}

	static float speed = 0.25f;
	if ( GetAsyncKeyState('U')) {
		g_Offsets[offsetIdx].y += DT * speed;
	} else if ( GetAsyncKeyState('I')) {
		g_Offsets[offsetIdx].y -= DT * speed;
	} else if ( GetAsyncKeyState('J')) {
		g_Offsets[offsetIdx].x -= DT * speed;
	} else if ( GetAsyncKeyState('K')) {
		g_Offsets[offsetIdx].x += DT * speed;
	}

	kbLog( "------" );
	for ( int i = 0; i < 5; i++ ) {
		kbLog( "%d: %f %f %f", i, g_CheckMarkPos[i].x + g_Offsets[i].x, g_CheckMarkPos[i].y + g_Offsets[i].y, g_CheckMarkPos[i].z + g_Offsets[i].z );
	}
		m_Widgets.back().SetRelativePosition( g_CheckMarkPos[m_SelectedWidgetIdx] + g_Offsets[m_SelectedWidgetIdx] );
		*/
	for ( size_t i = 0; i < m_Widgets.size(); i++ ) {
		kbUIWidgetComponent & widget = m_Widgets[i];
		widget.Update( DT );
	}

	for ( size_t i = 0; i < m_SliderWidgets.size(); i++ ) {
		kbUIWidgetComponent & widget = m_SliderWidgets[i];
		widget.Update( DT );
	}
}

/**
 *	CannonBallPauseMenuUIComponent::WidgetEventCB
 */
void CannonBallPauseMenuUIComponent::WidgetEventCB( kbUIWidgetComponent *const pWidget, const kbInput_t *const pInput  ) {

	if ( pWidget == &m_SliderWidgets[0] ) {
		// Volume
		g_pCannonGame->GetSoundManager().SetMasterVolume( m_SliderWidgets[0].GetNormalizedValue() );
	} else if ( pWidget == &m_SliderWidgets[2] ) {
		// Brightness
		kbShaderParamOverrides_t shaderParam;
		shaderParam.SetVec4( "globalTint", kbVec4( 0.0f, 0.0f, 0.0f, 1.0f - m_SliderWidgets[2].GetNormalizedValue() ) );
		g_pRenderer->SetGlobalShaderParam( shaderParam );
	} else if ( pWidget == &m_Widgets[0] ) {
		// Close Pause menu
		m_bRequestClose = true;
	} else if ( pWidget == &m_Widgets[1] ) { 
		// Exit Game
		g_pCannonGame->RequestQuitGame();
	}
}

/**
 *	CannonBallMainMenuComponent::Constructor
 */
void CannonBallMainMenuComponent::Constructor() {
	m_AnimationState = 0;
	m_TimeAnimStateBegan = -1.0f;
	m_StartRelativePos.Set( -1.0f, -1.0f, 0.0f );

	m_MainMenuIdx = 0;
}

/**
 *	CannonBallMainMenuComponent::InputCB
 */
static float selectionStartX[] = { 0.06f, 0.21999f, 0.41999f };
static float selectionOffset = 0.17f;

void CannonBallMainMenuComponent::InputCB( const kbInput_t & input ) {

	if ( m_AnimationState != 0 ) {
		return;
	}

	const kbVec2 & leftStick = GetLeftStick( &input );
	const kbVec2 & prevLeftStick = GetPrevLeftStick( &input );

	if ( leftStick.x < -0.5f && prevLeftStick.x >= -0.5f ) {
		m_MainMenuIdx = m_MainMenuIdx - 1;
		if ( m_MainMenuIdx < 0 ) {
			m_MainMenuIdx = 2;
		}
	} else if ( leftStick.x > 0.5f && prevLeftStick.x <= 0.5f ) {
		m_MainMenuIdx = m_MainMenuIdx + 1;
		if ( m_MainMenuIdx > 2 ) {
			m_MainMenuIdx = 0;
		}
	}

	kbVec3 relPos = m_ChildWidgets[1].GetRelativePosition();
	relPos.x = selectionStartX[m_MainMenuIdx];
	m_ChildWidgets[1].SetRelativePosition( relPos );

	if ( input.IsNonCharKeyPressedOrDown( kbInput_t::Return ) || WasAttackJustPressed() || WasSpecialAttackPressed() || WasStartButtonPressed() ) {
		FireEvent( &input );
	}
}

/**
 *	CannonBallMainMenuComponent::SetEnable_Internal
 */
void CannonBallMainMenuComponent::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {

		if ( g_UseEditor == false ) {
			if ( m_StartRelativePos.x < 0 ) {
				m_StartRelativePos = m_RelativePosition;
			} else {
				m_RelativePosition = m_StartRelativePos;
			}
		}
		SetAnimationFrame( 0 );
		Recalculate( nullptr, true );

		kbVec3 relPos = m_ChildWidgets[1].GetRelativePosition();
		relPos.x = selectionStartX[m_MainMenuIdx];
		m_ChildWidgets[1].SetRelativePosition( relPos );
	}
}

/**
 *	CannonBallMainMenuComponent::Update_Internal
 */
void CannonBallMainMenuComponent::Update_Internal( const float dt ) {

	Super::Update_Internal( dt );

	const float animStateSecElapsed = g_GlobalTimer.TimeElapsedSeconds() - m_TimeAnimStateBegan;
	if ( m_AnimationState == 1 ) {
		if ( animStateSecElapsed > 1.0f ) {
			SetAnimationFrame( 2 );
		}
	} else if ( m_AnimationState == 2 ) {
		SetAnimationFrame( 3 );
	} else if ( m_AnimationState == 3 ) {

		if ( animStateSecElapsed > 0.33f ) {
			this->m_RelativePosition.x -= dt * 0.85f;
			Recalculate( nullptr, true );

			if ( m_RelativePosition.x + m_RelativeSize.x <= 0.0f ) {
				GetOwner()->DisableAllComponents();
			}
		}
	}
}

/**
 *	CannonBallMainMenuComponent::SetAnimationFrame
 */
void CannonBallMainMenuComponent::SetAnimationFrame( const int idx ) {

	m_AnimationState = idx;
	m_TimeAnimStateBegan = g_GlobalTimer.TimeElapsedSeconds();

	if ( m_AnimationState == 0 ) {
		m_ChildWidgets[0].Enable( false );
		m_pModel->Enable( true );
		m_RelativePosition.x = 0.060000f;
	} else if ( m_AnimationState == 1 ) {

		PlayRandomSound( m_ActionVO );
	} else if ( m_AnimationState == 2 ) {
		m_ChildWidgets[0].Enable( true );
		Recalculate( nullptr, true );
	} else if ( m_AnimationState == 3 ) {
		m_pModel->Enable( false );
	}
}

/**
 *	CannonBallMainMenuComponent::WidgetEventCB
 */
void CannonBallMainMenuComponent::WidgetEventCB( kbUIWidgetComponent *const pWidget ) {

}

/**
 *	CannonBallGameSettingsComponent::Constructor
 */
void CannonBallGameSettingsComponent::Constructor() {

	m_Volume = 100;
	m_Brightness = 100;
	m_VisualQuality = 100;
}

/**
 *	CannonBallGameSettingsComponent::SaveSettings
 */
void CannonBallGameSettingsComponent::SaveSettings() {

	if ( g_UseEditor == true ) {
		return;
	}

	// Save Editor Settings
	kbFile outFile;
	outFile.Open( "./Settings/gameSettings.txt", kbFile::FT_Write );

	kbGameEntity gameSettingsEnt;
	gameSettingsEnt.AddComponent( this );
	outFile.WriteGameEntity( &gameSettingsEnt );
	outFile.Close();
	gameSettingsEnt.RemoveComponent( this );
}

/**
 *	CannonBallGameSettingsComponent::Get
 */
CannonBallGameSettingsComponent * CannonBallGameSettingsComponent::Get() {

	if ( s_pInstance != nullptr ) {
		return s_pInstance;
	}

	CannonBallGameSettingsComponent * pInstance = nullptr;

	kbFile gameSettingsFile;
	if ( gameSettingsFile.Open( "./Settings/gameSettings.txt", kbFile::FT_Read ) ) {
		kbGameEntity *const gameEntity = gameSettingsFile.ReadGameEntity();
		s_pInstance = (CannonBallGameSettingsComponent*)gameEntity->GetComponent<CannonBallGameSettingsComponent>();
		gameSettingsFile.Close();
	}

	if ( s_pInstance == nullptr ) {
		s_pInstance = new CannonBallGameSettingsComponent();
	}

	s_pInstance->SaveSettings();

	return s_pInstance;
}

/**
 *	CannonBallYesNoPromptComponent::Constructor
 */
void CannonBallYesNoPromptComponent::Constructor() {

}