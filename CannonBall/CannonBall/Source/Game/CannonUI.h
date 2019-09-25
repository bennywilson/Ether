//==============================================================================
// CannonUI.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KBCANNONUI_H_
#define _KBCANNONUI_H_

/**
 *	CannonHealthBarUIComponent
 */
class CannonHealthBarUIComponent : public kbUIComponent {

	KB_DECLARE_COMPONENT( CannonHealthBarUIComponent, kbUIComponent );

//---------------------------------------------------------------------------------------------------
public:

	void									SetTargetHealth( const float newHealth );

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	int										m_Dummy;

	// Runtime
	float									m_TargetNormalizedHealth;
	float									m_CurrentNormalizedHealth;

	kbStaticModelComponent *				m_pStaticModelComponent;
};

#endif