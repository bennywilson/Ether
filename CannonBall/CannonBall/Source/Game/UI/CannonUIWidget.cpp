//===================================================================================================
// CannonUIWidget.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "UI/CannonUI.h"

/**
 *	CannonUIWidget::Constructor
 */
void CannonUIWidget::Constructor() {

	m_StartingPosition.Set( 0.0f, 0.0f, 0.0f );
	m_StartingSize.Set( 0.5f, 0.5f, 1.0f );

	m_RelativePosition.Set( 0.0f, 0.0f, 0.0f );
	m_RelativeSize.Set( 0.5f, 0.5f, 1.0f );

	m_AbsolutePosition.Set( 0.0f, 0.0f, 0.0f );
	m_AbsoluteSize.Set( 0.5f, 0.5f, 1.0f );

	m_pModel = nullptr;
}

/**
 *	CannonUIWidget::RegisterEventListener
 */
void CannonUIWidget::RegisterEventListener( IUIWidgetListener * pListener ) {

	m_EventListeners.push_back( pListener );
}

/**
 *	CannonUIWidget::UnregisterEventListener
 */
void CannonUIWidget::UnregisterEventListener( IUIWidgetListener * pListener ) {

	VectorRemoveFast( m_EventListeners, pListener );
}

/**
 *	CannonUIWidget::UnregisterEventListener
 */
void CannonUIWidget::FireEvent() {

	for ( int i = 0; i < m_EventListeners.size(); i++ ) {
		m_EventListeners[i]->WidgetEventCB( this );
	}
}


/**
 *	CannonUIWidget::RecalculateOld
 */
void CannonUIWidget::SetRelativePosition( const kbVec3 & newPos ) {

	m_RelativePosition = newPos;

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;
}

/**
 *	CannonUIWidget::SetRelativeSize
 */
void CannonUIWidget::SetRelativeSize( const kbVec3 & newSize ) {

	m_RelativeSize = newSize;

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;
}

/**
 *	CannonUIWidget::RecalculateOld
 */
void CannonUIWidget::RecalculateOld( const kbUIComponent *const pParent ) {

	kbErrorCheck( pParent != nullptr, "CannonUIWidget::UpdateFromParent() - null parent" );
	
	if ( m_pModel != nullptr && pParent != nullptr && pParent->GetStaticModelComponent() != nullptr ) {
		m_pModel->SetRenderOrderBias( pParent->GetStaticModelComponent()->GetRenderOrderBias() - 1.0f );
	}

	m_CachedParentPosition = pParent->GetNormalizedAnchorPt();
	m_CachedParentSize = pParent->GetNormalizedScreenSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

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

	m_CachedParentPosition = pParent->GetAbsolutePosition();
	m_CachedParentSize = pParent->GetAbsoluteSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

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
	
	m_SliderBoundsMin.Set( 0.0f, 0.0f, 0.0f );
	m_SliderBoundsMax.Set( 1.0f, 1.0f, 1.0f );

	m_CalculatedSliderBoundsMin.Set( 0.0f, 0.0f, 0.0f );
	m_CalculatedSliderBoundsMax.Set( 1.0f, 1.0f, 1.0f );
}

/**
 *	CannonUISlider::SetEnable_Internal
 */
void CannonUISlider::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );
}

/**
 *	CannonUISlider::RecalculateOld
 */
void CannonUISlider::RecalculateOld( const kbUIComponent *const pParent ) {

	Super::RecalculateOld( pParent );
	m_CalculatedSliderBoundsMin = m_ChildWidgets[0].GetRelativeSize() * m_SliderBoundsMin + m_ChildWidgets[1].GetRelativePosition();
	m_CalculatedSliderBoundsMax = m_ChildWidgets[0].GetRelativeSize() * m_SliderBoundsMax + m_ChildWidgets[1].GetRelativePosition();

	kbLog( "Calculated Bounds is %f %f %f, %f %f %f", m_CalculatedSliderBoundsMin.x, m_CalculatedSliderBoundsMin.y, m_CalculatedSliderBoundsMin.z, m_CalculatedSliderBoundsMax.x, m_CalculatedSliderBoundsMax.y, m_CalculatedSliderBoundsMax.z );

}

/**
 *	CannonUISlider::Recalculate
 */
void CannonUISlider::Recalculate( const CannonUIWidget *const pParent ) {

	Super::Recalculate( pParent );

		kbLog( "2. Calculated Bounds is %f %f %f, %f %f %f", m_CalculatedSliderBoundsMin.x, m_CalculatedSliderBoundsMin.y, m_CalculatedSliderBoundsMin.z, m_CalculatedSliderBoundsMax.x, m_CalculatedSliderBoundsMax.y, m_CalculatedSliderBoundsMax.z );

	m_CalculatedSliderBoundsMin = m_ChildWidgets[0].GetRelativeSize() * m_SliderBoundsMin + m_ChildWidgets[1].GetRelativePosition();
	m_CalculatedSliderBoundsMax = m_ChildWidgets[0].GetRelativeSize() * m_SliderBoundsMax + m_ChildWidgets[1].GetRelativePosition();
}

/**
 *	CannonUISlider::Update_Internal
 */
void CannonUISlider::Update_Internal( const float dt ) {

	Super::Update_Internal( dt );

	kbVec3 curPos = m_ChildWidgets[1].GetRelativePosition();
	bool bMove = 0.0f;

	bool bFireEvent = false;
	if ( GetAsyncKeyState( VK_LEFT ) ) {
		curPos.x -= 0.02f;
		bFireEvent = true;
	}

		if ( GetAsyncKeyState( VK_RIGHT ) ) {
		curPos.x += 0.02f;
		bFireEvent = true;
	}

	curPos.x = kbClamp( curPos.x, m_CalculatedSliderBoundsMin.x, m_CalculatedSliderBoundsMax.x );
	m_ChildWidgets[1].SetRelativePosition( curPos );

	if ( bFireEvent ) {
		FireEvent();
	}
	//kbLog( "CannonUISlider Child = %f %f %f, %f %f %f", GetAbsolutePosition().x, GetAbsolutePosition().y, GetAbsolutePosition().z, GetAbsoluteSize().x, GetAbsoluteSize().y, GetAbsoluteSize().z );
}


/**
 *	CannonUISlider::GetNormalizedValue
 */
float CannonUISlider::GetNormalizedValue() {

	if ( m_ChildWidgets.size() < 2 ) {
		return 0.0f;
	}

	const float sliderPos = m_ChildWidgets[1].GetRelativePosition().x;
	return ( sliderPos - m_CalculatedSliderBoundsMin.x ) / ( m_CalculatedSliderBoundsMax.x - m_CalculatedSliderBoundsMin.x );
}


/**
 *	CannonUISlider::SetNormalizedValue
 */
void CannonUISlider::SetNormalizedValue( const float newValue ) {

	if ( m_ChildWidgets.size() < 2 ) {
		return;
	}

	kbVec3 relativePos = m_ChildWidgets[1].GetRelativePosition();
	relativePos.x = m_CalculatedSliderBoundsMin.x + ( m_CalculatedSliderBoundsMax.x - m_CalculatedSliderBoundsMin.x ) * newValue;
	m_ChildWidgets[1].SetRelativePosition( relativePos );
}
