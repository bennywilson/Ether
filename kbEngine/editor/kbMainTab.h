//===================================================================================================
// kbMainTab.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBEDITORWINDOW_H_
#define _KBEDITORWINDOW_H_


/*
 *	kbMainTab
 */
class kbMainTab : public kbWidget, public Fl_Tabs {
public:
	kbMainTab( int x, int y, int w, int h );

	const kbEditorWindow * GetEditorWindow() const { return m_pEditorWindow; }
	kbEditorWindow * GetGameWindow() const { return m_pGameWindow; }

	virtual void Update();

	virtual void EventCB( const widgetCBObject * widgetCBObject );

	const kbCamera * GetEditorWindowCamera() const { return &m_pEditorWindow->GetCamera(); }

	void AdjustCameraMoveSpeedMultiplier( const float newMultiplier ) { m_CameraMoveSpeedMultiplier = max( min( newMultiplier, 100.0f ), 0.1f ); }

private:

	void InputCB( const widgetCBObject * widgetCBObject );
	void CameraMoveCB( const widgetCBInputObject * widgetCBObject );
	void ObjectSelectedOrMovedCB( const widgetCBInputObject * widgetCBObject );
	void EntityTransformedCB( const widgetCBObject * widgetCBObject );

	kbEditorWindow *			GetCurrentWindow();

	kbEditorWindow *			m_pEditorWindow;
	kbEditorWindow *			m_pModelViewerWindow;
	kbEditorWindow *			m_pGameWindow;

	std::vector< Fl_Group * >	m_Groups;

	//
	const kbModel *				m_pCurrentlySelectedResource;

	kbManipulator				m_Manipulator;

	float						m_CameraMoveSpeedMultiplier;
};

#endif