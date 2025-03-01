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
	m_render_pass = RP_Lighting;
	m_render_order_bias = 0.0f;
	m_casts_shadow = false;
}

/// RenderComponent::~RenderComponent
RenderComponent::~RenderComponent() {
}

/// RenderComponent::EditorChange
void RenderComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	m_render_object.m_casts_shadow = this->GetCastsShadow();
	m_render_object.m_bIsSkinnedModel = false;
	m_render_object.m_EntityId = GetOwner()->GetEntityId();
	m_render_object.m_Orientation = GetOwner()->GetOrientation();
	m_render_object.m_position = GetOwner()->GetPosition();
	m_render_object.m_render_pass = m_render_pass;
	m_render_object.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
	m_render_object.m_render_order_bias = m_render_order_bias;

	// Editor Hack!
	if (propertyName == "Materials") {
		for (int i = 0; i < this->m_materials.size(); i++) {
			m_materials[i].SetOwningComponent(this);
		}
	}

	refresh_materials(true);
}

/// RenderComponent::PostLoad
void RenderComponent::post_load() {
	Super::post_load();

	if (GetOwner()->IsPrefab() == false) {
		refresh_materials(false);
	}
}

/// RenderComponent::RefreshMaterials
void RenderComponent::refresh_materials(const bool bRefreshRenderObject) {
	m_render_object.m_Materials.clear();
	for (int i = 0; i < m_materials.size(); i++) {
		const kbMaterialComponent& matComp = m_materials[i];

		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_shader = matComp.get_shader();
		newShaderParams.m_cull_override = matComp.cull_mode_override();

		const auto& srcShaderParams = matComp.shader_params();
		for (int j = 0; j < srcShaderParams.size(); j++) {
			if (srcShaderParams[j].texture() != nullptr) {
				newShaderParams.SetTexture(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].texture());
			} else if (srcShaderParams[j].render_texture() != nullptr) {

				newShaderParams.SetTexture(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].render_texture());
			} else {
				newShaderParams.SetVec4(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].vector());
			}
		}

		m_render_object.m_Materials.push_back(newShaderParams);
	}

	m_render_object.m_render_order_bias = m_render_order_bias;
	if (IsEnabled() && m_render_object.m_pComponent != nullptr && bRefreshRenderObject) {
		g_pRenderer->UpdateRenderObject(m_render_object);
	}
}

/// RenderComponent::set_material_param_vec4
void RenderComponent::set_material_param_vec4(const int idx, const std::string& paramName, const Vec4& paramValue) {
	if (idx < 0 || idx > 32 || idx >= m_materials.size()) {
		blk::warn("RenderComponent::set_material_param_vec4() called on invalid index");
		return;
	}

	kbShaderParamComponent newParam;
	newParam.set_param_name(paramName);
	newParam.set_vector(paramValue);
	m_materials[idx].set_shader_param(newParam);

	refresh_materials(true);
}

/// RenderComponent::set_material_param_texture
void RenderComponent::set_material_param_texture(const int idx, const std::string& paramName, kbTexture* const pTexture) {
	if (idx < 0 || idx > 32 || idx >= m_materials.size()) {
		blk::warn("RenderComponent::set_material_param_vec4() called on invalid index");
		return;
	}

	kbShaderParamComponent newParam;
	newParam.set_param_name(paramName);
	newParam.set_texture(pTexture);
	m_materials[idx].set_shader_param(newParam);

	refresh_materials(true);
}

/// RenderComponent::SetMaterialParamTexture
void RenderComponent::set_material_param_texture(const int idx, const std::string& paramName, kbRenderTexture* const pRenderTexture) {
	if (idx < 0 || idx > 32 || idx >= m_materials.size()) {
		blk::warn("RenderComponent::set_material_param_vec4() called on invalid index");
		return;
	}
	kbShaderParamComponent newParam;
	newParam.set_param_name(paramName);
	newParam.set_render_texture(pRenderTexture);
	m_materials[idx].set_shader_param(newParam);

	refresh_materials(true);

	if (IsEnabled()) {
		g_pRenderer->UpdateRenderObject(m_render_object);
	}
}

/// RenderComponent::GetShaderParamComponent
const kbShaderParamComponent* RenderComponent::shader_param_component(const int idx, const kbString& name) {
	if (idx < 0 || idx > 32 || idx >= m_materials.size()) {
		blk::warn("RenderComponent::set_material_param_vec4() called on invalid index");
		return nullptr;
	}

	return m_materials[idx].shader_param_component(name);
}

/// kbShaderParamComponent::Constructor
void kbShaderParamComponent::Constructor() {
	m_texture = nullptr;
	m_render_texture = nullptr;
	m_vector.set(0.0f, 0.0f, 0.0f, 0.0f);
}

/// kbMaterialComponent::Constructor
void kbMaterialComponent::Constructor() {
	m_shader = nullptr;
	m_cull_override = CullMode_ShaderDefault;
}

