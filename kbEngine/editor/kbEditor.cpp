//===================================================================================================
// kbEditor.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "DX11/kbRenderer_DX11.h"
#include "kbGame.h"
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbWidget.h"
#include "kbManipulator.h"
#include "kbMainTab.h"
#include "kbResourceTab.h"
#include "kbTypeInfo.h"
#include "kbPropertiesTab.h"
#include "kbEditor.h"
#include "kbModelComponent.h"
#include "kbEditorEntity.h"

// fltk
#pragma warning(push)
#pragma warning(disable:4312)
#include "FL/fl_ask.h"
#pragma warning(pop)

kbEditor * g_Editor = nullptr;
kbDialogBox * kbDialogBox::gCurrentDialogBox = nullptr;
bool g_bEditorIsUndoingAnAction = false;

Fl_Text_Buffer * g_OutputBuffer = nullptr;
Fl_Text_Buffer * g_StyleBuffer = nullptr;
static Fl_Text_Display::Style_Table_Entry stable[] = {  { FL_BLACK,	FL_HELVETICA, 12 },				// A
														{ FL_RED,		FL_HELVETICA, 12 } };		// B
/**
 * kbEditor
 */
kbEditor::kbEditor() :
	Fl_Window( 0, 0, GetSystemMetrics( SM_CXFULLSCREEN ), GetSystemMetrics( SM_CYFULLSCREEN ) ) {

	outputCB = kbEditor::OutputCB;
	m_UndoIDAtLastSave = UINT64_MAX;
	m_CurrentLevelFileName = "Untitled";

	g_Editor = this;

	m_pGame = nullptr;
	m_pGameWindow = nullptr;

	const int Screen_Width = GetSystemMetrics( SM_CXFULLSCREEN );
	const int Screen_Height = GetSystemMetrics( SM_CYFULLSCREEN );
	const int Menu_Bar_Height = 20;
	const int Menu_Buttons_Height = 30;
	const int Left_Panel = 200;
	const int Bottom_Panel_Height = 125;
	const int Panel_Border_size = 5;
	const int Right_Panel = 300;

	// Output display
	Fl_Text_Display * display = new Fl_Text_Display( 5, Screen_Height - Bottom_Panel_Height + Panel_Border_size * 4, Screen_Width - 5, Bottom_Panel_Height - Panel_Border_size - Panel_Border_size, "DISPLAY!"  );
	g_OutputBuffer = new Fl_Text_Buffer();
	display->buffer( g_OutputBuffer );
	
	g_StyleBuffer = new Fl_Text_Buffer();

	int stable_size = sizeof(stable)/sizeof(stable[0]);

	display->highlight_data( g_StyleBuffer, stable, stable_size, 'A', 0, 0 );

	// menu bar
	Fl_Menu_Bar * mainMenuBar = new Fl_Menu_Bar( 0, 0, Screen_Width, Menu_Bar_Height );
	mainMenuBar->add( "File/New Level", FL_CTRL+'n', NewLevel );
	mainMenuBar->add( "File/Open Level", FL_CTRL+'o', OpenLevel );
	mainMenuBar->add( "File/Save Level As", 0, SaveLevelAs );
	mainMenuBar->add( "File/Save", FL_CTRL+'s', SaveLevel );
	mainMenuBar->add( "Edit/Undo", FL_CTRL+'z', Undo );
	mainMenuBar->add( "Edit/Redo", FL_CTRL+'y', Redo );

	mainMenuBar->add( "Edit/Delete", FL_Delete, DeleteEntities );

	mainMenuBar->add( "File/Quit",   FL_CTRL+'q', Close, this );
	mainMenuBar->add( "Edit/Change", FL_CTRL+'c', NULL) ;
	mainMenuBar->add( "Edit/Submenu/Aaa" );
	mainMenuBar->add( "Edit/Submenu/Bbb" );
	mainMenuBar->add( "Add/Entity", FL_CTRL+'g', CreateGameEntity, this );
		
	std::map<std::string, const kbTypeInfoClass *> & componentMap = g_NameToTypeInfoMap->GetClassMap(); 
	std::map<std::string, const kbTypeInfoClass *>::iterator iter;

	for (iter = componentMap.begin(); iter != componentMap.end(); ++iter ) {
		std::string MenuName = "Add/Component/";
		MenuName += iter->second->GetClassNameA().c_str();
		mainMenuBar->add( MenuName.c_str(), FL_CTRL+'g', AddComponent, (void*)iter->second);	// Hack cast - unfortunate
	}
	
	mainMenuBar->add( "Play/Play Game From Here", FL_CTRL+'g', PlayGameFromHere );
	mainMenuBar->add( "Play/Stop Game", 0, StopGame );

	// buttons
	Fl_Button * translationButton = new Fl_Button( Left_Panel + 5, Menu_Bar_Height + 5, 25, Menu_Buttons_Height - 10, "T" );
	translationButton->callback( TranslationButtonCB );

	Fl_Button * rotationButton = new Fl_Button( Left_Panel + 35, Menu_Bar_Height + 5, 25, Menu_Buttons_Height - 10, "R" );
	rotationButton->callback( RotationButtonCB );

	Fl_Button * scaleButton = new Fl_Button( Left_Panel + 65, Menu_Bar_Height + 5, 25, Menu_Buttons_Height - 10, "S" );
	scaleButton->callback( ScaleButtonCB );

	m_pSpeedButton = new Fl_Button( Left_Panel + 95, Menu_Bar_Height + 5, 85, Menu_Buttons_Height - 10, "Speedx1" );

	m_pSpeedButton->callback( AdjustCameraSpeedCB );

	// main tab
	m_pMainTab = new kbMainTab( Left_Panel + 5, Menu_Bar_Height + Menu_Buttons_Height, Screen_Width - Left_Panel - Right_Panel, Screen_Height - Menu_Bar_Height - Menu_Bar_Height - Bottom_Panel_Height );

	// resource tab
	m_pResourceTab = new kbResourceTab( 0, Menu_Bar_Height + Menu_Buttons_Height, Left_Panel, Screen_Height - Menu_Bar_Height - Menu_Bar_Height - Bottom_Panel_Height );

	// properties tab
	m_pPropertiesTab = new kbPropertiesTab( Screen_Width - Right_Panel, 
					Menu_Bar_Height + Menu_Buttons_Height, 
					Right_Panel,
					Screen_Height - Menu_Bar_Height - Menu_Bar_Height - Bottom_Panel_Height + 5 );

	end();
	show();

	//buff->text( "asdlkjasldkjalskdjlaskjd" );
	//Fl::run();

	// setup the renderer
	if ( g_pRenderer == nullptr ) {
		g_pRenderer = new kbRenderer_DX11();
		g_pRenderer->Init( m_pMainTab->GetEditorWindow()->GetWindowHandle(), 1400, 833, false, false);
	}

	m_pResourceTab->PostRendererInit();

	m_IsRunning = true;

	m_Timer.Reset();

	// reserve textures
	g_pRenderer->LoadTexture( "../../kbEngine/assets/Textures/Editor/EntityIcon.jpg", 1 );
	g_pRenderer->LoadTexture( "../../kbEngine/assets/Textures/Editor/directionalLightIcon.jpg", 2 );

	SetWindowText( m_pMainTab->GetEditorWindow()->GetWindowHandle(), "kbEditor" );
}

