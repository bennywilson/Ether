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
#include "kbInputManager.h"

/**
 *	IUIWidgetListener
 */
class IUIWidgetListener abstract {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	virtual void WidgetEventCB( class kbUIWidgetComponent* const pWidget, const kbInput_t* const pInput ) = 0;

};

/**
 *	kbUIComponent
 */
class kbUIComponent : public kbGameComponent, public IInputListener {

	KB_DECLARE_COMPONENT( kbUIComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

												~kbUIComponent();

	virtual void								EditorChange( const std::string& propertyName ) override;

	const kbVec3&								GetNormalizedAnchorPt() const { return m_NormalizedAnchorPt; }
	const kbVec3&								GetUIToScreenSizeRatio() const { return m_UIToScreenSizeRatio; }
	const kbVec3&								GetNormalizedScreenSize() const { return m_NormalizedScreenSize; }

	const kbStaticModelComponent *				GetStaticModelComponent() const { return m_pStaticModelComponent; }

	void										RegisterEventListener( IUIWidgetListener* const pListener );
	void										UnregisterEventListener( IUIWidgetListener* const pListener );

	void										SetMaterialParamVector( const std::string& paramName, const kbVec4& paramValue );
	void										SetMaterialParamTexture( const std::string& paramName, kbTexture* const pTexture );

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	void										FindStaticModelComponent();
	void										RefreshMaterial();

	int											GetAuthoredWidth() const { return m_AuthoredWidth; }
	int											GetAuthoredHeight() const { return m_AuthoredHeight; }

	virtual void								InputCB( const kbInput_t& input ) override { }
	void										FireEvent( const kbInput_t* const pInput = nullptr );

private:

	// Editor
	int											m_AuthoredWidth;
	int											m_AuthoredHeight;
	kbVec3										m_NormalizedAnchorPt;
	kbVec3										m_UIToScreenSizeRatio;

	// Runtime
	std::vector<IUIWidgetListener*>				m_EventListeners;
	kbVec3										m_NormalizedScreenSize;
	bool										m_bHasFocus;

protected:
	kbStaticModelComponent*						m_pStaticModelComponent;
};

/**
 *	kbUIWidgetComponent
 */
class kbUIWidgetComponent : public kbGameComponent, public IInputListener {

	KB_DECLARE_COMPONENT( kbUIWidgetComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	enum eWidgetAnchor {
		TopLeft,
		MiddleLeft,
		BottomLeft,
		TopCenter,
		MiddleCenter,
		BottomCenter,
		TopRight,
		MiddleRight,
		BottomRight
	};

	enum eWidgetAxisLock {
		LockAll,
		LockXAxis,
		LockYAxis
	};

	virtual void							RecalculateOld( const kbUIComponent* const pParent, const bool bFull );
	virtual void							Recalculate( const kbUIWidgetComponent* const pParent, const bool bFull );

	void									SetRenderOrderBias( const float bias );
	float									GetRenderOrderBias() const;

	void									SetRelativePosition( const kbVec3& newPos );
	void									SetRelativeSize( const kbVec3& newSize );

	const kbVec3&							GetRelativePosition() const { return m_RelativePosition; }
	const kbVec3&							GetRelativeSize() const { return m_RelativeSize; }

	const kbVec3&							GetAbsolutePosition() const { return m_AbsolutePosition; }
	const kbVec3&							GetAbsoluteSize() const { return m_AbsoluteSize; }

	const kbVec3&							GetStartingPosition() const { return m_StartingPosition; }
	const kbVec3&							GetStartingSize() const { return m_StartingSize; }

	kbVec2i									GetBaseTextureDimensions() const;

	void									RegisterEventListener( IUIWidgetListener* const pListener );
	void									UnregisterEventListener( IUIWidgetListener* const pListener );

	const kbStaticModelComponent*			GetStaticModel() const { return m_pModel; }

	void									SetAdditiveTextureFactor( const float factor );

	virtual void							SetFocus( const bool bHasFocus );
	bool									HasFocus() const { return m_bHasFocus; }

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	void									FireEvent( const kbInput_t* const pInput = nullptr );

	// Editor
protected:

	std::vector<kbMaterialComponent> 		m_Materials;
	std::vector<kbUIWidgetComponent>					m_ChildWidgets;

private:

	virtual void							EditorChange( const std::string& propertyName ) override;
	
	virtual void							InputCB( const kbInput_t& input ) override;

	// Editor
	kbVec3									m_StartingPosition;
	kbVec3									m_StartingSize;
	eWidgetAnchor							m_Anchor;
	eWidgetAxisLock							m_AxisLock;

	// Runtime
protected:

	kbVec3									m_RelativePosition;
	kbVec3									m_RelativeSize;
	kbVec3									m_AbsolutePosition;
	kbVec3									m_AbsoluteSize;
	kbStaticModelComponent*					m_pModel;

	kbVec3									m_CachedParentPosition;
	kbVec3									m_CachedParentSize;

private:

	std::vector<IUIWidgetListener*>			m_EventListeners;
	bool									m_bHasFocus;
};

/**
 *	CannonUISlider
 */
class kbUISlider : public kbUIWidgetComponent {

	KB_DECLARE_COMPONENT( kbUISlider, kbUIWidgetComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	
	virtual void							RecalculateOld( const kbUIComponent* const pParent, const bool bFull ) override;
	virtual void							Recalculate( const kbUIWidgetComponent* const pParent, const bool bFull ) override;

	float									GetNormalizedValue();
	void									SetNormalizedValue( const float newValue );

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	// Editor
	kbVec3									m_SliderBoundsMin;
	kbVec3									m_SliderBoundsMax;

	// Runtime
	kbVec3									m_CalculatedSliderBoundsMin;
	kbVec3									m_CalculatedSliderBoundsMax;
};

#endif
