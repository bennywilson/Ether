//===================================================================================================
// kbStaticModelComponent.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include "kbModel.h"
#include "kbGameEntityHeader.h"

KB_DEFINE_COMPONENT(kbStaticModelComponent)

/**
 *	kbStaticModelComponent
 */
void kbStaticModelComponent::Constructor() {
	m_pModel = NULL;
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

	if ( propertyName == "Model" || propertyName == "ShaderOverride" ) {
		SetEnable_Internal( false );
		SetEnable_Internal( true );
	}
}

/**
 *	kbStaticModelComponent::SetEnable_Internal
 */
void kbStaticModelComponent::SetEnable_Internal( const bool isEnabled ) {
	
	if ( m_pModel == NULL ) {
		return;
	}

	if ( isEnabled ) {
		g_pRenderer->AddRenderObject( this, m_pModel, m_pParent->GetPosition(), m_pParent->GetOrientation(), m_pParent->GetScale(), m_RenderPass, &m_pOverrideShaderList, &m_ShaderParams );
	} else {
		g_pRenderer->RemoveRenderObject( this );
	}
}

/**
 *	kbStaticModelComponent:Update_Internal
 */
void kbStaticModelComponent::Update_Internal( const float DeltaTime ) {

	Super::Update_Internal( DeltaTime );

	if ( m_pModel != NULL && m_pParent->IsDirty() ) {
		g_pRenderer->UpdateRenderObject( this, m_pModel, m_pParent->GetPosition(), m_pParent->GetOrientation(), m_pParent->GetScale(), m_RenderPass, &m_pOverrideShaderList, &m_ShaderParams );
	}
}