/**
 * ~kbEditor
 */
kbEditor::~kbEditor() {
	ShutDown();
}

/**
 *  kbEditor::UnloadMap()
 */
void kbEditor::UnloadMap() {

	if ( g_pRenderer != nullptr ) {
		g_pRenderer->WaitForRenderingToComplete();
	}

	// Remove old entities
	DeselectEntities();

	for ( int i = 0; i < g_Editor->m_GameEntities.size(); i++ ) {
		delete m_GameEntities[i];
	}
	m_GameEntities.clear();

	m_CurrentLevelFileName = "Untitled";
	m_UndoIDAtLastSave = UINT64_MAX;
	m_UndoStack.Reset();
}

/**
 *  kbEditor::LoadMap()
 */
void kbEditor::LoadMap( const std::string & InMapName ) {

	UnloadMap();

	// Load map
	if ( InMapName.empty() == false ) {
		m_CurrentLevelFileName = InMapName;

		TCHAR NPath[MAX_PATH];

		GetCurrentDirectory( MAX_PATH, NPath );

		WIN32_FIND_DATA fdFile;
		HANDLE hFind = nullptr;

		std::string LevelPath = NPath;
		LevelPath += "/Levels/";
		std::string curLevelFolder = "";

		if ( m_CurrentLevelFileName.find( "." ) == std::string::npos ) {
			m_CurrentLevelFileName += ".kbLevel";
		}	

		hFind = FindFirstFile( ( LevelPath + "*" ).c_str(), &fdFile );
		BOOL nextFileFound = ( hFind != INVALID_HANDLE_VALUE  );
		do {
			m_CurrentLevelFileName = LevelPath + curLevelFolder + m_CurrentLevelFileName;
				
			kbFile inFile;		
			if ( inFile.Open( m_CurrentLevelFileName.c_str(), kbFile::FT_Read ) ) {

				kbGameEntity * gameEntity = inFile.ReadGameEntity();
				while ( gameEntity != nullptr ) {

					kbEditorEntity *const newEditorEntity = new kbEditorEntity( gameEntity );
					g_Editor->m_GameEntities.push_back( newEditorEntity );
					gameEntity = inFile.ReadGameEntity();
				}
				inFile.Close();
				
				const std::string windowText = "kbEditor - " + InMapName;
				SetWindowText( fl_xid( this ), windowText.c_str() );

				break;
			}

			if ( nextFileFound == FALSE ) {
				break;
			}

			do {
				if ( ( fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 && strcmp( fdFile.cFileName, "." ) != 0 && strcmp( fdFile.cFileName, ".." ) != 0 ) {
					curLevelFolder = "/";
					curLevelFolder += fdFile.cFileName;
					curLevelFolder += "/";
					break;
				}

			} while( nextFileFound = FindNextFile( hFind, &fdFile ) != FALSE );

			if ( nextFileFound != 0 ) {
				nextFileFound = FindNextFile( hFind, &fdFile );
			}
		} while( true );
	}

	m_UndoStack.Reset();
}

/**
 *  kbEditor::Update()
 */
void kbEditor::Update() {

	if ( m_IsRunning == false ) {
		return;
	}

	for ( int i = 0; i < m_UpdateWidgets.size(); i++ ) {
		m_UpdateWidgets[i]->Update();
	}

	// Wait for rendering to complete, sync up any game objects that need it and kick off a new scene to render
	g_pRenderer->WaitForRenderingToComplete();

	// Update editor entities and components
	for ( int i = 0; i < m_GameEntities.size(); i++ ) {
		m_GameEntities[i]->RenderSync();
	}
	
	// Remove any undeleted actors
	if ( m_RemovedEntities.size() > 0 ) {
		std::vector<kbUndoDeleteActor::DeletedActorInfo_t> deletedEntities;
		for ( int i = 0; i < m_RemovedEntities.size(); i++ ) {
			VectorRemoveFast( m_GameEntities, m_RemovedEntities[i] );

			kbUndoDeleteActor::DeletedActorInfo_t deletedActor;
			deletedActor.m_pEditorEntity = m_RemovedEntities[i];

			for ( int j = 0; j < m_RemovedEntities[i]->GetGameEntity()->NumComponents(); j++ ) {
				deletedActor.m_bComponentEnabled.push_back( m_RemovedEntities[i]->GetGameEntity()->GetComponent(j)->IsEnabled() );
				m_RemovedEntities[i]->GetGameEntity()->GetComponent(j)->Enable( false );
				m_RemovedEntities[i]->RenderSync();
			}

			deletedEntities.push_back( deletedActor );
		}

		g_Editor->GetSelectedObjects().clear();
		g_Editor->m_UndoStack.Push( new kbUndoDeleteActor( deletedEntities ) );

		g_Editor->BroadcastEvent( widgetCBEntityDeselected() );
		m_RemovedEntities.clear();
	}

	g_pRenderer->RenderSync();

	g_ResourceManager.RenderSync();

	g_pRenderer->SetReadyToRender();

	//m_pMainTab->GetCurrentWindow()->GetCamera().Update();

	if ( GetFocus() == fl_xid( this ) ) {
		// input
		if ( GetAsyncKeyState( 'W' ) ) {
			m_WidgetInputObject.keys.push_back( widgetCBInputObject::keyType_t::WidgetInput_Forward );
		} else if ( GetAsyncKeyState( 'S' ) ) {
			m_WidgetInputObject.keys.push_back( widgetCBInputObject::keyType_t::WidgetInput_Back );
		} 
	
		if ( GetAsyncKeyState( 'A' ) ) {
			m_WidgetInputObject.keys.push_back( widgetCBInputObject::keyType_t::WidgetInput_Left );
		} else if ( GetAsyncKeyState( 'D' ) ) {
			m_WidgetInputObject.keys.push_back( widgetCBInputObject::keyType_t::WidgetInput_Right );
		}

		if ( GetAsyncKeyState( VK_LCONTROL ) ) {
			m_WidgetInputObject.keys.push_back( widgetCBInputObject::keyType_t::WidgetInput_Ctrl );
		}

		if ( GetAsyncKeyState( VK_LSHIFT ) ) {
			m_WidgetInputObject.keys.push_back( widgetCBInputObject::keyType_t::WidgetInput_Shift );
		}

		if ( m_WidgetInputObject.keys.size() > 0 || m_WidgetInputObject.mouseDeltaX != 0 || m_WidgetInputObject.mouseDeltaY != 0 ||
			m_WidgetInputObject.leftMouseButtonDown || m_WidgetInputObject.rightMouseButtonDown ) {
			BroadcastEvent( m_WidgetInputObject );
		}
	}

	m_WidgetInputObject.ClearKeys();
	m_WidgetInputObject.mouseDeltaX = 0;
	m_WidgetInputObject.mouseDeltaY = 0;
	m_WidgetInputObject.leftMouseButtonPressed = false;
	m_WidgetInputObject.rightMouseButtonPressed = false;

	Fl::flush();

	float DT = m_Timer.TimeElapsedSeconds();
	m_Timer.Reset();

	if ( DT > 0.05f ) {
		DT = 0.05f;
	}
	// Update editor entities and components
	for ( int i = 0; i < m_GameEntities.size(); i++ ) {
		m_GameEntities[i]->Update( DT );
	}

	// Update title bar dirty status
	if ( m_UndoIDAtLastSave != m_UndoStack.GetLastDirtyActionId() ) {
		SetWindowText( fl_xid( this ), ( "kbEditor - " + m_CurrentLevelFileName + "*" ).c_str() );
	} else {
		SetWindowText( fl_xid( this ), ( "kbEditor - " + m_CurrentLevelFileName ).c_str() );
	}
}

/**
 *	ShutDown
 */
void kbEditor::ShutDown() {

	if ( m_IsRunning == false ) {
		return;
	}

	for ( int i = 0; i < m_GameEntities.size(); i++ ) {
		delete m_GameEntities[i];
	}
	m_GameEntities.clear();

	g_ResourceManager.Shutdown();

	m_IsRunning = false;
}

/**
 *  kbEditor::BroadcastEvent
 */
void kbEditor::BroadcastEvent( const widgetCBObject & cbObject ) {

	std::vector< kbWidget * > & receivers = m_EventReceivers[cbObject.widgetType];

	for ( int i = 0; i < receivers.size(); i++ ) {
		receivers[i]->EventCB( &cbObject );
	}
}

/**
 *  kbEditor::DeselectEntities
 */	
void kbEditor::DeselectEntities() {

	for ( int i = 0; i < m_GameEntities.size(); i++ ) {
		m_GameEntities[i]->SetIsSelected( false );
	}

	m_SelectedObjects.clear();
	g_Editor->BroadcastEvent( widgetCBEntityDeselected() ); 
}

/**
 *  kbEditor::SelectEntities
 */
void kbEditor::SelectEntities( std::vector< kbEditorEntity * > & entitiesToSelect, const bool bAppendToSelectedEntities ) {

	if ( g_bEditorIsUndoingAnAction == false ) {
		m_UndoStack.Push( new kbUndoSelectActor( m_SelectedObjects, entitiesToSelect ) );
	}

	if ( bAppendToSelectedEntities == false ) {
		DeselectEntities();
	}

	for ( int i = 0; i < entitiesToSelect.size(); i++ ) {
		entitiesToSelect[i]->SetIsSelected( true );
	}

	m_SelectedObjects.insert( m_SelectedObjects.end(), entitiesToSelect.begin(), entitiesToSelect.end() );

	widgetCBEntitySelected entitySelectedCB;
	entitySelectedCB.entitiesSelected = entitiesToSelect;

	g_Editor->BroadcastEvent( entitySelectedCB );
}

/**
 *	kbEditor::handle
 */
int kbEditor::handle( int theEvent ) {

	const int button = Fl::event_button();
	const int state = Fl::event_state();

	int newMouseX = Fl::event_x();
	int newMouseY = Fl::event_y();

	// check for right mouse button
	if ( theEvent == FL_PUSH ) {
		m_bRightMouseButtonDragged = false;

		// don't allow both buttons to be down
		/*if ( ( button == 3 && m_WidgetInputObject.leftMouseButtonDown ) ||
			( button == 1 && m_WidgetInputObject.rightMouseButtonDown ) ) {
				return 1;
		}*/

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;

		if ( button == 3 ) {
			m_WidgetInputObject.rightMouseButtonDown = true;
			m_WidgetInputObject.rightMouseButtonPressed = true;
		} else if ( button == 1 ) {
			m_WidgetInputObject.leftMouseButtonDown = true;
			m_WidgetInputObject.leftMouseButtonPressed = true;
		}

		Fl_Window::handle( theEvent );
		return 1;
	} else if ( theEvent == FL_RELEASE ) {

		if ( newMouseX >= m_pMainTab->x() && newMouseY >= m_pMainTab->y() && newMouseX < m_pMainTab->x() + m_pMainTab->w() && newMouseY < m_pMainTab->y() + m_pMainTab->h() ) {
			if ( m_WidgetInputObject.rightMouseButtonDown && m_bRightMouseButtonDragged == false ) {
				RightClickPopUpMenu();
			}
		}

		m_WidgetInputObject.leftMouseButtonDown = false;
		m_WidgetInputObject.leftMouseButtonPressed = false;
		m_WidgetInputObject.rightMouseButtonDown = false;
		m_WidgetInputObject.rightMouseButtonPressed = false;
		Fl_Window::handle( theEvent );
		return 1;
	} else if ( m_WidgetInputObject.leftMouseButtonDown && theEvent == FL_DRAG ) {
		m_WidgetInputObject.mouseDeltaX = newMouseX - m_WidgetInputObject.mouseX;
		m_WidgetInputObject.mouseDeltaY = newMouseY - m_WidgetInputObject.mouseY;

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;
	} else if ( m_WidgetInputObject.rightMouseButtonDown && theEvent == FL_DRAG ) {
		m_bRightMouseButtonDragged = true;
		m_WidgetInputObject.mouseDeltaX = newMouseX - m_WidgetInputObject.mouseX;
		m_WidgetInputObject.mouseDeltaY = newMouseY - m_WidgetInputObject.mouseY;

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;
		HWND hWnd = fl_xid( this );
		RECT rc;
		GetClientRect( hWnd, &rc );

		const int leftBorder = rc.left + 10;
		const int rightBorder = rc.right - 10;
		const int topBorder = rc.top + 10;
		const int bottomBorder = rc.bottom - 10;

		bool updateCursor = false;

		if ( newMouseX < leftBorder ) {
			updateCursor = true;
			newMouseX = rightBorder;
		} else if ( newMouseX > rightBorder ) {
			updateCursor = true;
			newMouseX = leftBorder;
		}
		
		if ( newMouseY < topBorder ) {
			updateCursor = true;
			newMouseY = bottomBorder;
		} else if ( newMouseY > bottomBorder ) {
			updateCursor = true;
			newMouseY = topBorder;
		}

		if ( updateCursor ) {
			POINT point;
			point.x = (LONG)newMouseX;
			point.y = (LONG)newMouseY;

			ClientToScreen( hWnd, &point );
			SetCursorPos( point.x, point.y );
		}

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;
	} else if ( m_WidgetInputObject.rightMouseButtonDown ) {

	}

	return Fl_Window::handle( theEvent );
}

/**
 *  kbEditor::Close
 */
void kbEditor::Close( Fl_Widget * widget, void * thisPtr ) {
	kbEditor * editor = static_cast< kbEditor * >( thisPtr );

	editor->ShutDown();
}

/**
 *	kbEditor::CreateGameEntity
 */
void kbEditor::CreateGameEntity( Fl_Widget * widget, void * thisPtr ) {
	kbEditor * editor = static_cast< kbEditor * >( thisPtr );

	const kbCamera * editorCamera = editor->m_pMainTab->GetEditorWindowCamera();

	if ( editorCamera == NULL ) {
		return;
	}

	kbVec3 entityLocation = editorCamera->m_Position + ( editorCamera->m_Rotation.ToMat4()[2] * 4.0f ).ToVec3();

	kbEditorEntity * pEditorEntity = new kbEditorEntity();
	pEditorEntity->SetPosition( entityLocation );

	editor->m_GameEntities.push_back( pEditorEntity );
}

/**
 *	kbEditor::AddComponent
 */
void kbEditor::AddComponent( Fl_Widget * widget, void * voidPtr ) {
	if ( voidPtr == nullptr || g_Editor == nullptr )
		return;

	kbTypeInfoClass *const typeInfoClass = static_cast< kbTypeInfoClass * >( voidPtr );
	std::vector<kbEditorEntity *> & selectedObjects =	g_Editor->GetSelectedObjects();

	if ( selectedObjects.size() > 0 ) {
		kbGameComponent *const newComponent = (kbGameComponent*)typeInfoClass->ConstructInstance();		// ENTITY HACK

		const_cast< kbGameEntity * >( selectedObjects[0]->GetGameEntity() )->AddComponent( newComponent );
		newComponent->Enable( true );

		widgetCBObject widgetCB;
		widgetCB.widgetType = WidgetCB_ComponentCreated;
		g_Editor->BroadcastEvent( widgetCB );
	}
}

/**
 *	kbEditor::TranslationButtonCB
 */
void kbEditor::TranslationButtonCB( class Fl_Widget *, void * ) {
	widgetCBObject cbObject;
	cbObject.widgetType = WidgetCB_TranslationButtonPressed;
	g_Editor->BroadcastEvent( cbObject );
}

/**
 *	kbEditor::RotationButtonCB
 */
void kbEditor::RotationButtonCB( class Fl_Widget *, void * ) {
	widgetCBObject cbObject;
	cbObject.widgetType = WidgetCB_RotationButtonPressed;
	g_Editor->BroadcastEvent( cbObject );
}

/**
 *	kbEditor::ScaleButtonCB
 */
void kbEditor::ScaleButtonCB( class Fl_Widget *, void * ) {
	widgetCBObject cbObject;
	cbObject.widgetType = WidgetCB_ScaleButtonPressed;
	g_Editor->BroadcastEvent( cbObject );
}

/**
 *	kbEditor::AdjustCameraSpeedCB
 */
void kbEditor::AdjustCameraSpeedCB( class Fl_Widget * widget, void * ) {

	float multiplier = 1.0f;

	if ( strstr( widget->label(), "Speedx15" ) ) {
		multiplier = 1.0f;
		g_Editor->m_pSpeedButton->label( "Speedx1" );
	} else if ( strstr( widget->label(), "Speedx1" ) ) {
		multiplier = 5.0f;
		g_Editor->m_pSpeedButton->label( "Speedx5" );
	}
	else if ( strstr( widget->label(), "Speedx5" ) )
	{
		multiplier = 15.0f;
		g_Editor->m_pSpeedButton->label( "Speedx15" );
	}

	g_Editor->m_pMainTab->AdjustCameraMoveSpeedMultiplier( multiplier );
}

/**
 *	kbEditor::NewLevel
 */
void kbEditor::NewLevel( Fl_Widget *, void * ) {
	const int areYouSure = fl_ask( "Creating a new level.  Any unsaved changes will be lost.  Are you sure?" );
	if ( areYouSure == 0 ) {
		return;
	}

	g_Editor->UnloadMap();

	g_Editor->m_GameEntities.clear();
	g_Editor->DeselectEntities();
}

/**
 *	kbEditor::OpenLevel
 */
void kbEditor::OpenLevel( class Fl_Widget *, void * ) {

	Fl_File_Chooser fileChooser( ".", "*.kbLevel", Fl_File_Chooser::SINGLE, "Open Level" );

	std::string currentDir = fileChooser.directory();
	currentDir += "/levels";
	fileChooser.directory( currentDir.c_str() );

	fileChooser.show();

	while(fileChooser.shown()) { Fl::wait(); }

	const char * fileName = fileChooser.value();

	if ( fileName == nullptr ) {
		return;
	}

	const int areYouSure = fl_ask( "You have unsaved changes.  Are you sure you want to open a new level?" );
	if ( areYouSure == false )	{
		return;
	}

	g_pRenderer->WaitForRenderingToComplete();

	// Remove old entities
	g_Editor->DeselectEntities();

	for ( int i = 0; i < g_Editor->m_GameEntities.size(); i++ ) {
		delete g_Editor->m_GameEntities[i];
	}
	g_Editor->m_GameEntities.clear();


	std::string fileNameStr = fileName;
	const size_t pos = fileNameStr.find_last_of( "\\/" );
	if ( pos != std::string::npos ) {
		fileNameStr = fileNameStr.substr( pos + 1, fileNameStr.length() - pos );
	}
	g_Editor->LoadMap( fileNameStr.c_str() );
}

/**
 *	kbEditor::SaveLevel_Internal
 */
void kbEditor::SaveLevel_Internal( const std::string & fileNameStr, const bool bForceSave ) {
	
	if ( bForceSave == false ) {
		std::ifstream f( fileNameStr.c_str() );
		if ( f.good() ) {
			const int overWriteIt = fl_ask( "File already exists.  Do you wish to overwrite it?" );
			if ( overWriteIt == 0 ) {
				f.close();
				return;
			}
		}
		f.close();
	}

	kbFile outFile;
	outFile.Open( fileNameStr.c_str(), kbFile::FT_Write );

	for ( int i = 0; i < g_Editor->m_GameEntities.size(); i++ ) {
		outFile.WriteGameEntity( g_Editor->m_GameEntities[i]->GetGameEntity() );
	}

	outFile.Close();

	m_UndoIDAtLastSave = m_UndoStack.GetLastDirtyActionId();
}

/**
 *	kbEditor::SaveLevelAs
 */
void kbEditor::SaveLevelAs( class Fl_Widget *, void * ) {

	Fl_File_Chooser fileChooser( ".", "*.kbLevel", Fl_File_Chooser::CREATE, "Save Level" );

	std::string currentDir = fileChooser.directory();
	currentDir += "/levels";
	fileChooser.directory( currentDir.c_str() );
	
	fileChooser.show();

	while( fileChooser.shown() ) { Fl::wait(); }
	
	std::string fileName = fileChooser.value();
	if ( fileName.empty() ) {
		return;
	}

	const std::string fileExt = GetFileExtension( fileName );
	if ( fileExt != "kbLevel" && fileExt != "kblevel" ) {
		fileName += ".kblevel";
	}
	g_Editor->SaveLevel_Internal( fileName, false );
}

/**
 *	kbEditor::SaveLevel
 */
void kbEditor::SaveLevel( class Fl_Widget *, void * ) {

	if ( g_Editor->m_CurrentLevelFileName.empty() ) {
		return;
	}

	g_Editor->SaveLevel_Internal( g_Editor->m_CurrentLevelFileName, true );
}

/**
 *	kbEditor::Undo
 */
void kbEditor::Undo( class Fl_Widget *, void * ) {
	g_Editor->m_UndoStack.Undo();
	g_Editor->m_pPropertiesTab->RequestRefreshNextUpdate();
}

/**
 *	kbEditor::Redo
 */
 void kbEditor::Redo( class Fl_Widget *, void * ) {
	g_Editor->m_UndoStack.Redo();
	g_Editor->m_pPropertiesTab->RequestRefreshNextUpdate();
}

/**
 *	kbEditor::PlayGameFromHere
 */
void kbEditor::PlayGameFromHere( class Fl_Widget *, void * ) {
	if ( g_Editor == NULL || g_Editor->m_pGame == NULL || g_Editor->m_pGame->IsPlaying() ) {
		return;
	}

	std::vector< const kbGameEntity * > GameEntitiesList;

	for ( int i = 0; i < g_Editor->m_GameEntities.size(); i++ ) {
		GameEntitiesList.push_back( g_Editor->m_GameEntities[i]->GetGameEntity() );
	}

	g_Editor->m_pMainTab->GetGameWindow()->show();
	g_pRenderer->CreateRenderView( g_Editor->m_pMainTab->GetGameWindow()->GetWindowHandle() );
	g_Editor->m_pGame->InitGame( g_Editor->m_pMainTab->GetGameWindow()->GetWindowHandle(), 1600, 900, GameEntitiesList );

	widgetCBObject widgetCB;
	widgetCB.widgetType = WidgetCB_GameStarted;
	g_Editor->BroadcastEvent( widgetCB );
	g_Editor->show();
	Fl::check();

}

/**
 *	kbEditor::DeleteEntities
 */
void kbEditor::DeleteEntities( class Fl_Widget *, void * ) {

	std::vector< kbEditorEntity * > SelectedObjects = g_Editor->GetSelectedObjects();
	std::vector<kbUndoDeleteActor::DeletedActorInfo_t> deletedEntities;

	for ( int i = 0; i < SelectedObjects.size(); i++ ) {
		g_Editor->m_RemovedEntities.push_back( SelectedObjects[i] );
	}
}

/**
 *	kbEditor::StopGame
 */
void kbEditor::StopGame( class Fl_Widget *, void * ) {
	if ( g_Editor == NULL || g_Editor->m_pGame == NULL || g_Editor->m_pGame->IsPlaying() == false ) {
		return;
	}

	g_Editor->m_pGame->StopGame();

	delete g_Editor->m_pGameWindow;
	g_Editor->m_pGameWindow = NULL;

	g_pRenderer->SetRenderWindow( NULL );

	widgetCBObject widgetCB;
	widgetCB.widgetType = WidgetCB_GameStopped;
	g_Editor->BroadcastEvent( widgetCB );
}

/**
 * kbEditor::OutputCB
 */
void kbEditor::OutputCB( kbOutputMessageType_t messageType, const char * output ) {
	// Spin in-case this was called by a seperate thread
	while( g_StyleBuffer == NULL );

	std::string outputBuffer = output;
	std::string styleBuffer;
	size_t outputLen = strlen( output );

	char outputColor = 'A';
	if ( messageType == kbOutputMessageType_t::Message_Error || messageType == kbOutputMessageType_t::Message_Assert ) {
		outputColor = 'B';
	}

	for ( int i = 0; i < outputLen; i++ ) {
		styleBuffer += outputColor;
	}

	g_OutputBuffer->append( outputBuffer.c_str() );
	g_StyleBuffer->append( styleBuffer.c_str() );

	if ( messageType == kbOutputMessageType_t::Message_Assert ) {
		fl_alert( output );
	}
}

/**
 *	kbEditor::RightClickPopUpMenu
 */
void kbEditor::RightClickPopUpMenu() {

	const kbPrefab *const prefab = g_Editor->m_pResourceTab->GetSelectedPrefab();
	std::string ReplacePrefabMessage = "Replace Prefab";
	std::string PlacePrefabMessage = "Place Prefab";
	if ( prefab == NULL ) {
		PlacePrefabMessage += "into scene";
	} else {
		PlacePrefabMessage += "[" + prefab->GetPrefabName() + "] into scene.";
		ReplacePrefabMessage += "[" + prefab->GetPrefabName() + "]";
	}


	Fl_Menu_Item rclick_menu[] = {
		{ "Create New Prefab",  0, AddEntityAsPrefab, ( void * ) 0 },
		{ ReplacePrefabMessage.c_str(), 0, ReplaceCurrentlySelectedPrefab, (void*) 1 },
		{ PlacePrefabMessage.c_str(),  0, InsertSelectedPrefabIntoScene, ( void * ) this },
		{ 0 }};

	if ( g_Editor->m_SelectedObjects.size() != 1 ) {
		rclick_menu[0].deactivate();
		rclick_menu[1].deactivate();
	}

	if ( prefab == NULL ) {
		rclick_menu[2].deactivate();
	}

	const Fl_Menu_Item * m = rclick_menu->popup( Fl::event_x(), Fl::event_y(), 0, 0, 0 );
	if ( m ) {
		m->do_callback( 0, m->user_data() );
	}
}

/**
 *	kbEditor::GetCurrentlySelectedPrefab
 */
const kbPrefab * kbEditor::GetCurrentlySelectedPrefab() const {
	return m_pResourceTab->GetSelectedPrefab();
}

/**
 *	kbEditor::ReplaceCurrentlySelectedPrefab
 */
void kbEditor::ReplaceCurrentlySelectedPrefab( class Fl_Widget *, void * ) {
	if ( g_Editor->m_SelectedObjects.size() != 1 ) {
		return;
	}

	kbPrefab * prefab = g_Editor->m_pResourceTab->GetSelectedPrefab();
	if ( prefab == NULL ) {
		return;
	}

	std::vector<kbGameEntity *> GameEntityList;
	for ( int i = 0; i < g_Editor->m_SelectedObjects.size(); i++ ) {
		GameEntityList.push_back( g_Editor->m_SelectedObjects[i]->GetGameEntity() );
	}

	g_ResourceManager.UpdatePrefab( prefab, GameEntityList ); 
	g_Editor->m_pResourceTab->MarkPrefabDirty( prefab );
//	g_ResourceManager.DumpPackageInfo();
	//g_ResourceManager.SavePackages();
}

/**
 *	kbEditor::AddEntityAsPrefab
 */
void kbEditor::AddEntityAsPrefab( Fl_Widget*, void * userdata ) {

   kbDialogBox dialogBox( "Add Prefab To Library Package", "Save Prefab", "Cancel" );
   dialogBox.AddTextField( "Package:" );
   dialogBox.AddTextField( "Folder:" );
   dialogBox.AddTextField( "Name:" );
   dialogBox.Run();

   kbLog( "0 : %s", dialogBox.GetFieldEntry( 0 ).c_str() );
   kbLog( "1 : %s", dialogBox.GetFieldEntry( 1 ).c_str() );
   kbLog( "2 : %s", dialogBox.GetFieldEntry( 2 ).c_str() );

	std::string PackageName = dialogBox.GetFieldEntry( 0 ).c_str();
	if ( GetFileExtension( PackageName ) != "kbPkg" ) {
		PackageName += ".kbPkg";
	}

	g_Editor->AddEntityAsPrefab_Internal( PackageName, dialogBox.GetFieldEntry( 1 ).c_str(),  dialogBox.GetFieldEntry( 2 ).c_str() );
 }

/**
 *	kbEditor::AddEntityAs_Prefab_Internal
 */
void kbEditor::AddEntityAsPrefab_Internal( const std::string & PackageName, const std::string & FolderName, const std::string & PrefabName ) {

	if ( m_SelectedObjects.size() != 1 ) {
		return;
	}

	if ( PackageName.empty() || FolderName.empty() || PrefabName.empty() ) {
		fl_alert( "Incomplete fields.  Prefab was not created" );
		return;
	}

	kbPrefab * prefab;
	if ( g_ResourceManager.AddPrefab( m_SelectedObjects[0]->GetGameEntity(), PackageName, FolderName, PrefabName, false, &prefab ) == false ) {
		int shouldOverwrite = fl_ask( "Prefab with that name and path already exist.  Overwrite?" );

		if ( shouldOverwrite == 0 )
			return;

		if ( g_ResourceManager.AddPrefab( m_SelectedObjects[0]->GetGameEntity(), PackageName, FolderName, PrefabName, true, &prefab ) == false ) {
			fl_alert( "Unable to add prefab" );
			return;
		}
	}

	m_pResourceTab->AddPrefab( prefab, PackageName, FolderName, PrefabName );
	//g_ResourceManager.DumpPackageInfo();
	//g_ResourceManager.SavePackages();

	fl_alert( "Prefab added successfully" );
 }

/**
 *	kbEditor::InsertSelectedPrefabIntoScene
 */
void kbEditor::InsertSelectedPrefabIntoScene( Fl_Widget*, void *userdata ) {
	const kbPrefab * prefabToCreate = g_Editor->m_pResourceTab->GetSelectedPrefab();

	if ( prefabToCreate == NULL ) {
		return;
	}

	kbEditor * editor = g_Editor;

	const kbCamera * editorCamera = editor->m_pMainTab->GetEditorWindowCamera();
	if ( editorCamera == NULL ) {
		return;
	}

	kbVec3 entityLocation = editorCamera->m_Position + ( editorCamera->m_Rotation.ToMat4()[2] * 4.0f ).ToVec3();

	for ( int i = 0; i < prefabToCreate->NumGameEntities(); i++ ) {
		kbGameEntity * pNewEntity = new kbGameEntity( prefabToCreate->m_GameEntities[i], false );
		kbEditorEntity * pEditorEntity = new kbEditorEntity( pNewEntity );
		pEditorEntity->SetPosition( entityLocation );
		editor->m_GameEntities.push_back( pEditorEntity );
	}
 }

