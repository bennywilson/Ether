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

	m_RenderObject.m_bCastsShadow = this->GetCastsShadow();
	m_RenderObject.m_bIsSkinnedModel = false;
	m_RenderObject.m_EntityId = GetOwner()->GetEntityId();
	m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
	m_RenderObject.m_Position = GetOwner()->GetPosition();
	m_RenderObject.m_RenderPass = m_RenderPass;
	m_RenderObject.m_Scale = GetOwner()->GetScale();
	m_RenderObject.m_TranslucencySortBias = m_TranslucencySortBias;

	if ( propertyName == "Materials" ) {
		for ( int i = 0; i < this->m_MaterialList.size(); i++ ) {
			m_MaterialList[i].SetOwningComponent( this );
		}
	}

	RefreshMaterials( true );
}

/**
 *	kbModelComponent::PostLoad
 */
void kbModelComponent::PostLoad() {
	Super::PostLoad();

	if ( GetOwner()->IsPrefab() == false ) {
		RefreshMaterials( false );
	}
}

/**
 *	kbModelComponent::SetNewMaterials
 */
void kbModelComponent::SetMaterial( const int idx, const kbMaterialComponent & newMats ) {
	if ( idx < 0 || idx > 32 ) {
		kbWarning( "kbModelComponent::SetMaterial() - Invalid index." );
		return;
	}

	if ( idx >= m_MaterialList.size() ) {
		m_MaterialList.resize( idx + 1 );
	}

	m_MaterialList[idx] = newMats;

	RefreshMaterials( false );

	if ( IsEnabled() ) {
		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}
}

/**
 *	kbModelComponent::RefreshMaterials
 */
void kbModelComponent::RefreshMaterials( const bool bRefreshRenderObejct ) {
	m_RenderObject.m_Materials.clear();
	for ( int i = 0; i < m_MaterialList.size(); i++ ) {
		kbMaterialComponent & matComp = m_MaterialList[i];
	
		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_pShader = matComp.GetShader();
	
		auto srcShaderParams = matComp.GetShaderParams();
		for ( int j = 0; j < srcShaderParams.size(); j++ ) {
			if ( srcShaderParams[j].GetTexture() != nullptr ) {
				newShaderParams.SetTexture( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetTexture() );
			} else if ( srcShaderParams[j].GetRenderTexture() != nullptr ) {
	
				newShaderParams.SetTexture( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetRenderTexture() );
			} else {
				newShaderParams.SetVec4( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetVector() );
			}
		}
	
		m_RenderObject.m_Materials.push_back( newShaderParams );
	}

	if ( IsEnabled() && m_RenderObject.m_pComponent != nullptr && bRefreshRenderObejct ) {
		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}
}

/**
 *	kbModelComponent:SetMaterialParamVector
 */
void kbModelComponent::SetMaterialShader( const int idx, kbShader *const pShader ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		kbWarning( "kbModelComponent::SetMaterialParamVector() called on invalid index" );
		return;
	}

	m_MaterialList[idx].SetShader( pShader );

	if ( IsEnabled() ) {
		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}
}

/**
 *	kbModelComponent:SetMaterialParamVector
 */
void kbModelComponent::SetMaterialParamVector( const int idx, const std::string & paramName, const kbVec4 paramValue ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		kbWarning( "kbModelComponent::SetMaterialParamVector() called on invalid index" );
		return;
	}

	kbShaderParamComponent newParam;
	newParam.SetParamName( paramName );
	newParam.SetVector( paramValue );
	m_MaterialList[idx].SetShaderParamComponent( newParam );

	RefreshMaterials( true );
}

/**
 *	kbModelComponent:SetMaterialParamTexture
 */
void kbModelComponent::SetMaterialParamTexture( const int idx, const std::string & paramName, kbTexture *const pTexture ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		kbWarning( "kbModelComponent::SetMaterialParamVector() called on invalid index" );
		return;
	}

	kbShaderParamComponent newParam;
	newParam.SetParamName( paramName );
	newParam.SetTexture( pTexture );
	m_MaterialList[idx].SetShaderParamComponent( newParam );

	RefreshMaterials( true );
}

/**
 *	kbModelComponent:SetMaterialParamTexture
 */
void kbModelComponent::SetMaterialParamTexture( const int idx, const std::string & paramName, kbRenderTexture *const pRenderTexture ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		kbWarning( "kbModelComponent::SetMaterialParamVector() called on invalid index" );
		return;
	}
	kbShaderParamComponent newParam;
	newParam.SetParamName( paramName );
	newParam.SetRenderTexture( pRenderTexture );
	m_MaterialList[idx].SetShaderParamComponent( newParam );

	RefreshMaterials( true );

	if ( IsEnabled() ) {
		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}
}

/**
 *	kbShaderParamComponent::Constructor
 */
void kbShaderParamComponent::Constructor() {
	m_pTexture = nullptr;
	m_pRenderTexture = nullptr;
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

	// Refresh owner
	if ( GetOwningComponent() != nullptr && GetOwningComponent()->IsA( kbModelComponent::GetType() ) ) {
		kbModelComponent *const pModelComp = (kbModelComponent*) GetOwningComponent();
		pModelComp->RefreshMaterials( true );
	} else {
		kbWarning( "kbMaterialComponent::EditorChange() - Material component doesn't have a model component owner.  Is this okay?" );
	}
}

/**
 *	kbMaterialComponent::SetShaderParamComponent
 */
void kbMaterialComponent::SetShaderParamComponent( const kbShaderParamComponent & inParam ) {
	
	for ( int i = 0; i < m_ShaderParamComponents.size(); i++ ) {
		if ( m_ShaderParamComponents[i].GetParamName() == inParam.GetParamName() ) {
			m_ShaderParamComponents[i] = inParam;
			return;
		}
	}

	m_ShaderParamComponents.push_back( inParam );
}
