//==============================================================================
// CapOnItLevelComponent.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _CAPONITLEVELCOMPONENT_H_
#define _CAPONITLEVELCOMPONENT_H_

#include "UI/CannonUI.h"
#include "kbLevelDirector.h"

namespace CapOnIt {

	enum eCapOnIt_State {
		CapOnIt_MainMenu,
		CapOnIt_Skkkkt,
		NumStates
	 };
};

/**
 *	CapOnItLevelComponent
 */
class CapOnItLevelComponent : public CannonLevelComponent {

	KB_DECLARE_COMPONENT( CapOnItLevelComponent, CannonLevelComponent );

public:
	static CapOnItLevelComponent *				Get() { return s_Inst; }

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	void										CapOnItLevelComponent::UpdateDebugAndCheats();

	static CapOnItLevelComponent *				s_Inst;
};

/**
 *	CapOnIt_BaseState
 */
class CapOnIt_BaseState : public StateMachineNode<CapOnIt::eCapOnIt_State> {

//---------------------------------------------------------------------------------------------------
public:
	CapOnIt_BaseState( CapOnItLevelComponent *const pLevelComponent ) : m_pLevelComponent( pLevelComponent ) { }

	virtual void WidgetEventCB( kbUIWidgetComponent *const pWidget, const kbInput_t *const pInput ) { }

protected:
	CapOnItLevelComponent * m_pLevelComponent;
};


/**
 *	CapOnItDirector
 */
class CapOnItDirector : public kbLevelDirector<CapOnIt_BaseState, CapOnIt::eCapOnIt_State>, public ISingleton<CapOnItDirector>, public IUIWidgetListener {

//---------------------------------------------------------------------------------------------------
public:

												CapOnItDirector();
	virtual										~CapOnItDirector();

	virtual void								UpdateStateMachine() override;

	virtual void								StateChangeCB( const CapOnIt::eCapOnIt_State previousState, const CapOnIt::eCapOnIt_State nextState );
	virtual void								WidgetEventCB( kbUIWidgetComponent *const pWidget, const kbInput_t *const pInput );

protected:

	virtual void								InitializeStateMachine_Internal();
	virtual void								ShutdownStateMachine_Internal();
};

#endif