/// kbMaterialComponent::EditorChange
void kbMaterialComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	if (propertyName == "Shader" && m_shader != nullptr) {

		std::vector<kbShaderParamComponent>	oldParams = m_shader_params;
		m_shader_params.clear();

		const kbShaderVarBindings_t& shaderBindings = m_shader->GetShaderVarBindings();
		for (int i = 0; i < shaderBindings.m_VarBindings.size(); i++) {
			auto& currentVar = shaderBindings.m_VarBindings[i];
			if (currentVar.m_bIsUserDefinedVar == false) {
				continue;
			}

			const kbString boundVarName(currentVar.m_VarName);
			bool boundParamFound = false;
			for (int iOldParam = 0; iOldParam < oldParams.size(); iOldParam++) {
				if (oldParams[iOldParam].param_name() == boundVarName) {
					m_shader_params.push_back(oldParams[iOldParam]);
					boundParamFound = true;
					break;
				}
			}

			if (boundParamFound == false) {
				kbShaderParamComponent newParam;
				newParam.set_param_name(boundVarName);
				newParam.set_vector(Vec4::zero);
				newParam.set_texture(nullptr);
				m_shader_params.push_back(newParam);
			}
		}

		for (int i = 0; i < shaderBindings.m_Textures.size(); i++) {
			auto& curTexture = shaderBindings.m_Textures[i];
			if (curTexture.m_bIsUserDefinedVar == false) {
				continue;
			}

			const kbString boundTextureName(curTexture.m_TextureName);
			bool boundParamFound = false;
			for (int iOldParam = 0; iOldParam < oldParams.size(); iOldParam++) {
				if (oldParams[iOldParam].param_name() == boundTextureName) {
					m_shader_params.push_back(oldParams[iOldParam]);
					boundParamFound = true;
					break;
				}
			}

			if (boundParamFound == false) {
				kbShaderParamComponent newParam;
				newParam.set_param_name(boundTextureName);
				newParam.set_vector(Vec4::zero);
				newParam.set_texture(nullptr);
				m_shader_params.push_back(newParam);
			}
		}
	}

	// Refresh owner
	if (GetOwningComponent() != nullptr && GetOwningComponent()->IsA(RenderComponent::GetType())) {
		RenderComponent* const pModelComp = (RenderComponent*)GetOwningComponent();
		pModelComp->refresh_materials(true);
	} else {
		blk::warn("kbMaterialComponent::editor_change() - Material component doesn't have a model component owner.  Is this okay?");
	}
}

/// kbMaterialComponent::SetShaderParamComponent
void kbMaterialComponent::set_shader_param(const kbShaderParamComponent& inParam) {
	for (int i = 0; i < m_shader_params.size(); i++) {
		if (m_shader_params[i].param_name() == inParam.param_name()) {
			m_shader_params[i] = inParam;
			return;
		}
	}

	m_shader_params.push_back(inParam);
}

/// kbMaterialComponent::GetShaderParamComponent
const kbShaderParamComponent* kbMaterialComponent::shader_param_component(const kbString& name) {
	for (int i = 0; i < m_shader_params.size(); i++) {
		if (m_shader_params[i].param_name() == name) {
			return &m_shader_params[i];
		}
	}

	return nullptr;
}


/// kbShaderModifierComponent::Constructor
void kbShaderModifierComponent::Constructor() {
	m_pRenderComponent = nullptr;
	m_start_time = -1.0f;
	m_anim_length_sec = -1.0f;
}

/// kbShaderModifierComponent::enable_internal
void kbShaderModifierComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	if (m_ShaderVectorEvents.size() == 0) {
		return;
	}

	m_pRenderComponent = nullptr;
	if (bEnable) {

		for (int i = 0; i < GetOwner()->NumComponents(); i++) {
			if (GetOwner()->GetComponent(i)->IsA(RenderComponent::GetType()) == false) {
				continue;
			}
			m_pRenderComponent = (RenderComponent*)GetOwner()->GetComponent(i);
			break;
		}
		m_anim_length_sec = m_ShaderVectorEvents[m_ShaderVectorEvents.size() - 1].GetEventTime();
		m_start_time = g_GlobalTimer.TimeElapsedSeconds();
	}
}

/// kbShaderModifierComponent::update_internal
void kbShaderModifierComponent::update_internal(const float dt) {
	if (m_pRenderComponent == nullptr || m_ShaderVectorEvents.size() == 0) {
		Enable(false);
		return;
	}

	// hack - Update is called before enable some how
	if (m_anim_length_sec == 0.0f) {
		return;
	}

	const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_start_time;
	const Vec4 shaderParam = kbVectorAnimEvent::Evaluate(m_ShaderVectorEvents, elapsedTime);
	m_pRenderComponent->set_material_param_vec4(0, m_ShaderVectorEvents[0].GetEventName().stl_str(), shaderParam);
}