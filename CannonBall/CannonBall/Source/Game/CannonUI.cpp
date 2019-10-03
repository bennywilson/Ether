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
 *	CannonUIWidget::Constructor
 */
void CannonUIWidget::Constructor() {
	m_RelativePosition.Set( 0.0f, 0.0f, 0.0f );
	m_RelativeSize.Set( 0.5f, 0.5f, 1.0f );

	m_AbsolutePosition.Set( 0.0f, 0.0f, 0.0f );
	m_RelativeSize.Set( 0.5f, 0.5f, 1.0f );

	m_pModel = nullptr;
}

/**
 *	CannonUIWidget::Recalculate
 */
void CannonUIWidget::Recalculate( const kbUIComponent *const pParent ) {

	kbErrorCheck( pParent != nullptr, "CannonUIWidget::UpdateFromParent() - null parent" );
	
	if ( m_pModel != nullptr && pParent != nullptr && pParent->GetStaticModelComponent() != nullptr ) {
		m_pModel->SetRenderOrderBias( pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );
	}

	const kbVec3 parentStart = pParent->GetNormalizedAnchorPt();
	const kbVec3 parentEnd = parentStart + pParent->GetNormalizedScreenSize();
	m_AbsolutePosition = parentStart + ( parentEnd - parentStart ) * m_RelativePosition;
	m_AbsoluteSize = pParent->GetNormalizedScreenSize() * m_RelativeSize;

	for ( int i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Recalculate( this );
	}
}

/**
 *	CannonUIWidget::Recalculate
 */
void CannonUIWidget::Recalculate( const CannonUIWidget *const pParent ) {

	kbErrorCheck( pParent != nullptr, "CannonUIWidget::UpdateFromParent() - null parent" );
	
	if ( m_pModel != nullptr && pParent != nullptr && pParent->m_pModel != nullptr ) {
		m_pModel->SetRenderOrderBias( pParent->m_pModel ->GetRenderOrderBias() - 1.0f );
	}

	const kbVec3 parentStart = pParent->GetAbsolutePosition();
	m_AbsolutePosition = parentStart + pParent->GetAbsoluteSize() * m_RelativePosition;
	m_AbsoluteSize = pParent->GetAbsoluteSize() * m_RelativeSize;

	for ( int i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Recalculate( this );
	}
}

/**
 *	CannonUIWidget::GetBaseTextureDimensions
 */
kbVec2i	CannonUIWidget::GetBaseTextureDimensions() const {

	kbVec2i retDim( -1, -1 );
	if ( m_pModel == nullptr ) {
		return retDim;
	}

	const kbShaderParamComponent *const pComp = m_pModel->GetShaderParamComponent( 0, kbString( "baseTexture" ) );
	if ( pComp == nullptr || pComp->GetTexture() == nullptr ) {
		return retDim;
	}

	retDim.x = pComp->GetTexture()->GetWidth();
	retDim.y = pComp->GetTexture()->GetHeight();
	return retDim;
}


/**
 *	CannonUIWidget::SetEnable_Internal
 */
void CannonUIWidget::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );

	static kbModel * pUnitQuad = nullptr;
	if ( pUnitQuad == nullptr ) {
		pUnitQuad = (kbModel*)g_ResourceManager.GetResource( "../../kbEngine/assets/Models/UnitQuad.ms3d", true, true );
	}

	if ( GetOwner() == nullptr ) {
		return;
	}

	if ( bEnable ) {
		if ( m_pModel == nullptr ) {
			m_pModel = new kbStaticModelComponent();
			GetUIGameEntity().AddComponent( m_pModel );
		}

		m_pModel->SetModel( pUnitQuad );
		m_pModel->SetMaterials( m_Materials );
		m_pModel->SetRenderPass( RP_UI );
		m_pModel->Enable( false );
		m_pModel->Enable( true );

		for ( size_t i = 0; i < m_ChildWidgets.size(); i++ ) {
		//	m_ChildWidgets[i].Recalculate( this );
			GetUIGameEntity().AddComponent( &m_ChildWidgets[i] );		// Note these children are responsible for removing themselves when disabled (see code block below)
			m_ChildWidgets[i].Enable( false );
			m_ChildWidgets[i].Enable( true );
		}


	} else {
		if ( m_pModel != nullptr ) {
			m_pModel->Enable( false );
			//delete m_pModel;
			//m_pModel = nullptr;
		}

		for ( size_t i = 0; i < m_ChildWidgets.size(); i++ ) {
			m_ChildWidgets[i].Enable( false );
		}

		GetUIGameEntity().RemoveComponent( this );
		GetUIGameEntity().RemoveComponent( m_pModel );
	}
}

