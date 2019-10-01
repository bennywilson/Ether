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

	const kbVec3 &								GetNormalizedAnchorPt() const { return m_NormalizedAnchorPt; }
	const kbVec3 &								GetUIToScreenSizeRatio() const { return m_UIToScreenSizeRatio; }
	const kbVec3 &								GetNormalizedScreenSize() const { return m_NormalizedScreenSize; }

	const kbStaticModelComponent *				GetStaticModelComponent() const { return m_pStaticModelComponent; }

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	void										FindStaticModelComponent();
	void										RefreshMaterial();

	int											GetAuthoredWidth() const { return m_AuthoredWidth; }
	int											GetAuthoredHeight() const { return m_AuthoredHeight; }

private:

	// Editor
	int											m_AuthoredWidth;
	int											m_AuthoredHeight;
	kbVec3										m_NormalizedAnchorPt;
	kbVec3										m_UIToScreenSizeRatio;

	// Runtime
	kbVec3										m_NormalizedScreenSize;

protected:
	kbStaticModelComponent *					m_pStaticModelComponent;
};



#endif
