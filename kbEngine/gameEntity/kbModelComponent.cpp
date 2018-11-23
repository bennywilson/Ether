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

	// MaterialHack
//	SetShaderParamList();
}

/**
 *	kbModelComponent::PostLoad
 */
void kbModelComponent::PostLoad() {
	Super::PostLoad();

	/*MATERIALHACK
	if ( GetOwner()->IsPrefab() == false ) {
		SetShaderParamList();
	}*/
}

/**
 *	kbShaderParamComponent::Constructor
 */
void kbShaderParamComponent::Constructor() {
	m_pTexture = nullptr;
	m_Vector.Set( 0.0f, 0.0f, 0.0f, 0.0f );
}

/**
 *	kbMaterialComponent::Constructor
 */
void kbMaterialComponent::Constructor() {
	m_pShader = nullptr;
}

/**
 *	kbMaterialComponent::EditorChange
 */
void kbMaterialComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "Shader" && m_pShader != nullptr ) {

		std::vector<kbShaderParamComponent>	oldParams = m_ShaderParamComponents;
		m_ShaderParamComponents.clear();

		const kbShaderVarBindings_t & shaderBindings = m_pShader->GetShaderVarBindings();
		for ( int i = 0; i < shaderBindings.m_VarBindings.size(); i++ ) {
			auto currentVar = shaderBindings.m_VarBindings[i];
			if ( currentVar.m_bIsUserDefinedVar == false ) {
				continue;
			}

			const kbString boundVarName( currentVar.m_VarName );
			bool boundParamFound = false;
			for ( int iOldParam = 0; iOldParam < oldParams.size(); iOldParam++ ) {
				if ( oldParams[iOldParam].GetParamName() == boundVarName ) {
					m_ShaderParamComponents.push_back( oldParams[iOldParam] );
					boundParamFound = true;
					break;
				}
			}

			if ( boundParamFound == false ) {
				kbShaderParamComponent newParam;
				newParam.SetParamName( boundVarName );
				newParam.SetVector( kbVec4::zero );
				newParam.SetTexture( nullptr );
				m_ShaderParamComponents.push_back( newParam );
			}
		}

		for ( int i = 0; i < shaderBindings.m_Textures.size(); i++ ) {
			const kbString boundTextureName( shaderBindings.m_Textures[i].m_TextureName );
			bool boundParamFound = false;
			for ( int iOldParam = 0; iOldParam < oldParams.size(); iOldParam++ ) {
				if ( oldParams[iOldParam].GetParamName() == boundTextureName ) {
					m_ShaderParamComponents.push_back( oldParams[iOldParam] );
					boundParamFound = true;
					break;
				}
			}

			if ( boundParamFound == false ) {
				kbShaderParamComponent newParam;
				newParam.SetParamName( boundTextureName );
				newParam.SetVector( kbVec4::zero );
				newParam.SetTexture( nullptr );
				m_ShaderParamComponents.push_back( newParam );			
			}
		}
	}
}