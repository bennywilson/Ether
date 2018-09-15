//===================================================================================================
// kbLightComponent.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBLIGHTCOMPONENT_H_
#define _KBLIGHTCOMPONENT_H_

/**
 *	kbLightComponent
 */
class kbLightComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbLightComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:
		

	virtual										~kbLightComponent();

	virtual void								PostLoad() override;

	void										SetColor( const kbColor & newColor ) { m_Color = newColor; }
	void										SetColor( const float R, const float G, const float B, const float A ) { m_Color.Set( R, G, B, A ); }

	float										GetBrightness() const { return m_Brightness; }
	const kbColor &								GetColor() const { return m_Color; }
	virtual float								GetRadius() const { return 0.0f; }
	virtual float								GetLength() const { return 0.0f; }

	bool										CastsShadow() const { return m_bCastsShadow; }

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	kbColor										m_Color;
	float										m_Brightness;
	bool										m_bCastsShadow;
};

/**
 *	kbPointLightComponent
 */
class kbPointLightComponent : public kbLightComponent {

	KB_DECLARE_COMPONENT( kbPointLightComponent, kbLightComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual float								GetRadius() const override { return m_Radius; }

protected:

	float										m_Radius;
};

/**
 *	kbCylindricalLightComponent
 */
class kbCylindricalLightComponent : public kbPointLightComponent {

	KB_DECLARE_COMPONENT( kbCylindricalLightComponent, kbPointLightComponent );

//---------------------------------------------------------------------------------------------------
public:
	virtual float								GetLength() const override { return m_Length; }

protected:

	float										m_Length;
};

/**
 *	kbDirectionalLightComponent
 */
class kbDirectionalLightComponent : public kbLightComponent {

	KB_DECLARE_COMPONENT( kbDirectionalLightComponent, kbLightComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbDirectionalLightComponent();


	virtual void								EditorChange( const std::string & propertyName ) override;
	const std::vector<float> &					GetSplitDistances() const { return m_SplitDistances; }

protected:

	std::vector<float>							m_SplitDistances;
};

/**
 *	kbCustomPointLightComponent
 */
class kbCustomPointLightComponent : public kbPointLightComponent {

	KB_DECLARE_COMPONENT( kbCustomPointLightComponent, kbPointLightComponent );

//---------------------------------------------------------------------------------------------------

protected:

	kbShader *									m_pShader;
	std::vector<kbShaderParamComponent>			m_ShaderParamList;
};


/**
 *	kbLightShaftsComponent
 */
class kbLightShaftsComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbLightShaftsComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbLightShaftsComponent();

	kbTexture *									GetTexture() const { return m_Texture; }
	const kbColor &								GetColor() const { return m_Color; }
	float										GetBaseWidth() const { return m_BaseWidth; }
	float										GetBaseHeight() const { return m_BaseHeight; }
	float										GetIterationWidth() const { return m_IterationWidth; }
	float										GetIterationHeight() const { return m_IterationHeight; }
	int											GetNumIterations() const { return m_NumIterations; }
	bool										IsDirectional() const { return m_Directional; }

	void										SetColor( const kbColor & newColor );

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;

	kbTexture *									m_Texture;
	kbColor										m_Color;
	float										m_BaseWidth;
	float										m_BaseHeight;
	float										m_IterationWidth;
	float										m_IterationHeight;
	int											m_NumIterations;
	bool										m_Directional;
};


/**
 *	kbFogComponent
 */
class kbFogComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbFogComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	void										SetColor( const kbColor & newColor ) { m_Color = newColor; }

protected:

	virtual void								Update_Internal( const float DeltaTime ) override;

	kbColor										m_Color;
	float										m_StartDistance;
	float										m_EndDistance;
};


#endif