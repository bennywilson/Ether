//==============================================================================
// kbModelComponent.h
//
//
// 2016-2018 kbEngine 2.0
//==============================================================================
#ifndef _KBMODELCOMPONENT_H_
#define _KBMODELCOMPONENT_H_

#include "kbRenderer_Defs.h"

/**
 *	kbShaderParamComponent
 */
class kbShaderParamComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbShaderParamComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	const kbString &							GetParamName() const { return m_ParamName; }
	const kbTexture *							GetTexture() const { return m_pTexture; }
	const kbVec4 &								GetVector() const { return m_Vector; }	

private:

	kbString									m_ParamName;
	kbTexture *									m_pTexture;
	kbVec4										m_Vector;
};

/**
 *	kbModelComponent
 */
class kbModelComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbModelComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbModelComponent();

	virtual void								EditorChange( const std::string & propertyName ) override;
	virtual void								PostLoad() override;

	void										SetShaderParamOverrides( const kbShaderParamOverrides_t & shaderParams );
	void										SetShaderVectorParam( const std::string & paramName, const kbVec4 & value );

	bool										GetCastsShadow() const { return m_bCastsShadow; }

protected:

	void										SetShaderParamList();

	enum ERenderPass							m_RenderPass;
	float										m_TranslucencySortBias;

	std::vector<kbShaderParamComponent>			m_ShaderParamList;

	kbRenderObject								m_RenderObject;

	bool										m_bCastsShadow;
};

#endif