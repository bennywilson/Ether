//===================================================================================================
// kbLightComponent.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbLightComponent)
KB_DEFINE_COMPONENT(kbDirectionalLightComponent)

/**
 *	kbLightComponent::Constructor
 */
void kbLightComponent::Constructor() {
	m_Color = kbColor::white;
	m_bCastsShadow = false;
	m_Brightness = 1;
	m_bShaderParamsDirty = false;
}

/**
 *	kbLightComponent::~kbLightComponent
 */
kbLightComponent::~kbLightComponent() {

}

/**
 *	kbLightComponent::PostLoad
 */
void kbLightComponent::PostLoad() {
	Super::PostLoad();
}

/**
 *	kbLightComponent::EditorChange
 */
void kbLightComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( IsEnabled() ) {
		m_bShaderParamsDirty = true;
	}

	// Editor Hack
	if ( propertyName == "Materials" ) {
		for ( int i = 0; i < m_MaterialList.size(); i++ ) {
			m_MaterialList[i].SetOwningComponent( this );
		}
	}
}

/**
 *	kbLightComponent::RenderSync
 */
void kbLightComponent::RenderSync() {
	Super::RenderSync();

	if ( m_bShaderParamsDirty ) {
		RefreshMaterials();
		m_bShaderParamsDirty = false;
	}
}

/**
 *	kbLightComponent::SetEnable_Internal
 */
void kbLightComponent::SetEnable_Internal( const bool bIsEnabled ) {

	Super::SetEnable_Internal( bIsEnabled );

	if ( g_pRenderer != nullptr ) {
		if ( bIsEnabled ) {
			m_bShaderParamsDirty = true;

			g_pRenderer->AddLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
		} else {
			g_pRenderer->RemoveLight( this );
		}
	}
}

/**
 *	kbLightComponent:RefreshMaterials
 */
void kbLightComponent::RefreshMaterials() {

	//m_RenderObject.m_Materials.clear();
	/*{//for ( int i = 0; i < m_MaterialList.size(); i++ ) {
		kbMaterialComponent & matComp = m_Material;
	
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

	/*if ( m_pOverrideShader == nullptr ) {
		return;
	}

	for ( int i = 0; i < m_OverrideShaderParamList.size(); i++ ) {
		const kbShaderParamComponent & curParam = m_OverrideShaderParamList[i];
		if ( curParam.GetParamName().stl_str().empty() ) {
			continue;
		}

		if ( curParam.GetTexture() != nullptr ) {
			m_OverrideShaderParams.SetTexture( curParam.GetParamName().stl_str(), curParam.GetTexture() );
		} else {
			m_OverrideShaderParams.SetVec4( curParam.GetParamName().stl_str(), curParam.GetVector() );
		}	
	}*/
}

/**
 *	kbLightComponent:Update_Internal
 */
void kbLightComponent::Update_Internal( const float DeltaTime ) {

	Super::Update_Internal( DeltaTime );

	if ( GetLifeTimeRemaining() >= 0 ) {
		// Hack fade for grenade
		m_Brightness = kbClamp( GetLifeTimeRemaining() / GetStartingLifeTime(), 0.0f, 1.0f );
		g_pRenderer->UpdateLight( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );
		return;
	}

	if ( this->IsA( kbDirectionalLightComponent::GetType() ) ) {
		kbShaderParamOverrides_t::kbShaderParam_t shaderParam;
		shaderParam.m_VarName = "sunDir";
		kbVec4 sunDir = GetOwner()->GetOrientation().ToMat4()[2] * -1.0f;
		shaderParam.m_Vec4List.push_back( sunDir );
		shaderParam.m_Type = kbShaderParamOverrides_t::kbShaderParam_t::SHADER_VEC4;
		g_pRenderer->SetGlobalShaderParam( shaderParam );
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

	{
		kbShaderParamOverrides_t::kbShaderParam_t shaderParam;
		shaderParam.m_VarName = "sunDir";
		kbVec4 sunDir = GetOwner()->GetOrientation().ToMat4()[2] * -1.0f;
		shaderParam.m_Vec4List.push_back( sunDir );
		shaderParam.m_Type = kbShaderParamOverrides_t::kbShaderParam_t::SHADER_VEC4;
		g_pRenderer->SetGlobalShaderParam( shaderParam );
	}
}

/**
 *	kbLightShaftsComponent::Constructor
 */
void kbLightShaftsComponent::Constructor() {
	m_Texture = ( kbTexture* )g_ResourceManager.GetResource( "../../kbEngine/assets/Textures/Editor/flare.jpg" );
	m_Color = kbColor::white;
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
}

/**
 *	kbLightShaftsComponent::Update_Internal
 */
void kbLightShaftsComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	g_pRenderer->UpdateLightShafts( this, GetOwner()->GetPosition(), GetOwner()->GetOrientation() );

	{
		kbShaderParamOverrides_t::kbShaderParam_t shaderParam;
		shaderParam.m_VarName = "lightShaftsDir";
		kbVec4 lightShaftsDir = GetOwner()->GetOrientation().ToMat4()[2] * -1.0f;
		shaderParam.m_Vec4List.push_back( lightShaftsDir );
		shaderParam.m_Type = kbShaderParamOverrides_t::kbShaderParam_t::SHADER_VEC4;
		g_pRenderer->SetGlobalShaderParam( shaderParam );
	}

	{
		kbShaderParamOverrides_t::kbShaderParam_t shaderParam;
		shaderParam.m_VarName = "lightShaftsColor";
		shaderParam.m_Vec4List.push_back( m_Color );
		shaderParam.m_Type = kbShaderParamOverrides_t::kbShaderParam_t::SHADER_VEC4;
		g_pRenderer->SetGlobalShaderParam( shaderParam );
	}

}

/**
 *	kbFogComponent::Constructor
 */
void kbFogComponent::Constructor() {
	m_Color = kbColor::white;
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
