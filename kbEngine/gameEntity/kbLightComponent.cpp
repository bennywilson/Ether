//===================================================================================================
// kbLightComponent.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"

KB_DEFINE_COMPONENT(kbLightComponent)
KB_DEFINE_COMPONENT(kbDirectionalLightComponent)

/**
 *	kbLightComponent::Constructor
 */
void kbLightComponent::Constructor() {
	m_Color.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_bCastsShadows = false;
	m_Brightness = 1;
}

/**
 *	kbLightComponent::~kbLightComponent
 */
kbLightComponent::~kbLightComponent() {

}

/**
 *	kbLightComponent::PoadLoad
 */
void kbLightComponent::PostLoad() {
	Super::PostLoad();
}

/**
 *	kbLightComponent::SetEnable_Internal
 */
void kbLightComponent::SetEnable_Internal( const bool bIsEnabled ) {

	Super::SetEnable_Internal( bIsEnabled );

	if ( g_pRenderer != nullptr ) {
		if ( bIsEnabled ) {
			g_pRenderer->AddLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
		} else {
			g_pRenderer->RemoveLight( this );
		}
	}
}

/**
 *	kbLightComponent:Update
 */
void kbLightComponent::Update_Internal( const float DeltaTime ) {

	Super::Update_Internal( DeltaTime );

	if ( GetLifeTimeRemaining() >= 0 ) {
		// Hack fade for grenade
		m_Brightness = kbClamp( GetLifeTimeRemaining() / GetStartingLifeTime(), 0.0f, 1.0f );
		g_pRenderer->UpdateLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
		return;
	}

	if ( IsDirty() ) {
		g_pRenderer->UpdateLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
	}
}

/**
 *	kbPointLightComponent::Constructor
 */
void kbPointLightComponent::Constructor() {
	m_Radius = 16.0f;
}

/**
 *	kbCylindricalLightComponent::Constructor
 */
void kbCylindricalLightComponent::Constructor() {
	m_Length = 32.0f;
}

/**
 *	kbDirectionalLightComponent::Constructor
 */
void kbDirectionalLightComponent::Constructor() {
}

/**
 *	kbDirectionalLightComponent::~kbDirectionalLightComponent
 */
kbDirectionalLightComponent::~kbDirectionalLightComponent() {

}

/**
 *	kbDirectionalLightComponent::EditorChange
 */
void kbDirectionalLightComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );
	// TODO: clamp shadow splits to 4.  Also ensure that the ordering is correct
}

/**
 *	kbLightShaftsComponent::Constructor
 */
void kbLightShaftsComponent::Constructor() {
	m_Texture = ( kbTexture* )g_ResourceManager.GetResource( "../../kbEngine/assets/Textures/Editor/flare.jpg" );
	m_Color.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_BaseWidth = m_BaseHeight = 20.0f;
	m_IterationWidth = m_IterationHeight = 1.0f;
	m_NumIterations = 4;
	m_Directional = true;
}

/**
 *	kbLightShaftsComponent::~kbLightShaftsComponent
 */
kbLightShaftsComponent::~kbLightShaftsComponent() {
}

/**
 *	kbLightShaftsComponent::SetEnable_Internal
 */
void kbLightShaftsComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	if ( g_pRenderer != nullptr ) {
		if ( isEnabled ) {
			g_pRenderer->AddLightShafts( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );	
		} else {
			g_pRenderer->RemoveLightShafts( this );
		}
	}
}

/**
 *	kbLightShaftsComponent::SetColor
 */
void kbLightShaftsComponent::SetColor( const kbColor & newColor ) {
	m_Color = newColor;
	if ( IsEnabled() ) {
		g_pRenderer->UpdateLightShafts( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
	}
}

/**
 *	kbFogComponent::Constructor
 */
void kbFogComponent::Constructor() {
	m_Color.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_StartDistance = 2100;
	m_EndDistance = 2200;
}

/**
 *	kbFogComponent::Update_Internal
 */
void kbFogComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	g_pRenderer->UpdateFog( m_Color, m_StartDistance, m_EndDistance );
}
