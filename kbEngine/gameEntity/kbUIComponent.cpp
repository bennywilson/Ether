//===================================================================================================
// kbUIComponent.cpp
//
//
// 2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbUIComponent.h"
#include "kbRenderer.h"
#include "kbInputManager.h"

kbGameEntity & GetUIGameEntity() {
	static kbGameEntity m_GameEnt;
	return m_GameEnt;
}

/**
 *	kbUIComponent::Constructor
 */
void kbUIComponent::Constructor() {
	m_AuthoredWidth = 128;
	m_AuthoredHeight = 128;
	m_NormalizedAnchorPt.Set( 0.05f, 0.05f, 0.0f );
	m_UIToScreenSizeRatio.Set( 0.1f, 0.0f, 0.0f );
	m_NormalizedScreenSize.Set( 0.f, 0.0f, 0.0f );

	m_pStaticModelComponent = nullptr;
}

/**
 *	kbUIComponent::~kbUIComponent
 */
kbUIComponent::~kbUIComponent() {
	m_AuthoredWidth = 128;
	m_AuthoredHeight = 128;
	m_NormalizedAnchorPt.Set( 0.05f, 0.05f, 0.0f );
	m_UIToScreenSizeRatio.Set( 0.1f, 0.0f, 0.0f );
}

/**
 *	kbUIComponent::EditorChange
 */
void kbUIComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );
	FindStaticModelComponent();
	RefreshMaterial();
}

/**
 *	kbUIComponent::SetEnable_Internal
 */
void kbUIComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		FindStaticModelComponent();

		if ( m_pStaticModelComponent != nullptr ) {
			m_pStaticModelComponent->Enable( true );
		}
		RefreshMaterial();
	} else {

		if ( m_pStaticModelComponent != nullptr ) {
			m_pStaticModelComponent->Enable( false );
			m_pStaticModelComponent = nullptr;
		}
	}
}

/**
 *	kbUIComponent:FindStaticModelComponent
 */
void kbUIComponent::FindStaticModelComponent() {
	m_pStaticModelComponent = GetOwner()->GetComponent<kbStaticModelComponent>();
}

/**
 *	kbUIComponent:RefreshMaterial
 */

void kbUIComponent::RefreshMaterial() {

	FindStaticModelComponent();
	if ( m_pStaticModelComponent == nullptr ) {
		return;
	}

	const float ScreenPixelWidth = (float)g_pRenderer->GetBackBufferWidth();
	const float ScreenPixelHeight = (float)g_pRenderer->GetBackBufferHeight();

	const float aspectRatio = (float)GetAuthoredWidth() / (float) GetAuthoredHeight();
	m_NormalizedScreenSize.x = GetUIToScreenSizeRatio().x;
	const float screenWidthPixel = m_NormalizedScreenSize.x * ScreenPixelWidth;
	const float screenHeightPixel = screenWidthPixel / aspectRatio;
	m_NormalizedScreenSize.y = screenHeightPixel / ScreenPixelHeight;
	m_NormalizedScreenSize.z = 1.0f;

//	kbLog( "%f %f %f %f", m_NormalizedScreenSize.x, m_NormalizedScreenSize.y,m_NormalizedAnchorPt.x - m_NormalizedScreenSize.x * 0.5f,m_NormalizedAnchorPt.y - m_NormalizedScreenSize.y * 0.5f);
	static kbString normalizedScreenSize_Anchor( "normalizedScreenSize_Anchor" );

	const kbVec4 sizeAndPos = kbVec4( m_NormalizedScreenSize.x,
								m_NormalizedScreenSize.y,
								m_NormalizedAnchorPt.x + m_NormalizedScreenSize.x * 0.5f,		// Upper left corner to anchor
								m_NormalizedAnchorPt.y + m_NormalizedScreenSize.y * 0.5f );		// Upper left corner to anchor

	m_pStaticModelComponent->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), sizeAndPos );
}


