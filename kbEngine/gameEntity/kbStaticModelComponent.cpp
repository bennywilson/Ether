//===================================================================================================
// kbStaticModelComponent.cpp
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbStaticModelComponent)

/**
 *	kbStaticModelComponent
 */
void kbStaticModelComponent::Constructor() {
	m_pModel = nullptr;
}

/**
 *	~kbStaticModelComponent
 */
kbStaticModelComponent::~kbStaticModelComponent() {
}

/**
 *	kbStaticModelComponent::EditorChange
 */
void kbStaticModelComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( IsEnabled() && ( propertyName == "Model" || propertyName == "ShaderOverride" ) ) {
		SetEnable_Internal( false );
		SetEnable_Internal( true );
	}
}

/**
 *	kbStaticModelComponent::SetEnable_Internal
 */
void kbStaticModelComponent::SetEnable_Internal( const bool isEnabled ) {
	
	Super::SetEnable_Internal( isEnabled );

	if ( m_pModel == nullptr ) {
		return;
	}

	m_RenderObject.m_pComponent = this;

	if ( isEnabled ) {

		m_RenderObject.m_bCastsShadow = this->GetCastsShadow();
		m_RenderObject.m_bIsSkinnedModel = false;
		m_RenderObject.m_EntityId = GetOwner()->GetEntityId();
		m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
		m_RenderObject.m_pModel = m_pModel;
		m_RenderObject.m_Position = GetOwner()->GetPosition();
		m_RenderObject.m_RenderPass = m_RenderPass;
		m_RenderObject.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
		m_RenderObject.m_RenderOrderBias = m_RenderOrderBias;

		RefreshMaterials( false );

		g_pRenderer->AddRenderObject( m_RenderObject );
	} else {
		g_pRenderer->RemoveRenderObject( m_RenderObject );
	}
}

/**
 *	kbStaticModelComponent:Update_Internal
 */
void kbStaticModelComponent::Update_Internal( const float DeltaTime ) {

	Super::Update_Internal( DeltaTime );

	if ( m_pModel != nullptr && GetOwner()->IsDirty() ) {
		m_RenderObject.m_Position = GetOwner()->GetPosition();
		m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
		m_RenderObject.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
		m_RenderObject.m_pModel = m_pModel;

		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}

	// m_pModel->DrawDebugTBN( GetOwner()->GetPosition(), GetOwner()->GetOrientation(), GetOwner()->GetScale() );
}
