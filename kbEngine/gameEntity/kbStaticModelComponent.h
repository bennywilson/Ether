//===================================================================================================
// kbStaticModelComponent.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBSTATICMESHCOMPONENT_H_
#define _KBSTATICMESHCOMPONENT_H_

/**
 *	kbStaticModelComponent
 */
class kbStaticModelComponent : public kbModelComponent {

	KB_DECLARE_COMPONENT( kbStaticModelComponent, kbModelComponent );

//---------------------------------------------------------------------------------------------------
public:
		
	virtual										~kbStaticModelComponent();

	void										SetModel( class kbModel * pModel ) { m_pModel = pModel; }

	virtual void								EditorChange( const std::string & propertyName );

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	kbRenderObject								m_RenderObject;
	class kbModel *								m_pModel;
	std::vector< class kbShader * >				m_pOverrideShaderList;
};

#endif