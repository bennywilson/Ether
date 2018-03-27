//===================================================================================================
// kbWidget.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBWIDGET_H_
#define _KBWIDGET_H_

#include <Windows.h>
#include "kbWidgetCBObjects.h"
#include "DX11/kbRenderer_DX11.h"
#include "kbCamera.h"

#pragma warning(push)
#pragma warning(disable:4312)
#include <FL/Fl.H>
#include <FL/FL_Window.h>
#include <FL/Fl_Text_Display.h>
#include <FL/Fl_Tabs.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Menu_Bar.h>
#include <FL/Fl_Select_Browser.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Check_Button.h>
#include <FL/Fl_File_Chooser.h>
#include <FL/fl_ask.H>
#include <FL/x.H>
#pragma warning(pop)

/**
 *	kbWidget
 */
class kbWidget {
public:
	kbWidget( const int x, const int y, const int width, const int height );
	
	virtual void Update() { }
	virtual void EventCB( const widgetCBObject * widgetCBObject ) { }

	bool IsPointWithinBounds( const int x, const int y );

protected:

	int					DisplayWidth() const;
	unsigned int		FontSize()	const { return 14; }
	unsigned int		LineSpacing() const { return FontSize() + 6; }

private:
	int m_X;
	int m_Y;
	int m_Width;
	int m_Height;
};

/**
 *	kbEditorWindow
 */
class kbEditorWindow : public Fl_Window, public kbWidget {
public:
	kbEditorWindow( int x, int y, int w, int h, const char* title = 0 );

	virtual void Update();
	virtual void EventCB( const widgetCBType_t );

	HWND GetWindowHandle() const { return fl_xid( this ); }


	kbCamera & GetCamera() { return m_Camera; }

private:
	kbCamera m_Camera;
	kbVec3 m_TargetPos;
	kbQuat m_TargetRotation;
};

#endif