/**
 *	CannonUIWidget::Update_Internal
 */
void CannonUIWidget::Update_Internal( const float dt ) {

	Super::Update_Internal( dt );

	if ( m_pModel == nullptr ) {
		return;
	}

	static const kbString normalizedScreenSize_Anchor( "normalizedScreenSize_Anchor" );

	kbVec3 parentEnd = kbVec3::one;
	kbVec3 widgetAbsPos = m_AbsolutePosition;
	kbVec3 widgetAbsSize = m_AbsoluteSize;
//	float renderOrderBias = 0.0f;

	float aspectRatio = 1.0f;
	const kbShaderParamComponent *const pComp = m_pModel->GetShaderParamComponent( 0, kbString( "baseTexture" ) );
	if ( pComp != nullptr ) {
		const kbTexture *const pTex = pComp->GetTexture();
		if ( pComp != nullptr ) {
			aspectRatio = (float)pTex->GetWidth() / (float)pTex->GetHeight();
		}
	}

	const float ScreenPixelWidth = (float)g_pRenderer->GetBackBufferWidth();
	const float ScreenPixelHeight = (float)g_pRenderer->GetBackBufferHeight();

	const float screenWidthPixel = widgetAbsSize.x * ScreenPixelWidth;
	const float screenHeightPixel = screenWidthPixel / aspectRatio;
	widgetAbsSize.y = screenHeightPixel / ScreenPixelHeight;

	m_pModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
			kbVec4( widgetAbsSize.x, 
					widgetAbsSize.y,
					widgetAbsPos.x + widgetAbsSize.x * 0.5f,
					widgetAbsPos.y + widgetAbsSize.y * 0.5f ) );
	//m_pModel->SetRenderOrderBias( renderOrderBias );
	m_pModel->RefreshMaterials( true );

	for ( size_t i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Update_Internal( dt );
	}
}

/**
 *	CannonUISlider::Constructor
 */
void CannonUISlider::Constructor() {

}

/**
 *	CannonUISlider::SetEnable_Internal
 */
void CannonUISlider::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );
}

/**
 *	CannonUISlider::Update_Internal
 */
void CannonUISlider::Update_Internal( const float dt ) {

	Super::Update_Internal( dt );

	kbVec3 curPos = m_ChildWidgets[1].GetRelativePosition();
	if ( GetAsyncKeyState( VK_LEFT ) ) {
		curPos.x -= 0.01f;
	}

		if ( GetAsyncKeyState( VK_RIGHT ) ) {
		curPos.x += 0.01f;
	}

	m_ChildWidgets[1].SetRelativePosition( curPos );
	//kbLog( "CannonUISlider Child = %f %f %f, %f %f %f", GetAbsolutePosition().x, GetAbsolutePosition().y, GetAbsolutePosition().z, GetAbsoluteSize().x, GetAbsoluteSize().y, GetAbsoluteSize().z );
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
			m_Widgets[i].Recalculate( this );
		}

		for ( int i = 0; i < m_SliderWidgets.size(); i++ ) {
			m_Entity.AddComponent( &m_SliderWidgets[i] );
			m_SliderWidgets[i].Enable( false );
			m_SliderWidgets[i].Enable( true );
			m_SliderWidgets[i].Recalculate( this );
		}
	} else {
		for ( int i = 0; i < m_Widgets.size(); i++ ) {
			m_Entity.RemoveComponent( &m_Widgets[i] );
			m_Widgets[i].Enable( false );
		}
		for ( int i = 0; i < m_SliderWidgets.size(); i++ ) {
			m_Entity.AddComponent( &m_SliderWidgets[i] );
			m_SliderWidgets[i].Enable( false );
		}
		if ( m_pStaticModelComponent != nullptr ) {
			m_pStaticModelComponent->Enable( false );
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
			targetWidgetPos.x -= targetWidgetSize.x;
		}	

		widget.SetRelativeSize( targetWidgetSize );
		widget.SetRelativePosition( targetWidgetPos );
		nextPos.y += m_SpaceBetweenWidgets;

		widget.Recalculate( this );
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
			targetWidgetPos.x -= targetWidgetSize.x;
		}	

		widget.SetRelativeSize( targetWidgetSize );
		widget.SetRelativePosition( targetWidgetPos );
		nextPos.y += m_SpaceBetweenWidgets;
		widget.Recalculate( this );
		widget.Update( DT );
	}
}

