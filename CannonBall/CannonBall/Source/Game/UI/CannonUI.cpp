//===================================================================================================
// CannonUI.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonUI.h"


kbGameEntity & GetUIGameEntity() {
	static kbGameEntity g_UIGameEntity;
	return g_UIGameEntity;
}

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
				randomOffset.Set( ( kbfrand() - 0.5f ) * normalizedScreenSize.x * 12.0f, ( kbfrand() - 0.5f ) * normalizedScreenSize.x * 14.0f );
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
	m_WidgetSize.Set( 0.1f, 0.1f, 1.0f );
	m_StartingWidgetAnchorPt.Set( 0.0f, 0.0f, 0.0f );
	m_SpaceBetweenWidgets = 0.05f;
}

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
			m_Widgets[i].RecalculateOld( this, true );
		}

		for ( int i = 0; i < m_SliderWidgets.size(); i++ ) {
			m_Entity.AddComponent( &m_SliderWidgets[i] );
			m_SliderWidgets[i].Enable( false );
			m_SliderWidgets[i].Enable( true );
			m_SliderWidgets[i].RecalculateOld( this, true );
			m_SliderWidgets[i].RegisterEventListener( this );
		}

		m_SliderWidgets[0].SetNormalizedValue( kbSoundManager::GetMasterVolume() );

	} else {
		for ( int i = 0; i < m_Widgets.size(); i++ ) {
			m_Entity.RemoveComponent( &m_Widgets[i] );
			m_Widgets[i].Enable( false );
		}
		for ( int i = 0; i < m_SliderWidgets.size(); i++ ) {
			m_Entity.RemoveComponent( &m_SliderWidgets[i] );
			m_SliderWidgets[i].Enable( false );
			m_SliderWidgets[i].UnregisterEventListener( this );
		}
		if ( m_pStaticModelComponent != nullptr ) {
			m_pStaticModelComponent->Enable( false );
		}

		if ( m_SliderWidgets.size() > 0 ) {
			CannonBallGameSettingsComponent *const pGameSettings = CannonBallGameSettingsComponent::Get();
			pGameSettings->m_Volume = (int)kbClamp( m_SliderWidgets[0].GetNormalizedValue() * 100.0f, 0.0f, 100.0f );
			pGameSettings->SaveSettings();
		}
	}
}

/**
 *	CannonBallPauseMenuUIComponent::Update_Internal
 */
void CannonBallPauseMenuUIComponent::Update_Internal( const float DT ) {

	Super::Update_Internal( DT );

	const float ScreenPixelWidth = (float)g_pRenderer->GetBackBufferWidth();
	const float ScreenPixelHeight = (float)g_pRenderer->GetBackBufferHeight();

	kbVec3 nextPos = m_StartingWidgetAnchorPt;
	for ( size_t i = 0; i < m_Widgets.size(); i++ ) {

		CannonUIWidget & widget = m_Widgets[i];
		const kbVec2i textureDim = widget.GetBaseTextureDimensions();
		kbVec3 targetWidgetSize = m_WidgetSize;
		kbVec3 targetWidgetPos = nextPos;
		if ( textureDim.x > 0 ) {

			// Height is fixed by design, so calculate Width
			const float baseTextureAspectRatio = (float)textureDim.x / (float)textureDim.y;
			const float pixelHeight = targetWidgetSize.y * ScreenPixelHeight;
			const float targetPixelWidth = pixelHeight * baseTextureAspectRatio;
			targetWidgetSize.x = (float)targetPixelWidth / ScreenPixelWidth;

			// Right Justify
		//	targetWidgetPos.x -= targetWidgetSize.x;
		}	

		widget.SetRelativeSize( targetWidgetSize );
		widget.SetRelativePosition( targetWidgetPos );
		nextPos.y += m_SpaceBetweenWidgets;

		widget.RecalculateOld( this, false );
		widget.Update( DT );
	}

	for ( size_t i = 0; i < m_SliderWidgets.size(); i++ ) {

		CannonUIWidget & widget = m_SliderWidgets[i];
		const kbVec2i textureDim = widget.GetBaseTextureDimensions();
		kbVec3 targetWidgetSize = m_WidgetSize;
		kbVec3 targetWidgetPos = nextPos;

		if ( textureDim.x > 0 ) {

			// Height is fixed by design, so calculate Width
			const float baseTextureAspectRatio = (float)textureDim.x / (float)textureDim.y;
			const float pixelHeight = targetWidgetSize.y * ScreenPixelHeight;
			const float targetPixelWidth = pixelHeight * baseTextureAspectRatio;
			targetWidgetSize.x = (float)targetPixelWidth / ScreenPixelWidth;

			// Right Justify
		//	targetWidgetPos.x -= targetWidgetSize.x;
		}	

		widget.SetRelativeSize( targetWidgetSize );
		widget.SetRelativePosition( targetWidgetPos );

		nextPos.y += m_SpaceBetweenWidgets;
		widget.RecalculateOld( this, false );
		widget.Update( DT );
	}
}

/**
 *	CannonBallPauseMenuUIComponent::WidgetEventCB
 */
void CannonBallPauseMenuUIComponent::WidgetEventCB( CannonUIWidget *const pWidget ) {

	if ( pWidget == &m_SliderWidgets[0] ) {

		kbSoundManager::SetMasterVolume( m_SliderWidgets[0].GetNormalizedValue() );
	}
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

	if ( InstanceExists() == true ) {
		return ISingleton<CannonBallGameSettingsComponent>::Get();
	}

	CannonBallGameSettingsComponent * pInstance = nullptr;

	kbFile gameSettingsFile;
	if ( gameSettingsFile.Open( "./Settings/gameSettings.txt", kbFile::FT_Read ) ) {
		kbGameEntity *const gameEntity = gameSettingsFile.ReadGameEntity();
		pInstance = (CannonBallGameSettingsComponent*)gameEntity->GetComponent<CannonBallGameSettingsComponent>();
		gameSettingsFile.Close();
	}

	if ( pInstance == nullptr ) {
		pInstance = new CannonBallGameSettingsComponent();
	}

	InitializeSingletonInstance( pInstance );
	pInstance->SaveSettings();

	return pInstance;
}