/**
 *	kbUIWidget::Constructor
 */
void kbUIWidget::Constructor() {

	m_StartingPosition.Set( 0.0f, 0.0f, 0.0f );
	m_StartingSize.Set( 0.5f, 0.5f, 1.0f );

	m_Anchor = kbUIWidget::MiddleLeft;
	m_AxisLock = kbUIWidget::LockAll;

	m_RelativePosition.Set( 0.0f, 0.0f, 0.0f );
	m_RelativeSize.Set( 0.5f, 0.5f, 1.0f );

	m_AbsolutePosition.Set( 0.0f, 0.0f, 0.0f );
	m_AbsoluteSize.Set( 0.5f, 0.5f, 1.0f );

	m_pModel = nullptr;

	m_bHasFocus = false;
}

/**
 *	kbUIWidget::RegisterEventListener
 */
void kbUIWidget::RegisterEventListener( IUIWidgetListener *const pListener ) {
	m_EventListeners.push_back( pListener );
}

/**
 *	kbUIWidget::UnregisterEventListener
 */
void kbUIWidget::UnregisterEventListener( IUIWidgetListener *const pListener ) {
	VectorRemoveFast( m_EventListeners, pListener );
}

/**
 *	kbUIWidget::SetAdditiveTextureFactor
 */
void kbUIWidget::SetAdditiveTextureFactor( const float factor ) {

	static const kbString additiveTextureParams( "additiveTextureParams" );
	m_pModel->SetMaterialParamVector( 0, additiveTextureParams.stl_str(), kbVec4( factor, 0.0f, 0.0f, 0.0f ) );
}

/**
 *	kbUIWidget::FireEvent
 */
void kbUIWidget::FireEvent() {

	for ( int i = 0; i < m_EventListeners.size(); i++ ) {
		m_EventListeners[i]->WidgetEventCB( this );
	}
}

/**
 *	kbUIWidget::EditorChange
 */
void kbUIWidget::EditorChange( const std::string & propertyName ) {

	Super::EditorChange( propertyName );

}

/**
 *	kbUIComponent::SetFocus
 */
void kbUIWidget::SetFocus( const bool bHasFocus ) {
	m_bHasFocus = bHasFocus;
}

/**
 *	kbUIWidget::SetRelativePosition
 */
void kbUIWidget::SetRelativePosition( const kbVec3 & newPos ) {
	m_RelativePosition = newPos;
	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;

	for ( size_t i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Recalculate( this, false );
	}
}

/**
 *	kbUIWidget::SetRelativeSize
 */
void kbUIWidget::SetRelativeSize( const kbVec3 & newSize ) {

	m_RelativeSize = newSize;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	for ( size_t i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Recalculate( this, false );
	}
}

/**
 *	kbUIWidget::RecalculateOld
 */
void kbUIWidget::RecalculateOld( const kbUIComponent *const pParent, const bool bFull ) {
	kbErrorCheck( pParent != nullptr, "kbUIWidget::UpdateFromParent() - null parent" );
	
/*	if ( m_pModel != nullptr && pParent != nullptr && pParent->GetStaticModelComponent() != nullptr ) {
		kbLog( "Setting render oreder bias to %f", pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );
		m_pModel->SetRenderOrderBias( pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );
	}*/

	m_CachedParentPosition = pParent->GetNormalizedAnchorPt();
	m_CachedParentSize = pParent->GetNormalizedScreenSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	SetRenderOrderBias( pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );

	for ( int i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].RecalculateOld( pParent, bFull );
	}
}

/**
 *	kbUIWidget::Recalculate
 */
