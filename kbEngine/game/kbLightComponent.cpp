//===================================================================================================
// kbLightComponent.cpp
//
//
// 2016-2019 blk 1.0
//===================================================================================================
#include "blk_core.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbLightComponent)
KB_DEFINE_COMPONENT(kbDirectionalLightComponent)

/// kbLightComponent::Constructor
void kbLightComponent::Constructor() {
	m_Color = kbColor::white;
	m_casts_shadow = false;
	m_Brightness = 1;
	m_bShaderParamsDirty = false;
}

/// kbLightComponent::~kbLightComponent
kbLightComponent::~kbLightComponent() {

}

/// kbLightComponent::PostLoad
void kbLightComponent::post_load() {
	Super::post_load();
}

/// kbLightComponent::EditorChange
void kbLightComponent::editor_change( const std::string & propertyName ) {
	Super::editor_change( propertyName );

	if ( IsEnabled() ) {
		m_bShaderParamsDirty = true;
	}

	// Editor Hack
	if ( propertyName == "Materials" ) {
		for ( int i = 0; i < m_materials.size(); i++ ) {
			m_materials[i].SetOwningComponent( this );
		}
	}
}

/// kbLightComponent::RenderSync
void kbLightComponent::RenderSync() {
	Super::RenderSync();

	if ( m_bShaderParamsDirty ) {
		refresh_materials();
		m_bShaderParamsDirty = false;
	}
}

/// kbLightComponent::enable_internal
void kbLightComponent::enable_internal( const bool bIsEnabled ) {

	Super::enable_internal( bIsEnabled );

	if ( g_pRenderer != nullptr ) {
		if ( bIsEnabled ) {
			m_bShaderParamsDirty = true;

			g_pRenderer->AddLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
		} else {
			g_pRenderer->RemoveLight( this );
		}
	}
}

/// kbLightComponent:RefreshMaterials
void kbLightComponent::refresh_materials() {

	//m_render_object.m_Materials.clear();
	/*{//for ( int i = 0; i < m_materials.size(); i++ ) {
		kbMaterialComponent & matComp = m_Material;
	
		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_shader = matComp.get_shader();
	
		auto srcShaderParams = matComp.shader_params();
		for ( int j = 0; j < srcShaderParams.size(); j++ ) {
			if ( srcShaderParams[j].Texture() != nullptr ) {
				newShaderParams.SetTexture( srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].Texture() );
			} else if ( srcShaderParams[j].RenderTexture() != nullptr ) {
	
				newShaderParams.SetTexture( srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].RenderTexture() );
			} else {
				newShaderParams.SetVec4( srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].GetVector() );
			}
		}
	
		m_render_object.m_Materials.push_back( newShaderParams );
	}

	if ( IsEnabled() && m_render_object.m_pComponent != nullptr && bRefreshRenderObejct ) {
		g_pRenderer->UpdateRenderObject( m_render_object );
	}

	/*if ( m_pOverrideShader == nullptr ) {
		return;
	}

	for ( int i = 0; i < m_OverrideShaderParamList.size(); i++ ) {
		const kbShaderParamComponent & curParam = m_OverrideShaderParamList[i];
		if ( curParam.param_name().stl_str().empty() ) {
			continue;
		}

		if ( curParam.Texture() != nullptr ) {
			m_OverrideShaderParams.SetTexture( curParam.param_name().stl_str(), curParam.Texture() );
		} else {
			m_OverrideShaderParams.SetVec4( curParam.param_name().stl_str(), curParam.GetVector() );
		}	
	}*/
}

/// kbLightComponent:update_internal
void kbLightComponent::update_internal( const float DeltaTime ) {

	Super::update_internal( DeltaTime );

	if ( GetLifeTimeRemaining() >= 0 ) {
		// Hack fade for grenade
		m_Brightness = kbClamp( GetLifeTimeRemaining() / GetStartingLifeTime(), 0.0f, 1.0f );
		g_pRenderer->UpdateLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
		return;
	}

	if ( this->IsA( kbDirectionalLightComponent::GetType() ) ) {

		kbShaderParamOverrides_t shaderParam;
		shaderParam.SetVec4( "sunDir", GetOwner()->GetOrientation().to_mat4()[2] * -1.0f );
		g_pRenderer->SetGlobalShaderParam( shaderParam );
	}

	if ( IsDirty() ) {
		g_pRenderer->UpdateLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
	}
}

/// kbPointLightComponent::Constructor
void kbPointLightComponent::Constructor() {
	m_Radius = 16.0f;
}

/// kbCylindricalLightComponent::Constructor
void kbCylindricalLightComponent::Constructor() {
	m_Length = 32.0f;
}

/// kbDirectionalLightComponent::Constructor
void kbDirectionalLightComponent::Constructor() {
}

/// kbDirectionalLightComponent::~kbDirectionalLightComponent
kbDirectionalLightComponent::~kbDirectionalLightComponent() {

}

/// kbDirectionalLightComponent::EditorChange
void kbDirectionalLightComponent::editor_change( const std::string & propertyName ) {
	Super::editor_change( propertyName );
	// TODO: clamp shadow splits to 4.  Also ensure that the ordering is correct

	{
		kbShaderParamOverrides_t shaderParam;
		shaderParam.SetVec4( "sunDir", GetOwner()->GetOrientation().to_mat4()[2] * -1.0f );
		g_pRenderer->SetGlobalShaderParam( shaderParam );
	}
}

/// kbLightShaftsComponent::Constructor
void kbLightShaftsComponent::Constructor() {
	m_Texture = nullptr;
	m_Color = kbColor::white;
	m_BaseWidth = m_BaseHeight = 20.0f;
	m_IterationWidth = m_IterationHeight = 1.0f;
	m_NumIterations = 4;
	m_Directional = true;
}

/// kbLightShaftsComponent::~kbLightShaftsComponent
kbLightShaftsComponent::~kbLightShaftsComponent() {
}

/// kbLightShaftsComponent::enable_internal
void kbLightShaftsComponent::enable_internal( const bool isEnabled ) {
	Super::enable_internal( isEnabled );

	if ( g_pRenderer != nullptr ) {
		if ( isEnabled ) {
			g_pRenderer->AddLightShafts( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
		} else {
			g_pRenderer->RemoveLightShafts( this );
		}
	}
}

/// kbLightShaftsComponent::SetColor
void kbLightShaftsComponent::SetColor( const kbColor & newColor ) {
	m_Color = newColor;
}

/// kbLightShaftsComponent::update_internal
void kbLightShaftsComponent::update_internal( const float DeltaTime ) {
	Super::update_internal( DeltaTime );

	g_pRenderer->UpdateLightShafts( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );

	kbShaderParamOverrides_t shaderParam;
	shaderParam.SetVec4( "lightShaftsDir", GetOwner()->GetOrientation().to_mat4()[2] * -1.0f );
	shaderParam.SetVec4( "lightShaftsColor", m_Color );
	g_pRenderer->SetGlobalShaderParam( shaderParam );
}

/// kbFogComponent::Constructor
void kbFogComponent::Constructor() {
	m_Color = kbColor::white;
	m_StartDistance = 2100;
	m_EndDistance = 2200;
}

/// kbFogComponent::update_internal
void kbFogComponent::update_internal( const float DT ) {
	Super::update_internal( DT );

	g_pRenderer->UpdateFog( m_Color, m_StartDistance, m_EndDistance );
}
