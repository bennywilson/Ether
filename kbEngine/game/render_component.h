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
	const kbString& GetParamName() const { return m_ParamName; }
	const kbTexture* GetTexture() const { return m_pTexture; }
	const kbRenderTexture* GetRenderTexture() const { return m_pRenderTexture; }
	const Vec4& GetVector() const { return m_Vector; }

	void SetRenderTexture(kbRenderTexture* const pTexture) { m_pRenderTexture = pTexture; }
	void SetParamName(const kbString& newName) { m_ParamName = newName; }
	void SetTexture(kbTexture* const pTexture) { m_pTexture = pTexture; }
	void SetVector(const Vec4& vector) { m_Vector = vector; }

private:
	kbString m_ParamName;
	kbTexture* m_pTexture;
	kbRenderTexture* m_pRenderTexture;
	Vec4 m_Vector;
};

/// kbShaderModifierComponent
class kbShaderModifierComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbShaderModifierComponent, kbGameComponent);

protected:
	virtual void SetEnable_Internal(const bool isEnabled) override;
	virtual void Update_Internal(const float DeltaTime) override;

	// Editor
	std::vector<kbVectorAnimEvent> m_ShaderVectorEvents;

	// Runtime
	class RenderComponent* m_pRenderComponent;
	float m_StartTime;
	float m_AnimationLengthSec;
};

/// kbMaterialComponent
class kbMaterialComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbMaterialComponent, kbGameComponent);

public:
	virtual void EditorChange(const std::string& propertyName) override;

	const kbShader* GetShader() const { return m_pShader; }
	const std::vector<kbShaderParamComponent>& GetShaderParams() const { return m_ShaderParamComponents; }
	ECullMode GetCullModeOverride() const { return m_CullModeOverride; }

	void SetShader(kbShader* const pShader) { m_pShader = pShader; }
	void SetShaderParamComponent(const kbShaderParamComponent& inParam);
	const kbShaderParamComponent* GetShaderParamComponent(const kbString& name);

private:

	kbShader* m_pShader;
	ECullMode m_CullModeOverride;
	std::vector<kbShaderParamComponent>	m_ShaderParamComponents;
};

/// RenderComponent
class RenderComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(RenderComponent, kbGameComponent);

public:
	virtual	~RenderComponent();

	virtual void EditorChange(const std::string& propertyName) override;
	virtual void PostLoad() override;

	bool GetCastsShadow() const { return m_bCastsShadow; }

	void SetMaterialParamVector(const int idx, const std::string& paramName, const Vec4& paramValue);
	void SetMaterialParamTexture(const int idx, const std::string& paramName, kbTexture* const pTexture);
	void SetMaterialParamTexture(const int idx, const std::string& paramName, kbRenderTexture* const pTexture);
	const kbShaderParamComponent* GetShaderParamComponent(const int idx, const kbString& name);

	void RefreshMaterials(const bool bUpdateRenderObject);

	float GetRenderOrderBias() const { return m_RenderOrderBias; }
	void SetRenderOrderBias(const float newBias) { m_RenderOrderBias = newBias; RefreshMaterials(true); }

	void SetMaterials(const std::vector<kbMaterialComponent>& materialList) { m_MaterialList = materialList; }

	enum ERenderPass GetRenderPass() const { return m_RenderPass; }
	void SetRenderPass(const ERenderPass newPass) { m_RenderPass = newPass; }

	const std::vector<kbMaterialComponent>& GetMaterialList() const { return m_MaterialList; }
	void CopyMaterialList(const std::vector<kbMaterialComponent>& matComp) { m_MaterialList = matComp; }

protected:
	ERenderPass	m_RenderPass;
	float m_RenderOrderBias;

	std::vector<kbMaterialComponent> m_MaterialList;

	kbRenderObject m_RenderObject;

	bool m_bCastsShadow;
};