void kbUIWidget::Recalculate( const kbUIWidget *const pParent, const bool bFull ) {

	if ( pParent != nullptr ) {
		m_CachedParentPosition = pParent->GetAbsolutePosition();
		m_CachedParentSize = pParent->GetAbsoluteSize();
		SetRenderOrderBias( pParent->GetRenderOrderBias() - 1.0f );
	} else {
		m_CachedParentPosition = kbVec3::zero;
		m_CachedParentSize = kbVec3::one;
		SetRenderOrderBias( 0.0f );
	}

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	for ( int i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Recalculate( this, bFull );
	}
}

/**
 *	kbUIWidget::SetRenderOrderBias
 */
void kbUIWidget::SetRenderOrderBias( const float bias ) {

	if ( m_pModel != nullptr ) {
		m_pModel->SetRenderOrderBias( bias );
	}
}

/**
 *	kbUIWidget::GetRenderOrderBias
 */
float kbUIWidget::GetRenderOrderBias() const {

	if ( m_pModel == nullptr ) {
		return 0.0f;
	}

	return m_pModel->GetRenderOrderBias();
}

/**
 *	kbUIWidget::GetBaseTextureDimensions
 */
kbVec2i	kbUIWidget::GetBaseTextureDimensions() const {

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
 *	kbUIWidget::SetEnable_Internal
 */
void kbUIWidget::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );

	static kbModel * pUnitQuad = nullptr;
	if ( pUnitQuad == nullptr ) {
		pUnitQuad = (kbModel*)g_ResourceManager.GetResource( "../../kbEngine/assets/Models/UnitQuad.ms3d", true, true );
	}

	if ( GetOwner() == nullptr ) {
		return;
	}

	if ( bEnable ) {

		m_RelativePosition = m_StartingPosition;
		m_RelativeSize = m_StartingSize;

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
			GetUIGameEntity().AddComponent( &m_ChildWidgets[i] );		// Note these children are responsible for removing themselves when disabled (see code block below)
			m_ChildWidgets[i].Enable( false );
			m_ChildWidgets[i].Enable( true );
		}


	} else {
		if ( m_pModel != nullptr ) {
			m_pModel->Enable( false );
		}

		for ( size_t i = 0; i < m_ChildWidgets.size(); i++ ) {
			m_ChildWidgets[i].Enable( false );
		}

		GetUIGameEntity().RemoveComponent( this );
		GetUIGameEntity().RemoveComponent( m_pModel );
	}
}

/**
 *	kbUIWidget::Update_Internal
 */
void kbUIWidget::Update_Internal( const float dt ) {

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
		if ( pTex != nullptr ) {
			aspectRatio = (float)pTex->GetWidth() / (float)pTex->GetHeight();
		}
	}

	const float BackBufferWidth = (float)g_pRenderer->GetBackBufferWidth();
	const float BackBufferHeight = (float)g_pRenderer->GetBackBufferHeight();

	if ( m_AxisLock == LockYAxis ) {
		const float widgetPixelHeight = widgetAbsSize.y * BackBufferHeight;
		const float widgetPixelWidth = widgetPixelHeight * aspectRatio;
		widgetAbsSize.x = widgetPixelWidth / BackBufferWidth;
	}

	if ( m_Anchor == kbUIWidget::MiddleRight ) {
		widgetAbsPos.x -= widgetAbsSize.x;
	}

	m_pModel->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
			kbVec4( widgetAbsSize.x, 
					widgetAbsSize.y,
					widgetAbsPos.x + widgetAbsSize.x * 0.5f,
					widgetAbsPos.y + widgetAbsSize.y * 0.5f ) );

	m_pModel->RefreshMaterials( true );

	for ( size_t i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Update_Internal( dt );
	}

	if ( HasFocus() ) {
		const kbInput_t & input = g_pInputManager->GetInput();
		if ( input.GamepadButtonStates[12].m_Action == kbInput_t::KA_JustPressed|| input.WasNonCharKeyJustPressed( kbInput_t::Return ) ) {
			FireEvent();
		}
	}
}

/**
 *	kbUISlider::Constructor
 */
