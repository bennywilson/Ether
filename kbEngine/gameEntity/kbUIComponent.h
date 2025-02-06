//===================================================================================================
// kbUIComponent.h
//
//
// 2019 blk 1.0
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

	const Vec3&								GetNormalizedAnchorPt() const { return m_NormalizedAnchorPt; }
	const Vec3&								GetUIToScreenSizeRatio() const { return m_UIToScreenSizeRatio; }
	const Vec3&								GetNormalizedScreenSize() const { return m_NormalizedScreenSize; }

	const kbStaticModelComponent *				GetStaticModelComponent() const { return m_pStaticModelComponent; }

	void										RegisterEventListener( IUIWidgetListener* const pListener );
	void										UnregisterEventListener( IUIWidgetListener* const pListener );

	void										SetMaterialParamVector( const std::string& paramName, const Vec4& paramValue );
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
	Vec3										m_NormalizedAnchorPt;
	Vec3										m_UIToScreenSizeRatio;

	// Runtime
	std::vector<IUIWidgetListener*>				m_EventListeners;
	Vec3										m_NormalizedScreenSize;
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

	void									SetRelativePosition( const Vec3& newPos );
	void									SetRelativeSize( const Vec3& newSize );

	const Vec3&							GetRelativePosition() const { return m_RelativePosition; }
	const Vec3&							GetRelativeSize() const { return m_RelativeSize; }

	const Vec3&							GetAbsolutePosition() const { return m_AbsolutePosition; }
	const Vec3&							GetAbsoluteSize() const { return m_AbsoluteSize; }

	const Vec3&							GetStartingPosition() const { return m_StartingPosition; }
	const Vec3&							GetStartingSize() const { return m_StartingSize; }

	Vec2i									GetBaseTextureDimensions() const;

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
	Vec3									m_StartingPosition;
	Vec3									m_StartingSize;
	eWidgetAnchor							m_Anchor;
	eWidgetAxisLock							m_AxisLock;

	// Runtime
protected:

	Vec3									m_RelativePosition;
	Vec3									m_RelativeSize;
	Vec3									m_AbsolutePosition;
	Vec3									m_AbsoluteSize;
	kbStaticModelComponent*					m_pModel;

	Vec3									m_CachedParentPosition;
	Vec3									m_CachedParentSize;

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
	Vec3									m_SliderBoundsMin;
	Vec3									m_SliderBoundsMax;

	// Runtime
	Vec3									m_CalculatedSliderBoundsMin;
	Vec3									m_CalculatedSliderBoundsMax;
};

#endif
