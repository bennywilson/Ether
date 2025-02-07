/// kbWidget.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "kbEditor.h"
#include "kbWidget.h"

/// kbWidget::kbWidget
kbWidget::kbWidget( const int x, const int y, const int w, const int h ) :
	m_X( x ),
	m_Y( y ),
	m_Width( w ),
	m_Height( h ) {
}

/// kbWidget::IsPointWithinBounds
bool kbWidget::IsPointWithinBounds( const int x, const int y ) const {
	return ( x >= m_X && x < m_X + m_Width && y >= m_Y && y < m_Y + m_Height );
}

///  *  kbWidget::DisplayWidth
int kbWidget::DisplayWidth() const {
	return m_Width - 2 * kbEditor::PanelBorderSize();
}

/// kbEditorWindow
kbEditorWindow::kbEditorWindow(	int x, int y, int w, int h, const char * title ) :
	kbWidget( x, y, w, h ),
	Fl_Window( x, y, w, h, title ) {
}

/// kbEditorWindow::Update
void kbEditorWindow::Update() {

}

/// kbEditorWindow::EventCB
void kbEditorWindow::EventCB( const widgetCBType_t cbType ) {

}