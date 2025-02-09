/// model_component.h
///
// 2016-2025 blk 1.0

#pragma once

#include "kbComponent.h"
#include "kbRenderer_Defs.h"

class kbTexture;
class kbShader;
class kbRenderTexture;

/// kbShaderParamComponent
class kbShaderParamComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbShaderParamComponent, kbGameComponent);

	friend class kbMaterialComponent;

public:
	const kbString& param_name() const { return m_param_name; }
	const kbTexture* texture() const { return m_texture; }
	const kbRenderTexture* render_texture() const { return m_render_texture; }
	const Vec4& vector() const { return m_vector; }

	void set_render_texture(kbRenderTexture* const pTexture) { m_render_texture = pTexture; }
	void set_param_name(const kbString& newName) { m_param_name = newName; }
	void set_texture(kbTexture* const pTexture) { m_texture = pTexture; }
	void set_vector(const Vec4& vector) { m_vector = vector; }

private:
	kbString m_param_name;
	kbTexture* m_texture;
	kbRenderTexture* m_render_texture;
	Vec4 m_vector;
};

/// kbShaderModifierComponent
class kbShaderModifierComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbShaderModifierComponent, kbGameComponent);

protected:
	virtual void enable_internal(const bool isEnabled) override;
	virtual void update_internal(const float DeltaTime) override;

	// Editor
	std::vector<kbVectorAnimEvent> m_ShaderVectorEvents;

	// Runtime
	class RenderComponent* m_pRenderComponent;
	float m_start_time;
	float m_anim_length_sec;
};

/// kbMaterialComponent
class kbMaterialComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbMaterialComponent, kbGameComponent);

public:
	virtual void editor_change(const std::string& propertyName) override;

	const kbShader* get_shader() const { return m_shader; }
	const std::vector<kbShaderParamComponent>& shader_params() const { return m_shader_params; }
	ECullMode cull_mode_override() const { return m_cull_override; }

	void set_shader(kbShader* const pShader) { m_shader = pShader; }
	void set_shader_param(const kbShaderParamComponent& inParam);
	const kbShaderParamComponent* shader_param_component(const kbString& name);

private:

	kbShader* m_shader;
	ECullMode m_cull_override;
	std::vector<kbShaderParamComponent>	m_shader_params;
};

/// RenderComponent
class RenderComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(RenderComponent, kbGameComponent);

public:
	virtual	~RenderComponent();

	virtual void editor_change(const std::string& propertyName) override;
	virtual void post_load() override;

	bool GetCastsShadow() const { return m_casts_shadow; }

	void set_material_param_vec4(const int idx, const std::string& paramName, const Vec4& paramValue);
	void set_material_param_texture(const int idx, const std::string& paramName, kbTexture* const pTexture);
	void set_material_param_texture(const int idx, const std::string& paramName, kbRenderTexture* const pTexture);
	const kbShaderParamComponent* shader_param_component(const int idx, const kbString& name);

	void refresh_materials(const bool bUpdateRenderObject);

	float render_order_bias() const { return m_render_order_bias; }
	void set_render_order_bias(const float newBias) { m_render_order_bias = newBias; refresh_materials(true); }

	void set_materials(const std::vector<kbMaterialComponent>& materialList) { m_materials = materialList; }

	enum ERenderPass render_pass() const { return m_render_pass; }
	void set_render_pass(const ERenderPass newPass) { m_render_pass = newPass; }

	const std::vector<kbMaterialComponent>& Materials() const { return m_materials; }
	void CopyMaterials(const std::vector<kbMaterialComponent>& matComp) { m_materials = matComp; }

protected:
	ERenderPass	m_render_pass;
	float m_render_order_bias;

	std::vector<kbMaterialComponent> m_materials;

	kbRenderObject m_render_object;

	bool m_casts_shadow;
};