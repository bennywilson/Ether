//===================================================================================================
// kbStaticModelComponent.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBSTATICMODELCOMPONENT_H_
#define _KBSTATICMODELCOMPONENT_H_

/**
 *	kbStaticModelComponent
 */
class kbStaticModelComponent : public kbModelComponent {

	KB_DECLARE_COMPONENT( kbStaticModelComponent, kbModelComponent );

//---------------------------------------------------------------------------------------------------
public:
		
	virtual										~kbStaticModelComponent();

	void										SetModel( class kbModel * pModel ) { m_pModel = pModel; }
	const kbModel *								GetModel() const { return m_pModel; }

	virtual void								EditorChange( const std::string & propertyName );

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	class kbModel *								m_pModel;
};

#endif