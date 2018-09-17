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
 *	kbModelComponent::EditorChange
 */
void kbModelComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	m_RenderObject.m_ShaderParamOverrides.m_ParamOverrides.clear();
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
}

/**
 *	kbModelComponent::PostLoad
 */
void kbModelComponent::PostLoad() {
	Super::PostLoad();

	if ( GetOwner()->IsPrefab() == false ) {
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
	}
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
 *	kbModelComponent::SetShaderVectorParam
 */
void kbModelComponent::SetShaderVectorParam( const std::string & paramName, const kbVec4 & value ) {
	m_RenderObject.m_ShaderParamOverrides.SetVec4( paramName, value );
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
