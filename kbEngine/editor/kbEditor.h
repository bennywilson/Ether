//===================================================================================================
// kbEditor.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBEDITOR_H_
#define _KBEDITOR_H_

#include "kbWidget.h"
#include "kbGame.h"
#include "kbUndoAction.h"

class kbWidget;
class kbEditorEntity;
class Fl_Widget;

enum widgetCBType_t;

/**
 *  kbEditor
 */
class kbEditor : Fl_Window {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
															kbEditor();
															~kbEditor();

	void													UnloadMap();
	void													LoadMap( const std::string & mapName );
	void													SetGame( class kbGame * pGame ) { m_pGame = pGame; }

	void													Update();
	virtual int												handle( int theEvent );

	const bool												IsRunning() const { return m_bIsRunning; }
	const bool												IsRunningGame() const { return m_pGame != nullptr && m_pGame->IsPlaying(); }

	void													RegisterUpdate( kbWidget * const widget ) { m_UpdateWidgets.push_back( widget ); }
	void													RegisterEvent( kbWidget *const widget, const widgetCBType_t eventType ) { m_EventReceivers[eventType].push_back( widget ); }
	void													BroadcastEvent( const class widgetCBObject & cbObject );

	void													AddEntity( kbEditorEntity *const pEditorEntity );
	void													SelectEntities( std::vector< kbEditorEntity * > & entitiesToSelect, bool AppendToSelectedList );
	void													DeselectEntities();

	void													PushUndoAction( kbUndoAction * pUndoAction ) { m_UndoStack.Push( pUndoAction ); }
	void													DeleteEntities( std::vector<kbEditorEntity*> & editorEntityList );

	std::vector<kbEditorEntity *> &							GetGameEntities() { return m_GameEntities; }
	std::vector<kbEditorEntity *> &							GetSelectedObjects() { return m_SelectedObjects; }

	const kbPrefab *										GetCurrentlySelectedPrefab() const;

    const widgetCBInputObject &                             GetInput() const { return m_WidgetInputObject; }

private:

	void													SaveLevel_Internal( const std::string & fileName, const bool bForceSave );

	std::string												m_CurrentLevelFileName;

	std::vector<kbWidget *>									m_UpdateWidgets;
	std::map<widgetCBType_t, std::vector< kbWidget  *>>		m_EventReceivers;
	std::vector<kbEditorEntity *>							m_GameEntities;
	std::vector<kbEditorEntity *>							m_SelectedObjects;
	std::vector<kbEditorEntity *>							m_RemovedEntities;

	kbUndoStack												m_UndoStack;

	kbGame *												m_pGame;
	kbEditorWindow *										m_pGameWindow;
	Fl_Button *												m_pSpeedButton;

	// widgets
	class kbMainTab *										m_pMainTab;
	class kbResourceTab *									m_pResourceTab;
	class Fl_Text_Buffer *									m_pOutputText;
	class kbPropertiesTab *									m_pPropertiesTab;
	class Fl_Choice *										m_pViewModeChoice;

	kbTimer													m_Timer;

	// input
	widgetCBInputObject										m_WidgetInputObject;

	bool													m_bIsRunning;
	bool													m_bRightMouseButtonDragged;

	// Stores a copy of the current undo action's id.  The level is dirty if the two values don't match
	UINT64													m_UndoIDAtLastSave;

	// internal functions and callbacks
	void													ShutDown();

	static void												NewLevel( Fl_Widget *, void * );
	static void												OpenLevel( Fl_Widget *, void * );
	static void												SaveLevelAs( Fl_Widget *, void * );
	static void												SaveLevel( Fl_Widget *, void * );

	static void												Undo( Fl_Widget *, void * );
	static void												Redo( Fl_Widget *, void * );
	static void												Close( Fl_Widget *, void * );
	static void												CreateGameEntity( Fl_Widget *, void * );
	static void												AddComponent( Fl_Widget *, void * );
	static void												TranslationButtonCB( Fl_Widget *, void * );
	static void												RotationButtonCB( Fl_Widget *, void * );
	static void												ScaleButtonCB( Fl_Widget *, void * );
	static void												AdjustCameraSpeedCB( Fl_Widget *, void * );
	static void												OutputCB( kbOutputMessageType_t, const char * );
	static void												PlayGameFromHere( Fl_Widget *, void * );
	static void												StopGame( Fl_Widget *, void * );
	static void												DeleteEntitiesCB( Fl_Widget *, void * );
	static void												ViewModeChoiceCB( Fl_Widget *, void * );

	void													RightClickPopUpMenu();
	static void												ReplaceCurrentlySelectedPrefab( Fl_Widget *, void * );
	static void												AddEntityAsPrefab( Fl_Widget *, void * );
	void													AddEntityAsPrefab_Internal( const std::string & PackageName, const std::string & FolderName, const std::string & PrefabeName );
	static void												InsertSelectedPrefabIntoScene( Fl_Widget *, void * );

public:

	static const int										TabHeight() { return 25; }
	static const int										PanelBorderSize( int Multiplier = 1 ) { return 5 * Multiplier; }
	static const int										LineSpacing( int Multiplier = 1 ) { return Multiplier * ( fl_height() + PanelBorderSize() ); }
};

