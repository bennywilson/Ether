/// kbLightComponent.h
///
/// 2016-2025 blk 1.0

#pragma once

/// kbLightComponent
class kbLightComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbLightComponent, kbGameComponent);

public:
	virtual	~kbLightComponent();

	virtual void PostLoad() override;

	virtual void								RenderSync();

	virtual void								EditorChange(const std::string& propertyName) override;

	void										SetColor(const kbColor& newColor) { m_Color = newColor; }
	void										SetColor(const float R, const float G, const float B, const float A) { m_Color.set(R, G, B, A); }

	float										GetBrightness() const { return m_Brightness; }
	const kbColor& GetColor() const { return m_Color; }
	virtual float								GetRadius() const { return 0.0f; }
	virtual float								GetLength() const { return 0.0f; }

	bool										CastsShadow() const { return m_bCastsShadow; }

	const std::vector<kbMaterialComponent>& GetMaterialList() const { return m_MaterialList; }

protected:

	void										RefreshMaterials();

	virtual void								SetEnable_Internal(const bool isEnabled) override;
	virtual void								Update_Internal(const float DeltaTime) override;

	std::vector<kbMaterialComponent>			m_MaterialList;

	kbColor										m_Color;
	float										m_Brightness;
	bool										m_bCastsShadow;
	bool										m_bShaderParamsDirty;

};

/// kbPointLightComponent
class kbPointLightComponent : public kbLightComponent {

	KB_DECLARE_COMPONENT(kbPointLightComponent, kbLightComponent);

	//---------------------------------------------------------------------------------------------------
public:

	virtual float								GetRadius() const override { return m_Radius; }

protected:

	float										m_Radius;
};

/// kbCylindricalLightComponent
class kbCylindricalLightComponent : public kbPointLightComponent {

	KB_DECLARE_COMPONENT(kbCylindricalLightComponent, kbPointLightComponent);

	//---------------------------------------------------------------------------------------------------
public:
	virtual float								GetLength() const override { return m_Length; }

protected:

	float										m_Length;
};

/// kbDirectionalLightComponent
class kbDirectionalLightComponent : public kbLightComponent {

	KB_DECLARE_COMPONENT(kbDirectionalLightComponent, kbLightComponent);

	//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbDirectionalLightComponent();


	virtual void								EditorChange(const std::string& propertyName) override;
	const std::vector<float>& GetSplitDistances() const { return m_SplitDistances; }

protected:

	std::vector<float>							m_SplitDistances;
};

/// kbLightShaftsComponent
class kbLightShaftsComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbLightShaftsComponent, kbGameComponent);

public:

	virtual ~kbLightShaftsComponent();

	kbTexture* GetTexture() const { return m_Texture; }
	const kbColor& GetColor() const { return m_Color; }
	float GetBaseWidth() const { return m_BaseWidth; }
	float GetBaseHeight() const { return m_BaseHeight; }
	float GetIterationWidth() const { return m_IterationWidth; }
	float GetIterationHeight() const { return m_IterationHeight; }
	int	GetNumIterations() const { return m_NumIterations; }
	bool IsDirectional() const { return m_Directional; }

	void SetColor(const kbColor& newColor);

protected:
	virtual void SetEnable_Internal(const bool isEnabled) override;
	virtual void Update_Internal(const float DeltaTime) override;

	kbTexture* m_Texture;
	kbColor	m_Color;
	float m_BaseWidth;
	float m_BaseHeight;
	float m_IterationWidth;
	float m_IterationHeight;
	int m_NumIterations;
	bool m_Directional;
};

/// kbFogComponent
class kbFogComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbFogComponent, kbGameComponent);

public:
	void SetColor(const kbColor& newColor) { m_Color = newColor; }

protected:
	virtual void Update_Internal(const float DeltaTime) override;

	kbColor	m_Color;
	float m_StartDistance;
	float m_EndDistance;
};
