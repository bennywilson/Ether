//===================================================================================================
// kbUIComponent.h
//
//
// 2019 kbEngine 2.0
//===================================================================================================
#ifndef _KBUICOMPONENT_H_
#define _KBUICOMPONENT_H_

#include "kbRenderer_defs.h"
#include "kbRenderBuffer.h"
#include "kbModel.h"

/**
 *	kbUIComponent
 */
class kbUIComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbUIComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

												~kbUIComponent();

	virtual void								EditorChange( const std::string & propertyName ) override;

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	void										FindStaticModelComponent();
	void										RefreshMaterial();

private:

	// Editor
	int											m_AuthoredWidth;
	int											m_AuthoredHeight;
	kbVec3										m_NormalizedAnchorPt;
	kbVec3										m_UIToScreenSizeRatio;

	// Runtime
protected:
	kbStaticModelComponent *					m_pStaticModelComponent;
};



#endif
