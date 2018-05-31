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

	void										SetShaderParam( const std::string & paramName, const kbShaderParamOverrides_t::kbShaderParam_t & shaderParam );

	bool										GetCastsShadow() const { return m_bCastsShadow; }

protected:

	enum ERenderPass							m_RenderPass;
	kbShaderParamOverrides_t					m_ShaderParams;

	bool										m_bCastsShadow;
};


#endif