extern kbEditor * g_Editor;

/**
 * kbDialogBox
 */
class kbDialogBox
{

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	kbDialogBox( const char *const title, const char *const acceptButtonName, const char *const cancelButtonName ) :
		m_PopUpWindow( nullptr ),
		m_Title( title ),
		m_AcceptButtonName( acceptButtonName ),
		m_CancelButtonName( cancelButtonName ),
		m_LineNumber( 0 ),
		m_bAccepted( false ) {

		if ( gCurrentDialogBox != nullptr ) {
			kbError( "Dialog box already open.  Only one allowed at a time" );
		}
	
		gCurrentDialogBox = this;
   }

   ~kbDialogBox() {
		gCurrentDialogBox = nullptr;
   }

   void AddTextField( const char *const field ) {
		kbDialogBoxField newField;
		newField.m_FieldName = field;
		m_Fields.push_back( newField );
   }

   bool Run() {
		kbErrorCheck( m_Fields.size() > 0, "Dialog box has no fields" );
		kbErrorCheck( gCurrentDialogBox != nullptr, "Run called on an invalid dialog box" );

		const int ButtonHeight = fl_height() + kbEditor::PanelBorderSize() * 2;
		const int popUpWidth = 600;
		const int popUpHeight = ( ( int ) m_Fields.size() + 2 ) * kbEditor::LineSpacing() + ButtonHeight;
		int dx, dy, w, h;
		int MaxNameLength = 0;

		for ( size_t i = 0; i < m_Fields.size(); i++ ) {
			fl_text_extents( m_Fields[i].m_FieldName.c_str(), dx, dy, w, h );

			if ( w > MaxNameLength ) {
				MaxNameLength = w;
			}
		}

		m_PopUpWindow = new Fl_Window( Fl::event_x(), Fl::event_y(), popUpWidth, popUpHeight );
		const int StartX = 2 * kbEditor::PanelBorderSize() + MaxNameLength;
		const int StartY = kbEditor::PanelBorderSize();

		for ( int i = 0; i < m_Fields.size(); i++ ) {
			m_Fields[i].m_Input = new Fl_Input( StartX, StartY + kbEditor::LineSpacing() * i, popUpWidth - ( MaxNameLength + kbEditor::PanelBorderSize() * 5 ), 20, m_Fields[i].m_FieldName.c_str() );
		}

		int MaxButtonNameLen = 0;
		fl_text_extents( m_AcceptButtonName.c_str(), dx, dy, MaxButtonNameLen, h );
		fl_text_extents( m_CancelButtonName.c_str(), dx, dy, w, h );

		if ( w > MaxButtonNameLen ) {
			MaxButtonNameLen = w;
		}

		Fl_Button *const pAcceptButton = new Fl_Button( StartX, StartY + kbEditor::LineSpacing( 4 ), MaxButtonNameLen + kbEditor::PanelBorderSize( 2 ), ButtonHeight, m_AcceptButtonName.c_str() );
		Fl_Button *const pCancelButton = new Fl_Button( StartX + MaxButtonNameLen + kbEditor::PanelBorderSize()* 4, StartY + kbEditor::LineSpacing( 4 ), MaxButtonNameLen, ButtonHeight, m_CancelButtonName.c_str() );

		pAcceptButton->callback( AcceptButtonClicked );
		pCancelButton->callback( CancelButtonClicked );

		m_PopUpWindow->show();

		while( m_PopUpWindow->shown() ) { Fl::wait(); }

		for ( size_t i = 0; i < m_Fields.size(); i++ ) {
			m_Fields[i].m_FieldValue = m_Fields[i].m_Input->value();
		}

		delete m_PopUpWindow;
		m_PopUpWindow = nullptr;
		gCurrentDialogBox = nullptr;

		return m_bAccepted;
	}

	const std::string & GetFieldEntry( const int fieldIdx ) const {
		if ( fieldIdx < 0 || fieldIdx >= m_Fields.size() ) {
			kbError( "Bad field idx %d for dialog box", fieldIdx );
		}

		return m_Fields[fieldIdx].m_FieldValue;
	}

private:

	Fl_Window *    m_PopUpWindow;
	int            m_LineNumber;

	struct kbDialogBoxField {
		std::string    m_FieldName;
		std::string    m_FieldValue;
		Fl_Input *     m_Input;
	};

	std::string                      m_Title;
	std::string                      m_AcceptButtonName;
	std::string                      m_CancelButtonName;
	std::vector<kbDialogBoxField>    m_Fields;
	bool                             m_bAccepted;

	void Exit() {
		m_PopUpWindow->hide();
	}

	static void AcceptButtonClicked( Fl_Widget*, void * userdata ) {
		gCurrentDialogBox->m_bAccepted = true;
		gCurrentDialogBox->Exit();
	}

	static void CancelButtonClicked( Fl_Widget*, void * userdata ) {
		gCurrentDialogBox->m_bAccepted = false;
		gCurrentDialogBox->Exit();
	}

	static kbDialogBox * gCurrentDialogBox;
};

#endif