void kbUISlider::Constructor() {
	
	m_SliderBoundsMin.Set( 0.0f, 0.0f, 0.0f );
	m_SliderBoundsMax.Set( 1.0f, 1.0f, 1.0f );

	m_CalculatedSliderBoundsMin.Set( 0.0f, 0.0f, 0.0f );
	m_CalculatedSliderBoundsMax.Set( 1.0f, 1.0f, 1.0f );
}

/**
 *	kbUISlider::SetEnable_Internal
 */
void kbUISlider::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );
}

/**
 *	kbUISlider::RecalculateOld
 */
void kbUISlider::RecalculateOld( const kbUIComponent *const pParent, const bool bFull ) {

	kbErrorCheck( pParent != nullptr, "kbUIWidget::UpdateFromParent() - null parent" );
	
	if ( m_pModel != nullptr && pParent != nullptr && pParent->GetStaticModelComponent() != nullptr ) {

		m_pModel->SetRenderOrderBias( pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );
	}

	m_CachedParentPosition = pParent->GetNormalizedAnchorPt();
	m_CachedParentSize = pParent->GetNormalizedScreenSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	SetRenderOrderBias( pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );

	for ( int i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].RecalculateOld( pParent, bFull );

		m_ChildWidgets[i].SetRenderOrderBias( pParent->GetStaticModelComponent()->GetRenderOrderBias() - ( 1.0f + (float)i) );
	}

	const float spaceBetweenLabelAndSlider = 0.05f;

	if ( bFull ) {
		kbVec3 pos = m_RelativePosition;
		pos.x = m_RelativePosition.x + m_RelativeSize.x + spaceBetweenLabelAndSlider;
		m_ChildWidgets[1].SetRelativePosition( pos );
	} else {
		kbVec3 pos = m_ChildWidgets[0].GetRelativePosition();
		pos.y = m_RelativePosition.y;

		pos = m_ChildWidgets[1].GetRelativePosition();
		pos.y = m_RelativePosition.y;
		m_ChildWidgets[1].SetRelativePosition( pos );
	}
	
	if ( m_ChildWidgets.size() < 2 ) {
		m_CalculatedSliderBoundsMin.Set( 0.0f, 0.0f, 0.0f );
		m_CalculatedSliderBoundsMax.Set( 0.0f, 0.0f, 0.0f );
	} else {
		m_CalculatedSliderBoundsMin = GetRelativePosition() + spaceBetweenLabelAndSlider;
		m_CalculatedSliderBoundsMax = m_CalculatedSliderBoundsMin + m_ChildWidgets[0].GetRelativeSize() * 0.9f;	// Hack

		m_ChildWidgets[0].SetRelativePosition( GetRelativePosition() + kbVec3( spaceBetweenLabelAndSlider, 0.0f, 0.0f ) );

		if ( bFull ) {
			m_ChildWidgets[1].SetRelativePosition( GetRelativePosition() + kbVec3( GetRelativeSize().x + 0.05f, 0.0f, 0.0f ) );
		}
	}
}

/**
 *	kbUISlider::Recalculate
 */
