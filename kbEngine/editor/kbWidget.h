/// kbWidget.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "kbWidgetCBObjects.h"
#include "kbCamera.h"

#pragma warning(push)
#pragma warning(disable:4312)
#include <FL/FL_Window.h>
#include <FL/x.H>
#pragma warning(pop)

///  kbWidget
class kbWidget {
public:
	kbWidget(const int x, const int y, const int width, const int height);

	virtual void Update() { }
	virtual void  RenderSync() { }
	virtual void EventCB(const widgetCBObject* widgetCBObject) { }
	  
	bool IsPointWithinBounds(const int x, const int y) const;
	 
protected:
	int	DisplayWidth() const;
	unsigned int FontSize()	const { return 14; }
	unsigned int LineSpacing() const { return FontSize() + 6; }

private:
	int m_X;
	int m_Y;
	int m_Width;
	int m_Height;
};

/// kbEditorWindow
class kbEditorWindow : public Fl_Window, public kbWidget {
public:
	kbEditorWindow(int x, int y, int w, int h, const char* title = 0);

	virtual void Update() override;
	virtual void EventCB(const widgetCBType_t);	// TODO this function differs from kbWidgets.  WHY?

	HWND GetWindowHandle() const { return fl_xid(this); }

	kbCamera& GetCamera() { return m_Camera; }

private:
	kbCamera m_Camera;
	kbVec3 m_TargetPos;
	kbQuat m_TargetRotation;
};
