//==============================================================================
// kbModelComponent.h
//
//
// 2016-2018 kbEngine 2.0
//==============================================================================
#ifndef _KBMODELCOMPONENT_H_
#define _KBMODELCOMPONENT_H_

/**
 *	kbModelComponent
 */
class kbModelComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbModelComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbModelComponent();

	void										SetShaderParam( const int index, const kbVec4 & newParam );

protected:

	enum ERenderPass							m_RenderPass;
	std::vector<class kbVec4>					m_ShaderParams;
};


#endif