void kbUISlider::Recalculate( const kbUIWidget *const pParent, const bool bFull ) {

	kbErrorCheck( pParent != nullptr, "kbUIWidget::UpdateFromParent() - null parent" );

	if ( m_pModel != nullptr && pParent != nullptr && pParent->GetStaticModel() != nullptr ) {
		m_pModel->SetRenderOrderBias( pParent->GetStaticModel() ->GetRenderOrderBias() - 1.0f );
				kbLog( "Slider: Setting render oreder bias to %f", pParent->GetStaticModel()->GetRenderOrderBias() - 1.0f );

	}

	m_CachedParentPosition = pParent->GetAbsolutePosition();
	m_CachedParentSize = pParent->GetAbsoluteSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	for ( int i = 0; i < m_ChildWidgets.size(); i++ ) {
		m_ChildWidgets[i].Recalculate( pParent, bFull );
	}

	if ( bFull ) {
		kbVec3 pos = m_RelativePosition;
		pos.x = m_RelativePosition.x + m_RelativeSize.x + 0.05f;

		m_ChildWidgets[0].SetRelativePosition( pos );
		m_ChildWidgets[1].SetRelativePosition( pos );
	} else {
		kbVec3 pos = m_ChildWidgets[0].GetRelativePosition();
		pos.y = m_RelativePosition.y;

		pos = m_ChildWidgets[1].GetRelativePosition();
		pos.y = m_RelativePosition.y;
		m_ChildWidgets[1].SetRelativePosition( pos );
	}

	if ( m_ChildWidgets.size() < 2 ) {
		m_CalculatedSliderBoundsMin.Set( 0.0f, 0.0f, 0.0f );
		m_CalculatedSliderBoundsMax.Set( 0.0f, 0.0f, 0.0f );
	} else {
		m_CalculatedSliderBoundsMin = GetRelativePosition() + GetRelativeSize() + 0.05f;
		m_CalculatedSliderBoundsMax = m_CalculatedSliderBoundsMin + m_ChildWidgets[0].GetRelativeSize();

		m_ChildWidgets[0].SetRelativePosition( GetRelativePosition() + kbVec3( GetRelativeSize().x + 0.05f, 0.0f, 0.0f ) );

		if ( bFull ) {
			m_ChildWidgets[1].SetRelativePosition( GetRelativePosition() + kbVec3( GetRelativeSize().x + 0.05f, 0.0f, 0.0f ) );
		}
	}
}

/**
 *	kbUISlider::Update_Internal
 */
void kbUISlider::Update_Internal( const float dt ) {

	Super::Update_Internal( dt );

	if ( HasFocus() ) {
		if ( m_ChildWidgets.size() > 1 ) {
			kbVec3 curPos = m_ChildWidgets[1].GetRelativePosition();
			bool bMove = 0.0f;

			bool bFireEvent = false;
			const kbInput_t & input = g_pInputManager->GetInput();
			if ( input.IsArrowPressedOrDown( kbInput_t::Left ) || input.IsKeyPressedOrDown( 'A' ) || input.m_LeftStick.x < -0.5f ) {
				curPos.x -= 0.01f;
				bFireEvent = true;
			}

			if ( input.IsArrowPressedOrDown( kbInput_t::Right ) || input.IsKeyPressedOrDown( 'D' ) || input.m_LeftStick.x > 0.5f ) {
				curPos.x += 0.01f;
				bFireEvent = true;
			}

			curPos.x = kbClamp( curPos.x, m_CalculatedSliderBoundsMin.x, m_CalculatedSliderBoundsMax.x );
			m_ChildWidgets[1].SetRelativePosition( curPos );

			if ( bFireEvent ) {
				FireEvent();
			}
		}
	}
}


/**
 *	kbUISlider::GetNormalizedValue
 */
float kbUISlider::GetNormalizedValue() {

	if ( m_ChildWidgets.size() < 2 ) {
		return 0.0f;
	}

	const float sliderPos = m_ChildWidgets[1].GetRelativePosition().x;
	return ( sliderPos - m_CalculatedSliderBoundsMin.x ) / ( m_CalculatedSliderBoundsMax.x - m_CalculatedSliderBoundsMin.x );
}


/**
 *	kbUISlider::SetNormalizedValue
 */
void kbUISlider::SetNormalizedValue( const float newValue ) {

	if ( m_ChildWidgets.size() < 2 ) {
		return;
	}


	kbVec3 relativePos = m_ChildWidgets[1].GetRelativePosition();
	relativePos.x = m_CalculatedSliderBoundsMin.x + ( m_CalculatedSliderBoundsMax.x - m_CalculatedSliderBoundsMin.x ) * newValue;

	m_ChildWidgets[1].SetRelativePosition( relativePos );
}
