//==============================================================================
// CannonUIWidgets.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KBCANNONUIWIDGETS_H_
#define _KBCANNONUIWIDGETS_H_

/**
 *	IUIWidgetListener
 */
class IUIWidgetListener abstract {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	virtual void WidgetEventCB( class CannonUIWidget *const pWidget ) = 0;

};

/**
 *	CannonUIWidget
 */
class CannonUIWidget : public kbGameComponent {

	KB_DECLARE_COMPONENT( CannonUIWidget, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual void							RecalculateOld( const kbUIComponent *const pParent );
	virtual void							Recalculate( const CannonUIWidget *const pParent );

	void									SetRelativePosition( const kbVec3 & newPos );
	void									SetRelativeSize( const kbVec3 & newSize );

	const kbVec3 &							GetRelativePosition() const { return m_RelativePosition; }
	const kbVec3 &							GetRelativeSize() const { return m_RelativeSize; }

	const kbVec3 &							GetAbsolutePosition() const { return m_AbsolutePosition; }
	const kbVec3 &							GetAbsoluteSize() const { return m_AbsoluteSize; }

	const kbVec3 &							GetStartingPosition() const { return m_StartingPosition; }
	const kbVec3 &							GetStartingSize() const { return m_StartingSize; }

	kbVec2i									GetBaseTextureDimensions() const;

	void									RegisterEventListener( IUIWidgetListener * pListener );
	void									UnregisterEventListener( IUIWidgetListener * pListener );

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	void									FireEvent();

	// Editor
protected:

	std::vector<kbMaterialComponent> 		m_Materials;
	std::vector<CannonUIWidget>				m_ChildWidgets;

private:

	// Editor
	kbVec3									m_StartingPosition;
	kbVec3									m_StartingSize;


	// Runtime
	kbVec3									m_RelativePosition;
	kbVec3									m_RelativeSize;
	kbVec3									m_AbsolutePosition;
	kbVec3									m_AbsoluteSize;
	kbStaticModelComponent *				m_pModel;

	kbVec3									m_CachedParentPosition;
	kbVec3									m_CachedParentSize;

	std::vector<IUIWidgetListener*>			m_EventListeners;
};

/**
 *	CannonUISlider
 */
class CannonUISlider : public CannonUIWidget {

	KB_DECLARE_COMPONENT( CannonUISlider, CannonUIWidget );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	
	virtual void							RecalculateOld( const kbUIComponent *const pParent ) override;
	virtual void							Recalculate( const CannonUIWidget *const pParent ) override;

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