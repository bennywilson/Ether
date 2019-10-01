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

	const kbVec2 normalizedScreenSize = GetNormalizedScreenSize();
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
