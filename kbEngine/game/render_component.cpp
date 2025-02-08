/// RenderComponent.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"
#include "kbModel.h"
#include "render_component.h"
#include "kbRenderer.h"
#include "kbGame.h"

KB_DEFINE_COMPONENT(RenderComponent)

/// RenderComponent::Constructor
void RenderComponent::Constructor() {
	m_RenderPass = RP_Lighting;
	m_RenderOrderBias = 0.0f;
	m_bCastsShadow = false;
}

/// RenderComponent::~RenderComponent
RenderComponent::~RenderComponent() {
}

/// RenderComponent::EditorChange
void RenderComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	m_RenderObject.m_bCastsShadow = this->GetCastsShadow();
	m_RenderObject.m_bIsSkinnedModel = false;
	m_RenderObject.m_EntityId = GetOwner()->GetEntityId();
	m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
	m_RenderObject.m_Position = GetOwner()->GetPosition();
	m_RenderObject.m_RenderPass = m_RenderPass;
	m_RenderObject.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
	m_RenderObject.m_RenderOrderBias = m_RenderOrderBias;

	// Editor Hack!
	if ( propertyName == "Materials" ) {
		for ( int i = 0; i < this->m_MaterialList.size(); i++ ) {
			m_MaterialList[i].SetOwningComponent( this );
		}
	}

	RefreshMaterials( true );
}

/// RenderComponent::PostLoad
void RenderComponent::PostLoad() {
	Super::PostLoad();

	if ( GetOwner()->IsPrefab() == false ) {
		RefreshMaterials( false );
	}
}

/// RenderComponent::RefreshMaterials
void RenderComponent::RefreshMaterials( const bool bRefreshRenderObject ) {
	m_RenderObject.m_Materials.clear();
	for ( int i = 0; i < m_MaterialList.size(); i++ ) {
		const kbMaterialComponent & matComp = m_MaterialList[i];
	
		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_pShader = matComp.GetShader();
		newShaderParams.m_CullModeOverride = matComp.GetCullModeOverride();

		const auto& srcShaderParams = matComp.GetShaderParams();
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

	m_RenderObject.m_RenderOrderBias = m_RenderOrderBias;
	if ( IsEnabled() && m_RenderObject.m_pComponent != nullptr && bRefreshRenderObject ) {
		g_pRenderer->UpdateRenderObject( m_RenderObject );
	}
}

/// RenderComponent:SetMaterialParamVector
void RenderComponent::SetMaterialParamVector( const int idx, const std::string & paramName, const Vec4& paramValue ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		blk::warn( "RenderComponent::SetMaterialParamVector() called on invalid index" );
		return;
	}

	kbShaderParamComponent newParam;
	newParam.SetParamName( paramName );
	newParam.SetVector( paramValue );
	m_MaterialList[idx].SetShaderParamComponent( newParam );

	RefreshMaterials( true );
}

/// RenderComponent:SetMaterialParamTexture
void RenderComponent::SetMaterialParamTexture( const int idx, const std::string & paramName, kbTexture *const pTexture ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		blk::warn( "RenderComponent::SetMaterialParamVector() called on invalid index" );
		return;
	}

	kbShaderParamComponent newParam;
	newParam.SetParamName( paramName );
	newParam.SetTexture( pTexture );
	m_MaterialList[idx].SetShaderParamComponent( newParam );

	RefreshMaterials( true );
}

