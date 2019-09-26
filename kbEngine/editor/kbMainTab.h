//===================================================================================================
// kbMainTab.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBEDITORWINDOW_H_
#define _KBEDITORWINDOW_H_


/**
 *	kbMainTab
 */
class kbMainTab : public kbWidget, public Fl_Tabs {

	friend class kbEditor;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

															kbMainTab( int x, int y, int w, int h );

	const kbEditorWindow *									GetEditorWindow() const { return m_pEditorWindow; }
	kbEditorWindow *										GetGameWindow() const { return m_pGameWindow; }

	virtual void											Update() override;
	virtual void											RenderSync() override;

	virtual void											EventCB( const widgetCBObject *const widgetCBObject );

	kbCamera *												GetEditorWindowCamera() const { return &m_pEditorWindow->GetCamera(); }

	void													SetCameraSpeedMultiplier( const float newMultiplier ) { m_CameraMoveSpeedMultiplier = max( min( newMultiplier, 100.0f ), 0.1f ); }

private:

	void													InputCB( const widgetCBObject *const widgetCBObject );
	void													CameraMoveCB( const widgetCBInputObject *const widgetCBObject );
	void													EntityTransformedCB( const widgetCBObject *const widgetCBObject );
	void													ManipulatorEvent( const bool bClicked, const kbVec2i & mouseXY );

	kbManipulator &											GetManipulator() { return m_Manipulator; }

	kbEditorWindow *										GetCurrentWindow();

	kbEditorWindow *										m_pEditorWindow;
	kbEditorWindow *										m_pModelViewerWindow;
	kbEditorWindow *										m_pGameWindow;

	std::vector<Fl_Group *>									m_Groups;

	//
	const kbModel *											m_pCurrentlySelectedResource;

	kbManipulator											m_Manipulator;

	float													m_CameraMoveSpeedMultiplier;
};

#endif