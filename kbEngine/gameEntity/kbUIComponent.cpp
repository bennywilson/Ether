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
		RefreshMaterial();
	} else {
		m_pStaticModelComponent = nullptr;
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

	static kbString normalizedScreenSize_Anchor( "normalizedScreenSize_Anchor" );
	m_pStaticModelComponent->SetMaterialParamVector( 0, normalizedScreenSize_Anchor.stl_str(), 
		kbVec4( m_NormalizedScreenSize.x,
			    m_NormalizedScreenSize.y,
				m_NormalizedAnchorPt.x,
				m_NormalizedAnchorPt.y ) );
}