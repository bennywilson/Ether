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
 *	kbModelComponent
 */
class kbModelComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbModelComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbModelComponent();

	void										SetShaderParams( const kbShaderParamOverrides_t & shaderParams );

	//void										SetShaderParam( const std::string & paramName, const kbShaderParamOverrides_t::kbShaderParam_t & shaderParam );

	bool										GetCastsShadow() const { return m_bCastsShadow; }

protected:

	enum ERenderPass							m_RenderPass;
	float										m_TranslucencySortBias;

	kbShaderParamOverrides_t					m_ShaderParams;

	kbRenderObject								m_RenderObject;

	bool										m_bCastsShadow;
};

/**
 *	kbShaderParamComponent
 */
class kbShaderParamComponent : public kbGameComponent {

	friend class kbShaderParamListComponent;

	KB_DECLARE_COMPONENT( kbShaderParamComponent, kbGameComponent );

public:


private:
	kbString							m_ParamName;
	kbTexture *							m_pTexture;
	kbVec4								m_Vector;
};

/**
 *	kbShaderParamListComponent
 */
class kbShaderParamListComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbShaderParamListComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual void								EditorChange( const std::string & propertyName ) override;

	virtual void								Enable( const bool setEnabled ) override;

private:

	void										UpdateModelWithParams();

	std::vector<kbShaderParamComponent>			m_ShaderParamList;

};
#endif