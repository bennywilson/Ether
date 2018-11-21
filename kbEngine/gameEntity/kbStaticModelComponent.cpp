//===================================================================================================
// kbStaticModelComponent.cpp
//
//
// 2016-2018 kbEngine 2.0
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
		m_RenderObject.m_Scale = GetOwner()->GetScale();
		m_RenderObject.m_OverrideShaderList = m_pOverrideShaderList;
		m_RenderObject.m_TranslucencySortBias = m_TranslucencySortBias;

		for ( int i = 0; i < m_ShaderParamList.size(); i++ ) {
			if ( m_ShaderParamList[i].GetParamName().stl_str().empty() ) {
				continue;
			}

			if ( m_ShaderParamList[i].GetTexture() != nullptr ) {
				m_RenderObject.m_ShaderParamOverrides.SetTexture( m_ShaderParamList[i].GetParamName().stl_str(), m_ShaderParamList[i].GetTexture() );
			} else {
				m_RenderObject.m_ShaderParamOverrides.SetVec4( m_ShaderParamList[i].GetParamName().stl_str(), m_ShaderParamList[i].GetVector() );

			}
		}

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
		m_RenderObject.m_Scale = GetOwner()->GetScale();
		m_RenderObject.m_pModel = m_pModel;

		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}

	// m_pModel->DrawDebugTBN( GetOwner()->GetPosition(), GetOwner()->GetOrientation(), GetOwner()->GetScale() );
}
