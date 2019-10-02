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
					screenPos.x + ( 1.0f - randomScaleFactor.x ) * 0.5f * screenSize.x,
					screenPos.y + ( 1.0f - randomScaleFactor.y ) * 0.5f * screenSize.y ) );
	}

	if ( m_pSmokeModel != nullptr ) {

		const kbVec2 screenSize = kbVec2( normalizedScreenSize.x * m_SmokeRelativeSize.x, normalizedScreenSize.y * m_SmokeRelativeSize.y );
		const kbVec2 screenPos = normalizedAnchorPt + kbVec2( GetNormalizedScreenSize().x * m_SmokeRelativePosition.x, GetNormalizedScreenSize().y * m_SmokeRelativePosition.y );
		m_pSmokeModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
							kbVec4( screenSize.x, screenSize.y, screenPos.x + ( 1.0f - 1.0f ) * 0.5f * screenSize.x, screenPos.y + ( 1.0f - 1.0f ) * 0.5f * screenSize.y ) );
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
				randomOffset.Set( ( kbfrand() - 0.5f ) * normalizedScreenSize.x * 0.5f, ( kbfrand() - 0.5f ) * normalizedScreenSize.x * 0.5f );
			}
			kbVec4 textureMask = kbVec4::zero;
			textureMask[idx] = 1.0f;

			static kbString channelMask( "channelMask" );
			m_pSmokeModel->SetMaterialParamVector( 0, channelMask.stl_str(), textureMask );
		}

		const kbVec2 screenSize = kbVec2( normalizedScreenSize.x * m_BoomRelativeSize.x, normalizedScreenSize.y * m_BoomRelativeSize.y );
		const kbVec2 screenPos = normalizedAnchorPt + kbVec2( GetNormalizedScreenSize().x * m_BoomRelativePosition.x, GetNormalizedScreenSize().y * m_BoomRelativePosition.y );
		m_pBoomModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
							kbVec4( screenSize.x, screenSize.y, screenPos.x + randomOffset.x, screenPos.y + randomOffset.y ) );
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

	m_pModel = nullptr;
	m_pParent = nullptr;
}

/**
 *	CannonUIWidget::SetParent
 */
void CannonUIWidget::SetParent( const kbUIComponent *const pParent ) {

	kbErrorCheck( pParent != nullptr, "CannonUIWidget::UpdateFromParent() - null parent" );

	m_pParent = pParent;
	
	if ( m_pModel != nullptr && pParent != nullptr && pParent->GetStaticModelComponent() != nullptr ) {
		m_pModel->SetRenderOrderBias( m_pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );
	}
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
			m_GameEntity.AddComponent( m_pModel );
		}

		m_pModel->SetModel( pUnitQuad );
		m_pModel->SetMaterials( m_Materials );
		m_pModel->SetRenderPass( RP_UI );
		m_pModel->Enable( false );
		m_pModel->Enable( true );
	} else {
		if ( m_pModel != nullptr ) {
			m_pModel->Enable( false );
			//delete m_pModel;
			//m_pModel = nullptr;
		}
	}
}

/**
 *	CannonUIWidget::Update_Internal
 */
void CannonUIWidget::Update_Internal( const float dt ) {

	static const kbString normalizedScreenSize_Anchor( "normalizedScreenSize_Anchor" );

	const kbVec3 parentStart = m_pParent->GetNormalizedAnchorPt();
	const kbVec3 parentEnd = parentStart + m_pParent->GetNormalizedScreenSize();
	const kbVec3 widgetAbsPos = parentStart + ( parentEnd - parentStart ) * m_RelativePosition;
	const kbVec3 widgetAbsSize = m_pParent->GetNormalizedScreenSize() * m_RelativeSize;

	m_pModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), kbVec4( widgetAbsSize.x, widgetAbsSize.y, widgetAbsPos.x, widgetAbsPos.y ) );
	m_pModel->SetRenderOrderBias( m_pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );
	m_pModel->RefreshMaterials( true );
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
			m_Widgets[i].SetParent( this );
			m_Widgets[i].Enable( false );
			m_Widgets[i].Enable( true );
		}
	} else {
		for ( int i = 0; i < m_Widgets.size(); i++ ) {
			m_Entity.RemoveComponent( &m_Widgets[i] );
			m_Widgets[i].SetParent( this );
			m_Widgets[i].Enable( false );
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

	kbVec3 nextPos = this->m_StartingWidgetAnchorPt;
	for ( size_t i = 0; i < m_Widgets.size(); i++ ) {
		m_Widgets[i].SetRelativeSize( m_WidgetSize );
		m_Widgets[i].SetRelativePosition( nextPos );
		nextPos.y += m_SpaceBetweenWidgets;

		m_Widgets[i].Update( DT );
	}
/*	
	const static kbString baseTexture( "baseTexture" );
	static const kbString normalizedScreenSize_Anchor( "normalizedScreenSize_Anchor" );
	float nexty = 0.0f;

	for ( int i = 0; i < NumOptions; i++ ) {

		kbStaticModelComponent *const pNewOptionModel = m_OptionModels[i];
		pNewOptionModel->SetMaterialParamTexture( 0, baseTexture.stl_str(), m_OptionImages[i] );
		pNewOptionModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), kbVec4( 0.1f, 0.1f, 0.5f, nexty ) );
		nexty += 0.1f;
	}*/
}
