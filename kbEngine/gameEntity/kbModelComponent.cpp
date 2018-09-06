//===================================================================================================
// kbModelComponent.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"		// <--- TODO: Temp, game entity should not be accessed from renderer
#include "kbModel.h"
#include "kbModelComponent.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbModelComponent)

/**
 *	kbModelComponent::Constructor
 */
void kbModelComponent::Constructor() {
	m_RenderPass = RP_Lighting;
	m_TranslucencySortBias = 0.0f;
	m_bCastsShadow = false;
}

/**
 *	kbModelComponent::~kbModelComponent
 */
kbModelComponent::~kbModelComponent() {
}

/**
 *	kbModelComponent:SetShaderParams
 */
void kbModelComponent::SetShaderParams( const kbShaderParamOverrides_t & shaderParams ) {
	m_RenderObject.m_ShaderParamOverrides = shaderParams;

	if ( IsEnabled() ) {
		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}
}

/**
 *	kbShaderParamComponent::Constructor
 */
void kbShaderParamComponent::Constructor() {
	m_pTexture = nullptr;
	m_Vector.Set( 0.0f, 0.0f, 0.0f, 0.0f );
}

/**
 *	kbShaderParamListComponent::Constructor
 */
void kbShaderParamListComponent::Constructor() {

}

/**
 *	kbShaderParamListComponent::EditorChange
 */
void kbShaderParamListComponent::EditorChange( const std::string & propertyName ) {
	UpdateModelWithParams();
}

/**
 *	kbShaderParamListComponent::Enable
 */
void kbShaderParamListComponent::Enable( const bool setEnabled ) {
	Super::Enable( setEnabled );

	if ( setEnabled ) {
		UpdateModelWithParams();
	}
}

/**
 *	kbShaderParamListComponent::UpdateModelWithParams
 */								
void kbShaderParamListComponent::UpdateModelWithParams() {
	kbGameEntity *const pGameEntity = GetOwner();
	kbModelComponent *const pModelComponent = (kbModelComponent *)GetOwner()->GetComponentByType( kbModelComponent::GetType() );
	if ( pModelComponent == nullptr ) {
		kbWarning( "kbShaderParamListComponent::UpdateModelWithParams() - Missing model component" );
		return;
	}

	kbShaderParamOverrides_t shaderOverride;

	for ( int i = 0; i < m_ShaderParamList.size(); i++ ) {
		kbShaderParamComponent & curParam = m_ShaderParamList[i];
		const std::string & paramName = curParam.m_ParamName.stl_str();
		if ( paramName.empty() ) {
			continue;
		}

		if ( curParam.m_pTexture != nullptr ) {
			shaderOverride.SetTexture( paramName, curParam.m_pTexture );
		} else {
			shaderOverride.SetVec4( paramName, curParam.m_Vector );
		}
	}

	pModelComponent->SetShaderParams( shaderOverride );
}