/// RenderComponent::SetMaterialParamTexture
void RenderComponent::SetMaterialParamTexture( const int idx, const std::string & paramName, kbRenderTexture *const pRenderTexture ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		blk::warn( "RenderComponent::SetMaterialParamVector() called on invalid index" );
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

/// RenderComponent::GetShaderParamComponent
const kbShaderParamComponent * RenderComponent::GetShaderParamComponent( const int idx, const kbString & name ) {
	if ( idx < 0 || idx > 32 || idx >= m_MaterialList.size() ) {
		blk::warn( "RenderComponent::SetMaterialParamVector() called on invalid index" );
		return nullptr;
	}

	return m_MaterialList[idx].GetShaderParamComponent( name );
}

/// kbShaderParamComponent::Constructor
void kbShaderParamComponent::Constructor() {
	m_pTexture = nullptr;
	m_pRenderTexture = nullptr;
	m_Vector.set( 0.0f, 0.0f, 0.0f, 0.0f );
}

/// kbMaterialComponent::Constructor
void kbMaterialComponent::Constructor() {
	m_pShader = nullptr;
	m_CullModeOverride = CullMode_ShaderDefault;
}

/// kbMaterialComponent::EditorChange
void kbMaterialComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "Shader" && m_pShader != nullptr ) {

		std::vector<kbShaderParamComponent>	oldParams = m_ShaderParamComponents;
		m_ShaderParamComponents.clear();

		const kbShaderVarBindings_t & shaderBindings = m_pShader->GetShaderVarBindings();
		for ( int i = 0; i < shaderBindings.m_VarBindings.size(); i++ ) {
			auto & currentVar = shaderBindings.m_VarBindings[i];
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
				newParam.SetVector( Vec4::zero );
				newParam.SetTexture( nullptr );
				m_ShaderParamComponents.push_back( newParam );
			}
		}

		for ( int i = 0; i < shaderBindings.m_Textures.size(); i++ ) {
			auto & curTexture = shaderBindings.m_Textures[i];
			if ( curTexture.m_bIsUserDefinedVar == false ) {
				continue;
			}

			const kbString boundTextureName( curTexture.m_TextureName );
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
				newParam.SetVector( Vec4::zero );
				newParam.SetTexture( nullptr );
				m_ShaderParamComponents.push_back( newParam );			
			}
		}
	}

	// Refresh owner
	if ( GetOwningComponent() != nullptr && GetOwningComponent()->IsA( RenderComponent::GetType() ) ) {
		RenderComponent *const pModelComp = (RenderComponent*) GetOwningComponent();
		pModelComp->RefreshMaterials( true );
	} else {
		blk::warn( "kbMaterialComponent::EditorChange() - Material component doesn't have a model component owner.  Is this okay?" );
	}
}

/// kbMaterialComponent::SetShaderParamComponent
void kbMaterialComponent::SetShaderParamComponent( const kbShaderParamComponent & inParam ) {
	
	for ( int i = 0; i < m_ShaderParamComponents.size(); i++ ) {
		if ( m_ShaderParamComponents[i].GetParamName() == inParam.GetParamName() ) {
			m_ShaderParamComponents[i] = inParam;
			return;
		}
	}

	m_ShaderParamComponents.push_back( inParam );
}

/// kbMaterialComponent::GetShaderParamComponent
const kbShaderParamComponent * kbMaterialComponent::GetShaderParamComponent( const kbString & name ) {

	for ( int i = 0; i < m_ShaderParamComponents.size(); i++ ) {
		if ( m_ShaderParamComponents[i].GetParamName() == name) {
			return &m_ShaderParamComponents[i];
		}
	}

	return nullptr;
}


/// kbShaderModifierComponent::Constructor
void kbShaderModifierComponent::Constructor() {

	m_pRenderComponent = nullptr;
	m_StartTime = -1.0f;
	m_AnimationLengthSec = -1.0f;
}

/// kbShaderModifierComponent::SetEnable_Internal
void kbShaderModifierComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( m_ShaderVectorEvents.size() == 0 ) {
		return;
	}

	m_pRenderComponent = nullptr;
	if ( bEnable ) {

		for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
			if ( GetOwner()->GetComponent(i)->IsA( RenderComponent::GetType() ) == false ) {
				continue;
			}
			m_pRenderComponent = (RenderComponent*)GetOwner()->GetComponent(i);
			break;
		}
		m_AnimationLengthSec = m_ShaderVectorEvents[m_ShaderVectorEvents.size() - 1].GetEventTime();
		m_StartTime = g_GlobalTimer.TimeElapsedSeconds();
	}
}

/// kbShaderModifierComponent::Update_Internal
void kbShaderModifierComponent::Update_Internal( const float dt ) {

	if ( m_pRenderComponent == nullptr || m_ShaderVectorEvents.size() == 0 ) {
		Enable( false );
		return;
	}

	// hack - Update is called before enable some how
	if ( m_AnimationLengthSec == 0.0f ) {
		return;
	}

	const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_StartTime;
	const Vec4 shaderParam = kbVectorAnimEvent::Evaluate( m_ShaderVectorEvents, elapsedTime );
	m_pRenderComponent->SetMaterialParamVector( 0, m_ShaderVectorEvents[0].GetEventName().stl_str(), shaderParam );
}