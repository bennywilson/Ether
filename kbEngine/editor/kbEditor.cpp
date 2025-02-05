/// kbEditor.cpp
///
/// 2016-2025 blk 1.0

#pragma warning(push)
#pragma warning(disable:4312)
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

#include <iomanip>
#include <sstream>
#include "kbCore.h"
#include "containers.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "render_defs.h"
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
#pragma warning(disable:4099)
#include "FL/fl_ask.h"
#pragma warning(pop)

kbEditor* g_Editor = nullptr;
kbDialogBox* kbDialogBox::gCurrentDialogBox = nullptr;
bool g_bEditorIsUndoingAnAction = false;

Fl_Text_Buffer* g_OutputBuffer = nullptr;
Fl_Text_Buffer* g_StyleBuffer = nullptr;
static Fl_Text_Display::Style_Table_Entry stable[] = { { FL_BLACK,	FL_HELVETICA, 12 },				// A
												{ FL_RED,		FL_HELVETICA, 12 } };		// B

// Editor camera speed
struct EditorCamSpeedBind {
	EditorCamSpeedBind(const kbString& displayName, const float multiplier) :
		m_DisplayName(displayName),
		m_SpeedMultiplier(multiplier) { }

	kbString m_DisplayName;
	float m_SpeedMultiplier;
};

const static EditorCamSpeedBind g_EditorCamSpeedBindings[] = {
	EditorCamSpeedBind(kbString("0.05x"), 0.05f),
	EditorCamSpeedBind(kbString("0.25x"), 0.25f),
	EditorCamSpeedBind(kbString("1x"), 1.0f),
	EditorCamSpeedBind(kbString("5x"), 5.0f),
	EditorCamSpeedBind(kbString("15x"), 15.0f)
};
const static size_t g_NumEditorCamSpeedBindings = sizeof(g_EditorCamSpeedBindings) / sizeof(EditorCamSpeedBind);


/// kbEditor
kbEditor::kbEditor() :
	Fl_Window(0, 0, GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN)) {

	m_bGameUpdating = false;
	const float editorInitStartTime = g_GlobalTimer.TimeElapsedSeconds();

	outputCB = kbEditor::OutputCB;
	m_UndoIDAtLastSave = UINT64_MAX;
	m_CurrentLevelFileName = "Untitled";

	g_Editor = this;

	m_pGame = nullptr;
	m_pGameWindow = nullptr;

	const int Screen_Width = GetSystemMetrics(SM_CXFULLSCREEN);
	const int Screen_Height = GetSystemMetrics(SM_CYFULLSCREEN);
	const int Menu_Bar_Height = 20;
	const int Menu_Buttons_Height = 30;
	const int Left_Panel = 200;
	const int Bottom_Panel_Height = 125;
	const int Panel_Border_size = 5;
	const int Right_Panel = 300;

	// Output display
	m_pOutputText = new Fl_Text_Display(5, Screen_Height - Bottom_Panel_Height + Panel_Border_size * 4, Screen_Width - 5, Bottom_Panel_Height - Panel_Border_size - Panel_Border_size, "DISPLAY!");
	g_OutputBuffer = new Fl_Text_Buffer();
	m_pOutputText->buffer(g_OutputBuffer);

	g_StyleBuffer = new Fl_Text_Buffer();

	int stable_size = sizeof(stable) / sizeof(stable[0]);

	m_pOutputText->highlight_data(g_StyleBuffer, stable, stable_size, 'A', 0, 0);

	// menu bar
	Fl_Menu_Bar* mainMenuBar = new Fl_Menu_Bar(0, 0, Screen_Width, Menu_Bar_Height);
	mainMenuBar->add("File/New Level", FL_CTRL + 'n', NewLevel);
	mainMenuBar->add("File/Open Level", FL_CTRL + 'o', OpenLevel);
	mainMenuBar->add("File/Save Level As", 0, SaveLevelAs);
	mainMenuBar->add("File/Save", FL_CTRL + 's', SaveLevel);
	mainMenuBar->add("Edit/Undo", FL_CTRL + 'z', Undo);
	mainMenuBar->add("Edit/Redo", FL_CTRL + 'y', Redo);

	mainMenuBar->add("Edit/Delete", FL_Delete, DeleteEntitiesCB);

	mainMenuBar->add("File/Quit", 0, Close, this);
	mainMenuBar->add("Edit/Change", FL_CTRL + 'c', nullptr);
	mainMenuBar->add("Edit/Submenu/Aaa");
	mainMenuBar->add("Edit/Submenu/Bbb");
	mainMenuBar->add("Add/Entity", 0, CreateGameEntity, this);

	std::map<std::string, const kbTypeInfoClass*>& componentMap = g_NameToTypeInfoMap->GetClassMap();
	std::map<std::string, const kbTypeInfoClass*>::iterator iter;

	for (iter = componentMap.begin(); iter != componentMap.end(); ++iter) {
		std::string MenuName = "Add/Component/";
		MenuName += iter->second->GetClassNameA().c_str();
		mainMenuBar->add(MenuName.c_str(), FL_CTRL + 'g', AddComponent, (void*)iter->second);	// Hack cast - unfortunate
	}

	mainMenuBar->add("Play/Play Game From Here", FL_CTRL + 'p', PlayGameFromHere);
	mainMenuBar->add("Play/Stop Game", FL_CTRL + 'q', StopGame);

	// buttons
	const int buttonSpacing = 5;
	const int TRSButtonWidth = 25;
	const int buttonHeight = Menu_Buttons_Height - 10;
	int curX = Left_Panel + buttonSpacing;
	int curY = Menu_Bar_Height + 5;

	Fl_Button* const translationButton = new Fl_Button(curX, curY, TRSButtonWidth, buttonHeight, "T");
	translationButton->callback(TranslationButtonCB);
	curX += TRSButtonWidth + buttonSpacing;

	Fl_Button* const rotationButton = new Fl_Button(curX, curY, TRSButtonWidth, buttonHeight, "R");
	rotationButton->callback(RotationButtonCB);
	curX += TRSButtonWidth + buttonSpacing;

	Fl_Button* const scaleButton = new Fl_Button(curX, curY, TRSButtonWidth, buttonHeight, "S");
	scaleButton->callback(ScaleButtonCB);
	curX += TRSButtonWidth * 3 + buttonSpacing;

	const int AdjustButtonWidth = 12 * (int)strlen("X+");
	Fl_Button* const xPlusAdjust = new Fl_Button(curX, curY, AdjustButtonWidth, buttonHeight, "X+");
	xPlusAdjust->callback(XPlusAdjustButtonCB);
	curX += AdjustButtonWidth + buttonSpacing;

	Fl_Button* const xNegAdjust = new Fl_Button(curX, curY, AdjustButtonWidth, buttonHeight, "X-");
	xNegAdjust->callback(XNegAdjustButtonCB);
	curX += AdjustButtonWidth + buttonSpacing;

	Fl_Button* const yPlusAdjust = new Fl_Button(curX, curY, AdjustButtonWidth, buttonHeight, "Y+");
	yPlusAdjust->callback(YPlusAdjustButtonCB);
	curX += AdjustButtonWidth + buttonSpacing;

	Fl_Button* const yNegAdjust = new Fl_Button(curX, curY, AdjustButtonWidth, buttonHeight, "Y-");
	yNegAdjust->callback(YNegAdjustButtonCB);
	curX += AdjustButtonWidth + buttonSpacing;

	Fl_Button* const zPlusAdjust = new Fl_Button(curX, curY, AdjustButtonWidth, buttonHeight, "Z+");
	zPlusAdjust->callback(ZPlusAdjustButtonCB);
	curX += AdjustButtonWidth + buttonSpacing;

	Fl_Button* const zNegAdjust = new Fl_Button(curX, curY, AdjustButtonWidth, buttonHeight, "Z-");
	zNegAdjust->callback(ZNegAdjustButtonCB);
	curX += AdjustButtonWidth + buttonSpacing;

	m_pXFormInput = new Fl_Input(curX, curY, AdjustButtonWidth, buttonHeight, "");
	m_pXFormInput->value("0");
	curX += TRSButtonWidth * 3 + buttonSpacing;

	const int speedButtonWidth = 85;
	curX += (int)fl_width("Cam Speed");
	m_pSpeedChoice = new Fl_Choice(curX, curY, (int)fl_width("x100000"), buttonHeight, "Cam Speed:");
	for (size_t i = 0; i < g_NumEditorCamSpeedBindings; i++) {
		m_pSpeedChoice->add(g_EditorCamSpeedBindings[i].m_DisplayName.c_str());
	}
	m_pSpeedChoice->callback(AdjustCameraSpeedCB);

	curX += speedButtonWidth + buttonSpacing * 2;

	const int toggleIconButtonWidth = 85 * 2;
	Fl_Button* const iconToggleButton = new Fl_Button(curX, curY, speedButtonWidth, buttonHeight, "Toggle Icons");
	iconToggleButton->callback(ToggleIconsCB);
	curX += toggleIconButtonWidth + buttonSpacing * 2;

	m_pViewModeChoice = new Fl_Choice(curX, curY, (int)fl_width("Wireframe") + TRSButtonWidth, buttonHeight);
	m_pViewModeChoice->add("Shaded");		// Note: These have to be in the same order as the entries in kbViewMode_t
	m_pViewModeChoice->add("Wireframe");
	m_pViewModeChoice->add("Color");
	m_pViewModeChoice->add("Normals");
	m_pViewModeChoice->add("Specular");
	m_pViewModeChoice->add("Depth");

	m_pViewModeChoice->value(0);
	m_pViewModeChoice->callback(ViewModeChoiceCB);

	// main tab
	m_pMainTab = new kbMainTab(Left_Panel + 5, Menu_Bar_Height + Menu_Buttons_Height, Screen_Width - Left_Panel - Right_Panel, Screen_Height - Menu_Bar_Height - Menu_Bar_Height - Bottom_Panel_Height);

	// resource tab
	m_pResourceTab = new kbResourceTab(0, Menu_Bar_Height + Menu_Buttons_Height, Left_Panel, Screen_Height - Menu_Bar_Height - Menu_Bar_Height - Bottom_Panel_Height);

	// properties tab
	m_pPropertiesTab = new kbPropertiesTab(Screen_Width - Right_Panel,
					Menu_Bar_Height + Menu_Buttons_Height,
					Right_Panel,
					Screen_Height - Menu_Bar_Height - Menu_Bar_Height - Bottom_Panel_Height + 5);

	end();
	show();

	//buff->text( "asdlkjasldkjalskdjlaskjd" );
	//Fl::run();

	// setup the renderer
	if (g_pRenderer == nullptr) {
		g_pRenderer = new kbRenderer_DX11();
		g_pRenderer->Init(m_pMainTab->GetEditorWindow()->GetWindowHandle(), 1920, 1080);
		g_pRenderer->EnableDebugBillboards(true);
	}

	m_pResourceTab->PostRendererInit();

	m_bIsRunning = true;

	m_Timer.Reset();

	// reserve textures
	g_pRenderer->LoadTexture("../../kbEngine/assets/Textures/Editor/EntityIcon.jpg", 1);
	g_pRenderer->LoadTexture("../../kbEngine/assets/Textures/Editor/directionalLightIcon.jpg", 2);

	SetWindowText(m_pMainTab->GetEditorWindow()->GetWindowHandle(), "kbEditor");

	// Load Editor Settings
	kbEditorGlobalSettingsComponent* pEditorGlobalComponent = nullptr;

	kbFile levelEditorFile;
	if (levelEditorFile.Open("./assets/editorSettings.txt", kbFile::FT_Read)) {
		kbGameEntity* const gameEntity = levelEditorFile.ReadGameEntity();
		pEditorGlobalComponent = (kbEditorGlobalSettingsComponent*)gameEntity->GetComponentByType(kbEditorGlobalSettingsComponent::GetType());
		levelEditorFile.Close();
	}

	if (pEditorGlobalComponent == nullptr) {
		m_pSpeedChoice->value(0);
		m_pMainTab->SetCameraSpeedMultiplier(g_EditorCamSpeedBindings[0].m_SpeedMultiplier);
	} else {

		if (pEditorGlobalComponent->m_CameraSpeedIdx >= 0 && pEditorGlobalComponent->m_CameraSpeedIdx < g_NumEditorCamSpeedBindings) {
			const int idx = pEditorGlobalComponent->m_CameraSpeedIdx;
			m_pSpeedChoice->value(idx);
			m_pMainTab->SetCameraSpeedMultiplier(g_EditorCamSpeedBindings[idx].m_SpeedMultiplier);
		} else {
			m_pSpeedChoice->value(0);
			m_pMainTab->SetCameraSpeedMultiplier(g_EditorCamSpeedBindings[0].m_SpeedMultiplier);
		}
	}

	blk::log("Editor init time took %f seconds", g_GlobalTimer.TimeElapsedSeconds() - editorInitStartTime);
}

/// ~kbEditor
kbEditor::~kbEditor() {
	ShutDown();
}

/// kbEditor::UnloadMap
void kbEditor::UnloadMap() {
	if (g_pRenderer != nullptr) {
		g_pRenderer->WaitForRenderingToComplete();
	}

	// Remove old entities
	DeselectEntities();

	for (int i = 0; i < g_Editor->m_GameEntities.size(); i++) {
		delete m_GameEntities[i];
	}
	m_GameEntities.clear();

	m_CurrentLevelFileName = "Untitled";
	m_UndoIDAtLastSave = UINT64_MAX;
	m_UndoStack.Reset();

	m_pResourceTab->RefreshEntitiesTab();
}

/// kbEditor::LoadMap
void kbEditor::LoadMap(const std::string& InMapName) {
	blk::log("LoadMap() called for map %s", InMapName.c_str());
	const float loadMapStartTime = g_GlobalTimer.TimeElapsedSeconds();

	UnloadMap();

	const kbEditorLevelSettingsComponent* pLevelSettings = nullptr;

	// Load map
	if (InMapName.empty() == false) {
		m_CurrentLevelFileName = InMapName;

		TCHAR NPath[MAX_PATH];

		GetCurrentDirectory(MAX_PATH, NPath);

		WIN32_FIND_DATA fdFile;
		HANDLE hFind = nullptr;

		std::string LevelPath = NPath;
		LevelPath += "/Assets/Levels/";
		std::string curLevelFolder = "";

		if (m_CurrentLevelFileName.find(".") == std::string::npos) {
			m_CurrentLevelFileName += ".kbLevel";
		}

		hFind = FindFirstFile((LevelPath + "*").c_str(), &fdFile);
		BOOL nextFileFound = (hFind != INVALID_HANDLE_VALUE);
		do {
			std::string nextFileName = LevelPath + curLevelFolder + m_CurrentLevelFileName;

			kbFile inFile;
			if (inFile.Open(nextFileName.c_str(), kbFile::FT_Read)) {
				m_CurrentLevelFileName = nextFileName;

				kbGameEntity* pGameEntity = inFile.ReadGameEntity();

				while (pGameEntity != nullptr) {

					if (pLevelSettings == nullptr) {
						pLevelSettings = (kbEditorLevelSettingsComponent*)pGameEntity->GetComponentByType(kbEditorLevelSettingsComponent::GetType());
						if (pLevelSettings != nullptr) {
							pGameEntity = inFile.ReadGameEntity();
							continue;
						}
					}

					kbEditorEntity* const newEditorEntity = new kbEditorEntity(pGameEntity);
					g_Editor->m_GameEntities.push_back(newEditorEntity);
					pGameEntity = inFile.ReadGameEntity();
				}
				inFile.Close();

				const std::string windowText = "kbEditor - " + InMapName;
				SetWindowText(fl_xid(this), windowText.c_str());

				break;
			}

			if (nextFileFound == FALSE) {
				break;
			}

			do {
				if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0) {
					curLevelFolder = "/";
					curLevelFolder += fdFile.cFileName;
					curLevelFolder += "/";
					break;
				}

			} while (nextFileFound = FindNextFile(hFind, &fdFile) != FALSE);

			if (nextFileFound != 0) {
				nextFileFound = FindNextFile(hFind, &fdFile);
			}
		} while (true);
	}

	if (pLevelSettings != nullptr) {
		SetMainCameraPos(pLevelSettings->m_CameraPosition);
		SetMainCameraRot(pLevelSettings->m_CameraRotation);
	} else {
		SetMainCameraPos(kbVec3::zero);
		SetMainCameraRot(kbQuat::identity);
	}

	m_UndoStack.Reset();


	std::sort(g_Editor->m_GameEntities.begin(), g_Editor->m_GameEntities.end(),
			  [](const kbEditorEntity* a, const kbEditorEntity* b) -> bool {

				  if (a->GetGameEntity()->GetComponentByType(kbLevelComponent::GetType())) {
					  return true;
				  } else if (b->GetGameEntity()->GetComponentByType(kbLevelComponent::GetType())) {
					  return false;
				  }

				  return a->GetGameEntity()->GetName().stl_str().compare(b->GetGameEntity()->GetName().stl_str()) < 0;
	});


	m_pResourceTab->RefreshEntitiesTab();

	blk::log("	LoadMap finished.  Took %f seconds", g_GlobalTimer.TimeElapsedSeconds() - loadMapStartTime);
}

/// kbEditor::Update
void kbEditor::Update() {
	if (m_bIsRunning == false) {
		return;
	}

	if (m_bGameUpdating && GetAsyncKeyState(VK_BACK)) {
		StopGame(nullptr, nullptr);
	}

	for (int i = 0; i < m_UpdateWidgets.size(); i++) {
		m_UpdateWidgets[i]->Update();
	}

	// Wait for rendering to complete, sync up any game objects that need it and kick off a new scene to render
	if (g_pRenderer != nullptr) {
		g_pRenderer->WaitForRenderingToComplete();
	}

	static int NumFrames = 0;
	static float StartTime = g_GlobalTimer.TimeElapsedSeconds();

	NumFrames++;
	static float FPS = 0;

	if (NumFrames > 100) {
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		FPS = (float)NumFrames / (curTime - StartTime);
		NumFrames = 0;
		StartTime = curTime;
	}

	{//if ( g_ShowFPS.GetBool() ) {
		std::string fpsString = "FPS: ";
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << FPS;
		fpsString += stream.str();
		g_pRenderer->DrawDebugText(fpsString, 0.85f, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green);
	}

	// Update editor entities and components
	for (int i = 0; i < m_GameEntities.size(); i++) {
		m_GameEntities[i]->RenderSync();
	}

	for (int i = 0; i < m_UpdateWidgets.size(); i++) {
		m_UpdateWidgets[i]->RenderSync();
	}

	// Remove any undeleted actors
	if (m_RemovedEntities.size() > 0) {
		std::vector<kbUndoDeleteActor::DeletedActorInfo_t> deletedEntities;
		for (int i = 0; i < m_RemovedEntities.size(); i++) {
			blk::std_remove_swap(m_GameEntities, m_RemovedEntities[i]);

			kbUndoDeleteActor::DeletedActorInfo_t deletedActor;
			deletedActor.m_pEditorEntity = m_RemovedEntities[i];

			for (int j = 0; j < m_RemovedEntities[i]->GetGameEntity()->NumComponents(); j++) {
				deletedActor.m_bComponentEnabled.push_back(m_RemovedEntities[i]->GetGameEntity()->GetComponent(j)->IsEnabled());
				m_RemovedEntities[i]->GetGameEntity()->GetComponent(j)->Enable(false);
				m_RemovedEntities[i]->RenderSync();
			}

			deletedEntities.push_back(deletedActor);
		}

		g_Editor->GetSelectedObjects().clear();
		g_Editor->m_UndoStack.Push(new kbUndoDeleteActor(deletedEntities));

		g_Editor->BroadcastEvent(widgetCBEntityDeselected());
		m_RemovedEntities.clear();
	}

	g_pRenderer->RenderSync();

	g_ResourceManager.RenderSync();

	g_pGame->GetParticleManager().RenderSync();

	g_pRenderer->SetReadyToRender();

	//m_pMainTab->GetCurrentWindow()->GetCamera().Update();

	if (GetFocus() == fl_xid(this)) {

		/*if ( GetAsyncKeyState( 'C' ) ) {

			kbEditorEntity* pMasterBridge = nullptr;
			kbStaticModelComponent* pMasterComp = nullptr;
			static kbString skMasterBridge( "Bridge - Master" );

			for ( int i = 0; i < m_GameEntities.size(); i++ ) {
				kbEditorEntity* const pEditorEntity = m_GameEntities[i];
				if ( pEditorEntity == nullptr ) {
					continue;
				}

				kbGameEntity* const pGameEnt = pEditorEntity->GetGameEntity();
				if ( pGameEnt == nullptr ) {
					continue;
				}

				if ( pGameEnt->GetName() == skMasterBridge ) {
					pMasterBridge = m_GameEntities[i];
					pMasterComp = pMasterBridge->GetGameEntity()->GetComponent<kbStaticModelComponent>();
					break;
				}
			}

			if ( pMasterBridge != nullptr ) {
				for ( int i = 0; i < m_GameEntities.size(); i++ ) {

					kbEditorEntity* const pEditorEntity = m_GameEntities[i];
					if ( pEditorEntity == nullptr ) {
						continue;
					}

					kbGameEntity* const pGameEnt = pEditorEntity->GetGameEntity();
					if ( pGameEnt == nullptr ) {
						continue;
					}
					if ( pGameEnt->GetName().stl_str().find( "Bridge" ) != std::string::npos ) {
						kbStaticModelComponent* pTargetComp = pGameEnt->GetComponent<kbStaticModelComponent>();
						if ( pTargetComp != nullptr ) {
							pTargetComp->CopyMaterialList( pMasterComp->GetMaterialList() );
							continue;
						}
					}
				}
			}
		}*/
		// input
		if (GetAsyncKeyState('W')) {
			m_WidgetInputObject.keys.push_back(widgetCBInputObject::keyType_t::WidgetInput_Forward);
		} else if (GetAsyncKeyState('S')) {
			m_WidgetInputObject.keys.push_back(widgetCBInputObject::keyType_t::WidgetInput_Back);
		}

		if (GetAsyncKeyState('A')) {
			m_WidgetInputObject.keys.push_back(widgetCBInputObject::keyType_t::WidgetInput_Left);
		} else if (GetAsyncKeyState('D')) {
			m_WidgetInputObject.keys.push_back(widgetCBInputObject::keyType_t::WidgetInput_Right);
		}

		if (GetAsyncKeyState(VK_LCONTROL)) {
			m_WidgetInputObject.keys.push_back(widgetCBInputObject::keyType_t::WidgetInput_Ctrl);
		}

		if (GetAsyncKeyState(VK_LSHIFT)) {
			m_WidgetInputObject.keys.push_back(widgetCBInputObject::keyType_t::WidgetInput_Shift);
		}

		if (m_WidgetInputObject.keys.size() > 0 || m_WidgetInputObject.mouseDeltaX != 0 || m_WidgetInputObject.mouseDeltaY != 0 ||
			m_WidgetInputObject.leftMouseButtonDown || m_WidgetInputObject.rightMouseButtonDown) {
			BroadcastEvent(m_WidgetInputObject);
		}

		if (m_WidgetInputObject.leftMouseButtonPressed) {
			m_WidgetInputObject.leftMouseButtonPressed = false;
			m_WidgetInputObject.leftMouseButtonDown = true;
		}

		if (m_WidgetInputObject.rightMouseButtonPressed) {
			m_WidgetInputObject.rightMouseButtonPressed = false;
			m_WidgetInputObject.rightMouseButtonDown = true;
		}
	} else {
		m_WidgetInputObject.leftMouseButtonPressed = false;
		m_WidgetInputObject.rightMouseButtonPressed = false;
		m_WidgetInputObject.leftMouseButtonDown = false;
		m_WidgetInputObject.rightMouseButtonDown = false;
	}

	m_WidgetInputObject.ClearKeys();
	m_WidgetInputObject.mouseDeltaX = 0;
	m_WidgetInputObject.mouseDeltaY = 0;
	m_WidgetInputObject.leftMouseButtonPressed = false;
	m_WidgetInputObject.rightMouseButtonPressed = false;

	Fl::flush();

	float DT = m_Timer.TimeElapsedSeconds();
	m_Timer.Reset();

	if (DT > 0.05f) {
		DT = 0.05f;
	}
	// Update editor entities and components
	for (int i = 0; i < m_GameEntities.size(); i++) {
		m_GameEntities[i]->Update(DT);
	}

	if (m_pGame != nullptr && m_bGameUpdating) {
		m_pGame->HackEditorUpdate(DT, m_pMainTab->GetEditorWindowCamera());
	}

	// Update title bar dirty status
	if (m_UndoIDAtLastSave != m_UndoStack.GetLastDirtyActionId()) {
		SetWindowText(fl_xid(this), ("kbEditor - " + m_CurrentLevelFileName + "*").c_str());
	} else {
		SetWindowText(fl_xid(this), ("kbEditor - " + m_CurrentLevelFileName).c_str());
	}
}

// kbEditor::ShutDown
void kbEditor::ShutDown() {
	// Save Editor Settings
	kbFile outFile;
	outFile.Open("./assets/editorSettings.txt", kbFile::FT_Write);

	kbGameEntity levelInfoEnt;
	kbEditorGlobalSettingsComponent* const pLevelInfo = new kbEditorGlobalSettingsComponent();
	pLevelInfo->m_CameraSpeedIdx = m_pSpeedChoice->value();
	levelInfoEnt.AddComponent(pLevelInfo);
	outFile.WriteGameEntity(&levelInfoEnt);
	outFile.Close();

	if (m_bIsRunning == false) {
		return;
	}

	for (int i = 0; i < m_GameEntities.size(); i++) {
		delete m_GameEntities[i];
	}
	m_GameEntities.clear();

	g_ResourceManager.Shutdown();

	m_bIsRunning = false;
}

/// kbEditor::ShutDown
void kbEditor::BroadcastEvent(const widgetCBObject& cbObject) {

	std::vector< kbWidget* >& receivers = m_EventReceivers[cbObject.widgetType];

	for (int i = 0; i < receivers.size(); i++) {
		receivers[i]->EventCB(&cbObject);
	}
}

/// kbEditor::SetMainCameraPos
void kbEditor::SetMainCameraPos(const kbVec3& newCamPos) {
	m_pMainTab->GetEditorWindowCamera()->m_Position = newCamPos;
}

/// kbEditor::GetMainCameraPos
kbVec3 kbEditor::GetMainCameraPos() const {
	return m_pMainTab->GetEditorWindowCamera()->m_Position;
}

/// kbEditor::SetMainCameraRot
void kbEditor::SetMainCameraRot(const kbQuat& newCamRot) {
	m_pMainTab->GetEditorWindowCamera()->m_Rotation = newCamRot;
	m_pMainTab->GetEditorWindowCamera()->m_RotationTarget = newCamRot;
}

/// kbEditor::GetMainCameraRot
kbQuat kbEditor::GetMainCameraRot() const {
	return m_pMainTab->GetEditorWindowCamera()->m_Rotation;
}

/// kbEditor::DeselectEntities
void kbEditor::DeselectEntities() {

	for (int i = 0; i < m_GameEntities.size(); i++) {
		m_GameEntities[i]->SetIsSelected(false);
	}

	m_SelectedObjects.clear();
	g_Editor->BroadcastEvent(widgetCBEntityDeselected());
}

/// kbEditor::AddEntity
void kbEditor::AddEntity(kbEditorEntity* const pEditorEntity) {
	blk::error_check(blk::std_contains(m_GameEntities, pEditorEntity) == false, "kbEditor::AddEntity() - Called on an entity that has already been added.");

	m_GameEntities.push_back(pEditorEntity);
}

/// kbEditor::SelectEntities
void kbEditor::SelectEntities(std::vector< kbEditorEntity* >& entitiesToSelect, const bool bAppendToSelectedEntities) {
	if (g_bEditorIsUndoingAnAction == false) {
		m_UndoStack.Push(new kbUndoSelectActor(m_SelectedObjects, entitiesToSelect));
	}

	if (bAppendToSelectedEntities == false) {
		DeselectEntities();
	}

	for (int i = 0; i < entitiesToSelect.size(); i++) {
		entitiesToSelect[i]->SetIsSelected(true);
	}

	m_SelectedObjects.insert(m_SelectedObjects.end(), entitiesToSelect.begin(), entitiesToSelect.end());

	widgetCBEntitySelected entitySelectedCB;
	entitySelectedCB.entitiesSelected = entitiesToSelect;

	g_Editor->BroadcastEvent(entitySelectedCB);
}

/// kbEditor::handle
int kbEditor::handle(int theEvent) {
	const int button = Fl::event_button();
	const int state = Fl::event_state();

	int newMouseX = Fl::event_x();
	int newMouseY = Fl::event_y();

	// check for right mouse button
	if (theEvent == FL_PUSH) {
		m_bRightMouseButtonDragged = false;

		// don't allow both buttons to be down
		/*if ( ( button == 3 && m_WidgetInputObject.leftMouseButtonDown ) ||
			( button == 1 && m_WidgetInputObject.rightMouseButtonDown ) ) {
				return 1;
		}*/

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;

		if (button == 3) {
			//	m_WidgetInputObject.rightMouseButtonDown = true;
			m_WidgetInputObject.rightMouseButtonPressed = true;
		} else if (button == 1) {
			//m_WidgetInputObject.leftMouseButtonDown = true;
			m_WidgetInputObject.leftMouseButtonPressed = true;
		}

		if (m_WidgetInputObject.rightMouseButtonPressed && m_bRightMouseButtonDragged == false) {
			if (newMouseX >= m_pOutputText->x() && newMouseY >= m_pOutputText->y() && newMouseX < m_pOutputText->x() + m_pOutputText->w() && newMouseY < m_pOutputText->y() + m_pOutputText->h()) {
				RightClickOnOutputWindow();
			}
		}

		Fl_Window::handle(theEvent);
		return 1;
	} else if (theEvent == FL_RELEASE) {

		if (m_WidgetInputObject.rightMouseButtonDown && m_bRightMouseButtonDragged == false) {
			if (newMouseX >= m_pMainTab->x() && newMouseY >= m_pMainTab->y() && newMouseX < m_pMainTab->x() + m_pMainTab->w() && newMouseY < m_pMainTab->y() + m_pMainTab->h()) {
				RightClickOnMainTab();
			}
		}

		m_WidgetInputObject.leftMouseButtonDown = false;
		m_WidgetInputObject.leftMouseButtonPressed = false;
		m_WidgetInputObject.rightMouseButtonDown = false;
		m_WidgetInputObject.rightMouseButtonPressed = false;
		Fl_Window::handle(theEvent);
		return 1;
	} else if (m_WidgetInputObject.leftMouseButtonDown && theEvent == FL_DRAG) {
		m_WidgetInputObject.mouseDeltaX = newMouseX - m_WidgetInputObject.mouseX;
		m_WidgetInputObject.mouseDeltaY = newMouseY - m_WidgetInputObject.mouseY;

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;
	} else if (m_WidgetInputObject.rightMouseButtonDown && theEvent == FL_DRAG) {
		m_bRightMouseButtonDragged = true;
		m_WidgetInputObject.mouseDeltaX = newMouseX - m_WidgetInputObject.mouseX;
		m_WidgetInputObject.mouseDeltaY = newMouseY - m_WidgetInputObject.mouseY;

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;
		HWND hWnd = fl_xid(this);
		RECT rc;
		GetClientRect(hWnd, &rc);

		const int leftBorder = rc.left + 10;
		const int rightBorder = rc.right - 10;
		const int topBorder = rc.top + 10;
		const int bottomBorder = rc.bottom - 10;

		bool updateCursor = false;

		if (newMouseX < leftBorder) {
			updateCursor = true;
			newMouseX = rightBorder;
		} else if (newMouseX > rightBorder) {
			updateCursor = true;
			newMouseX = leftBorder;
		}

		if (newMouseY < topBorder) {
			updateCursor = true;
			newMouseY = bottomBorder;
		} else if (newMouseY > bottomBorder) {
			updateCursor = true;
			newMouseY = topBorder;
		}

		if (updateCursor) {
			POINT point;
			point.x = (LONG)newMouseX;
			point.y = (LONG)newMouseY;

			ClientToScreen(hWnd, &point);
			SetCursorPos(point.x, point.y);
		}

		m_WidgetInputObject.mouseX = newMouseX;
		m_WidgetInputObject.mouseY = newMouseY;
	} else if (m_WidgetInputObject.rightMouseButtonDown) {

	}

	return Fl_Window::handle(theEvent);
}


/// kbEditor::Close
void kbEditor::Close(Fl_Widget* widget, void* thisPtr) {
	kbEditor* editor = static_cast<kbEditor*>(thisPtr);

	editor->ShutDown();
}

/// kbEditor::CreateGameEntity
void kbEditor::CreateGameEntity(Fl_Widget* widget, void* thisPtr) {

	const kbCamera* const editorCamera = g_Editor->m_pMainTab->GetEditorWindowCamera();

	if (editorCamera == nullptr) {
		return;
	}

	kbEditorEntity* const pEditorEntity = new kbEditorEntity();
	const kbVec3 entityLocation = editorCamera->m_Position + (editorCamera->m_Rotation.ToMat4()[2] * 4.0f).ToVec3();
	pEditorEntity->SetPosition(entityLocation);

	g_Editor->m_GameEntities.push_back(pEditorEntity);

	g_Editor->m_pResourceTab->RefreshEntitiesTab();
}

/// kbEditor::AddComponent
void kbEditor::AddComponent(Fl_Widget* widget, void* voidPtr) {
	if (voidPtr == nullptr || g_Editor == nullptr)
		return;

	kbTypeInfoClass* const typeInfoClass = static_cast<kbTypeInfoClass*>(voidPtr);
	std::vector<kbEditorEntity*>& selectedObjects = g_Editor->GetSelectedObjects();

	if (selectedObjects.size() > 0) {
		kbGameComponent* const newComponent = (kbGameComponent*)typeInfoClass->ConstructInstance();		// ENTITY HACK

		const_cast<kbGameEntity*>(selectedObjects[0]->GetGameEntity())->AddComponent(newComponent);
		newComponent->Enable(true);

		widgetCBObject widgetCB;
		widgetCB.widgetType = WidgetCB_ComponentCreated;
		g_Editor->BroadcastEvent(widgetCB);
	}
}

/// kbEditor::TranslationButtonCB
void kbEditor::TranslationButtonCB(class Fl_Widget*, void*) {
	widgetCBObject cbObject;
	cbObject.widgetType = WidgetCB_TranslationButtonPressed;
	g_Editor->BroadcastEvent(cbObject);
}

/// kbEditor::RotationButtonCB
void kbEditor::RotationButtonCB(class Fl_Widget*, void*) {
	widgetCBObject cbObject;
	cbObject.widgetType = WidgetCB_RotationButtonPressed;
	g_Editor->BroadcastEvent(cbObject);
}

/// kbEditor::ScaleButtonCB
void kbEditor::ScaleButtonCB(class Fl_Widget*, void*) {
	widgetCBObject cbObject;
	cbObject.widgetType = WidgetCB_ScaleButtonPressed;
	g_Editor->BroadcastEvent(cbObject);
}

/// XFormEntities
void XFormEntities(const kbManipulator& manipulator, const kbVec4 xForm) {
	std::vector<kbEditorEntity*>& entityList = g_Editor->GetGameEntities();
	for (int i = 0; i < entityList.size(); i++) {
		if (entityList[i]->IsSelected()) {

			if (manipulator.GetMode() == kbManipulator::Translate) {
				entityList[i]->SetPosition(entityList[i]->GetPosition() + xForm.ToVec3() * xForm.w);
			} else if (manipulator.GetMode() == kbManipulator::Rotate) {
				kbQuat rot(xForm.ToVec3(), xForm.a);
				rot = (entityList[i]->GetOrientation() * rot).Normalized();
				entityList[i]->SetOrientation(rot);
			} else if (manipulator.GetMode() == kbManipulator::Scale) {
				entityList[i]->SetScale(entityList[i]->GetScale() + xForm.ToVec3() * xForm.w);
			}
		}
	}
}

/// kbEditor::XPlusAdjustButtonCB
void kbEditor::XPlusAdjustButtonCB(Fl_Widget*, void*) {

	XFormEntities(g_Editor->m_pMainTab->m_Manipulator, kbVec4(1.0f, 0.0f, 0.0f, (float)atof(g_Editor->m_pXFormInput->value())));
}

/// kbEditor::XNegAdjustButtonCB
void kbEditor::XNegAdjustButtonCB(Fl_Widget*, void*) {

	XFormEntities(g_Editor->m_pMainTab->m_Manipulator, kbVec4(-1.0f, 0.0f, 0.0f, (float)atof(g_Editor->m_pXFormInput->value())));
}

/// kbEditor::YPlusAdjustButtonCB
void kbEditor::YPlusAdjustButtonCB(Fl_Widget*, void*) {

	XFormEntities(g_Editor->m_pMainTab->m_Manipulator, kbVec4(0.0f, 1.0f, 0.0f, (float)atof(g_Editor->m_pXFormInput->value())));
}

/// kbEditor::YNegAdjustButtonCB
void kbEditor::YNegAdjustButtonCB(Fl_Widget*, void*) {

	XFormEntities(g_Editor->m_pMainTab->m_Manipulator, kbVec4(0.0f, -1.0f, 0.0f, (float)atof(g_Editor->m_pXFormInput->value())));
}

/// kbEditor::ZPlusAdjustButtonCB
void kbEditor::ZPlusAdjustButtonCB(Fl_Widget*, void*) {

	XFormEntities(g_Editor->m_pMainTab->m_Manipulator, kbVec4(0.0f, 0.0f, 1.0f, (float)atof(g_Editor->m_pXFormInput->value())));
}

/// kbEditor::ZNegAdjustButtonCB
void kbEditor::ZNegAdjustButtonCB(Fl_Widget*, void*) {

	XFormEntities(g_Editor->m_pMainTab->m_Manipulator, kbVec4(0.0f, 0.0f, -1.0f, (float)atof(g_Editor->m_pXFormInput->value())));
}

/// kbEditor::AdjustCameraSpeedCB
void kbEditor::AdjustCameraSpeedCB(class Fl_Widget* widget, void*) {
	const Fl_Choice* const pChoiceWidget = g_Editor->m_pSpeedChoice;
	const int selectionIdx = pChoiceWidget->value();
	if (selectionIdx < 0 || selectionIdx >= g_NumEditorCamSpeedBindings) {
		blk::warning("kbEditor::AdjustCameraSpeedCB() - Invalid choice selected.");
		return;
	}

	const float multiplier = g_EditorCamSpeedBindings[selectionIdx].m_SpeedMultiplier;
	g_Editor->m_pMainTab->SetCameraSpeedMultiplier(multiplier);
}

/// kbEditor::ToggleIconsCB
bool g_bBillboardsEnabled = true;
void kbEditor::ToggleIconsCB(Fl_Widget* widget, void* userData) {

	g_bBillboardsEnabled = !g_bBillboardsEnabled;
	g_pRenderer->EnableDebugBillboards(g_bBillboardsEnabled);
}

/// kbEditor::NewLevel
void kbEditor::NewLevel(Fl_Widget*, void*) {
	const int areYouSure = fl_ask("Creating a new level.  Any unsaved changes will be lost.  Are you sure?");
	if (areYouSure == 0) {
		return;
	}

	g_Editor->UnloadMap();

	g_Editor->m_GameEntities.clear();
	g_Editor->DeselectEntities();
}

/// kbEditor::OpenLevel
void kbEditor::OpenLevel(class Fl_Widget*, void*) {
	Fl_File_Chooser fileChooser(".", "*.kbLevel", Fl_File_Chooser::SINGLE, "Open Level");

	std::string currentDir = fileChooser.directory();
	currentDir += "/assets/levels";
	fileChooser.directory(currentDir.c_str());

	fileChooser.show();

	while (fileChooser.shown()) { Fl::wait(); }

	const char* const fileName = fileChooser.value();
	if (fileName == nullptr) {
		return;
	}

	const int areYouSure = fl_ask("You have unsaved changes.  Are you sure you want to open a new level?");
	if (areYouSure == false) {
		return;
	}

	g_pRenderer->WaitForRenderingToComplete();

	// Remove old entities
	g_Editor->DeselectEntities();

	for (int i = 0; i < g_Editor->m_GameEntities.size(); i++) {
		delete g_Editor->m_GameEntities[i];
	}
	g_Editor->m_GameEntities.clear();


	std::string fileNameStr = fileName;
	const size_t pos = fileNameStr.find_last_of("\\/");
	if (pos != std::string::npos) {
		fileNameStr = fileNameStr.substr(pos + 1, fileNameStr.length() - pos);
	}
	g_Editor->LoadMap(fileNameStr.c_str());
}

/// kbEditor::SaveLevel_Internal
void kbEditor::SaveLevel_Internal(const std::string& fileNameStr, const bool bForceSave) {
	if (bForceSave == false) {
		std::ifstream f(fileNameStr.c_str());
		if (f.good()) {
			const int overWriteIt = fl_ask("File already exists.  Do you wish to overwrite it?");
			if (overWriteIt == 0) {
				f.close();
				return;
			}
		}
		f.close();
	}

	kbFile outFile;
	outFile.Open(fileNameStr.c_str(), kbFile::FT_Write);

	{
		const kbCamera* const pCam = m_pMainTab->GetEditorWindowCamera();

		kbEditorLevelSettingsComponent* const pLevelSettingsComp = new kbEditorLevelSettingsComponent();
		pLevelSettingsComp->m_CameraPosition = pCam->m_Position;
		pLevelSettingsComp->m_CameraRotation = pCam->m_Rotation;

		kbGameEntity* const pLevelSettingsEnt = new kbGameEntity();
		pLevelSettingsEnt->AddComponent(pLevelSettingsComp);

		outFile.WriteGameEntity(pLevelSettingsEnt);
		delete pLevelSettingsEnt;
	}

	for (int i = 0; i < g_Editor->m_GameEntities.size(); i++) {
		outFile.WriteGameEntity(g_Editor->m_GameEntities[i]->GetGameEntity());
	}

	outFile.Close();

	m_UndoIDAtLastSave = m_UndoStack.GetLastDirtyActionId();
}

/// kbEditor::SaveLevelAs
void kbEditor::SaveLevelAs(class Fl_Widget*, void*) {

	Fl_File_Chooser fileChooser("./assets/levels", "*.kbLevel", Fl_File_Chooser::CREATE, "Save Level");
	std::string currentDir = fileChooser.directory();
	currentDir += "/assets/levels";

	fileChooser.show();

	while (fileChooser.shown()) { Fl::wait(); }

	std::string fileName = fileChooser.value();
	if (fileName.empty()) {
		return;
	}

	const std::string fileExt = GetFileExtension(fileName);
	if (fileExt != "kbLevel" && fileExt != "kblevel") {
		fileName += ".kblevel";
	}
	g_Editor->SaveLevel_Internal(fileName, false);
}

/// kbEditor::SaveLevel
void kbEditor::SaveLevel(class Fl_Widget*, void*) {

	if (g_Editor->m_CurrentLevelFileName.empty()) {
		return;
	}

	g_Editor->SaveLevel_Internal(g_Editor->m_CurrentLevelFileName, true);
}

/// kbEditor::Undo
void kbEditor::Undo(class Fl_Widget*, void*) {
	g_Editor->m_UndoStack.Undo();
	g_Editor->m_pPropertiesTab->RequestRefreshNextUpdate();
	g_Editor->m_pResourceTab->RefreshEntitiesTab();
}

/// kbEditor::Redo
void kbEditor::Redo(class Fl_Widget*, void*) {
	g_Editor->m_UndoStack.Redo();
	g_Editor->m_pPropertiesTab->RequestRefreshNextUpdate();
	g_Editor->m_pResourceTab->RefreshEntitiesTab();
}

/// kbEditor::PlayGameFromHere
void kbEditor::PlayGameFromHere(class Fl_Widget*, void*) {
	if (g_Editor == nullptr || g_Editor->m_pGame == nullptr || g_Editor->m_pGame->IsPlaying()) {
		return;
	}

	g_Editor->m_bGameUpdating = true;
	g_pGame->HackEditorInit(g_Editor->m_pMainTab->GetEditorWindow()->GetWindowHandle(), g_Editor->m_GameEntities);
	/*std::vector< const kbGameEntity * > GameEntitiesList;

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
	Fl::check();*/
}

/// kbEditor::StopGame
void kbEditor::StopGame(class Fl_Widget*, void*) {
	if (g_Editor == nullptr || g_Editor->m_pGame == nullptr) {
		return;
	}

	g_Editor->m_bGameUpdating = false;
	ShowCursor(true);

	g_pGame->HackEditorShutdown();
	/*
	g_Editor->m_pGame->StopGame();

	delete g_Editor->m_pGameWindow;
	g_Editor->m_pGameWindow = nullptr;

	g_pRenderer->SetRenderWindow( nullptr );

	widgetCBObject widgetCB;
	widgetCB.widgetType = WidgetCB_GameStopped;
	g_Editor->BroadcastEvent( widgetCB );*/
}

/// kbEditor::DeleteEntities
void kbEditor::DeleteEntities(std::vector<kbEditorEntity*>& editorEntityList) {
	std::vector<kbUndoDeleteActor::DeletedActorInfo_t> deletedEntities;

	for (int i = 0; i < editorEntityList.size(); i++) {
		g_Editor->m_RemovedEntities.push_back(editorEntityList[i]);
		blk::std_remove_swap(m_GameEntities, editorEntityList[i]);
	}

}

/// kbEditor::DeleteEntitiesCB
void kbEditor::DeleteEntitiesCB(class Fl_Widget*, void*) {
	std::vector<kbEditorEntity*> SelectedObjects = g_Editor->GetSelectedObjects();
	g_Editor->DeleteEntities(SelectedObjects);
}

/// kbEditor::OutputCB
void kbEditor::OutputCB(kbOutputMessageType_t messageType, const char* output) {
	// Spin in-case this was called by a seperate thread
	while (g_StyleBuffer == nullptr);

	std::string outputBuffer = output;
	std::string styleBuffer;
	size_t outputLen = strlen(output);

	char outputColor = 'A';
	if (messageType == kbOutputMessageType_t::Message_Error || messageType == kbOutputMessageType_t::Message_Assert) {
		outputColor = 'B';
	} else if (messageType == kbOutputMessageType_t::Message_Warning) {
		outputColor = 'B';
	}

	for (int i = 0; i < outputLen; i++) {
		styleBuffer += outputColor;
	}

	g_OutputBuffer->append(outputBuffer.c_str());
	g_StyleBuffer->append(styleBuffer.c_str());

	if (messageType == kbOutputMessageType_t::Message_Assert) {
		fl_alert(output);
	}
}

/// kbEditor::RightClickOnMainTab
void kbEditor::RightClickOnMainTab() {

	const kbPrefab* const prefab = g_Editor->m_pResourceTab->GetSelectedPrefab();
	std::string ReplacePrefabMessage = "Replace Prefab";
	std::string PlacePrefabMessage = "Place Prefab";
	std::string DuplicateMessage = "Duplicate Entity";

	if (g_Editor->GetSelectedObjects().size() > 0) {
		DuplicateMessage += g_Editor->GetSelectedObjects()[0]->GetGameEntity()->GetName().stl_str();
	}

	if (prefab == nullptr) {
		PlacePrefabMessage += "into scene";
	} else {
		PlacePrefabMessage += "[" + prefab->GetPrefabName() + "] into scene.";
		ReplacePrefabMessage += "[" + prefab->GetPrefabName() + "]";
	}

	Fl_Menu_Item rclick_menu[] = {
		{ DuplicateMessage.c_str(), 0, DuplicateEntity, 0 },
		{ "Create New Prefab",  0, AddEntityAsPrefab, (void*)0 },
		{ ReplacePrefabMessage.c_str(), 0, ReplaceCurrentlySelectedPrefab, (void*)1 },
		{ PlacePrefabMessage.c_str(),  0, InsertSelectedPrefabIntoScene, (void*)this },
		{ 0 } };

	if (g_Editor->m_SelectedObjects.size() != 1) {
		rclick_menu[0].deactivate();
		rclick_menu[1].deactivate();
		rclick_menu[2].deactivate();
	}

	if (prefab == nullptr) {
		rclick_menu[3].deactivate();
	}

	const Fl_Menu_Item* m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
	if (m) {
		m->do_callback(0, m->user_data());
	}
}

/// kbEditor::RightClickOnOutputWindow
void kbEditor::RightClickOnOutputWindow() {
	Fl_Menu_Item rclick_menu[] = {
		{ "Clear Output",  0, ClearOutputBuffer, (void*)0 },
		{ 0 }
	};

	const Fl_Menu_Item* const m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
	if (m != nullptr) {
		m->do_callback(0, m->user_data());
	}
}

/// kbEditor::GetCurrentlySelectedPrefab
const kbPrefab* kbEditor::GetCurrentlySelectedPrefab() const {
	return m_pResourceTab->GetSelectedPrefab();
}

/// kbEditor::ReplaceCurrentlySelectedPrefab
void kbEditor::ReplaceCurrentlySelectedPrefab(class Fl_Widget*, void*) {
	if (g_Editor->m_SelectedObjects.size() != 1) {
		return;
	}

	kbPrefab* const pPrefab = g_Editor->m_pResourceTab->GetSelectedPrefab();
	if (pPrefab == nullptr) {
		return;
	}

	std::vector<kbGameEntity*> GameEntityList;
	for (int i = 0; i < g_Editor->m_SelectedObjects.size(); i++) {
		GameEntityList.push_back(g_Editor->m_SelectedObjects[i]->GetGameEntity());
	}

	g_ResourceManager.UpdatePrefab(pPrefab, GameEntityList);
	g_Editor->m_pResourceTab->MarkPrefabDirty(pPrefab);
	//	g_ResourceManager.DumpPackageInfo();
		//g_ResourceManager.SavePackages();
}

/// kbEditor::DuplicateEntity
void kbEditor::DuplicateEntity(Fl_Widget*, void* userdata) {
	auto& selectedObjects = g_Editor->GetSelectedObjects();
	if (selectedObjects.size() == 0) {
		return;
	}

	kbGameEntity* const pSrcEntity = selectedObjects[0]->GetGameEntity();
	kbGameEntity* const pDstEntity = new kbGameEntity(pSrcEntity, false);

	kbEditorEntity* const pEditorEntity = new kbEditorEntity(pDstEntity);
	pEditorEntity->SetPosition(pSrcEntity->GetPosition());
	g_Editor->m_GameEntities.push_back(pEditorEntity);
	g_Editor->m_pResourceTab->RefreshEntitiesTab();
}

/// kbEditor::AddEntityAsPrefab
void kbEditor::AddEntityAsPrefab(Fl_Widget*, void* userdata) {

	kbDialogBox dialogBox("Add Prefab To Library Package", "Save Prefab", "Cancel");
	dialogBox.AddTextField("Package:");
	dialogBox.AddTextField("Folder:");
	dialogBox.AddTextField("Name:");
	dialogBox.Run();

	blk::log("0 : %s", dialogBox.GetFieldEntry(0).c_str());
	blk::log("1 : %s", dialogBox.GetFieldEntry(1).c_str());
	blk::log("2 : %s", dialogBox.GetFieldEntry(2).c_str());

	std::string PackageName = dialogBox.GetFieldEntry(0).c_str();
	if (GetFileExtension(PackageName) != "kbPkg") {
		PackageName += ".kbPkg";
	}

	g_Editor->AddEntityAsPrefab_Internal(PackageName, dialogBox.GetFieldEntry(1).c_str(), dialogBox.GetFieldEntry(2).c_str());
}

/// kbEditor::AddEntityAsPrefab_Internal
void kbEditor::AddEntityAsPrefab_Internal(const std::string& PackageName, const std::string& FolderName, const std::string& PrefabName) {

	if (m_SelectedObjects.size() != 1) {
		return;
	}

	if (PackageName.empty() || FolderName.empty() || PrefabName.empty()) {
		fl_alert("Incomplete fields.  Prefab was not created");
		return;
	}

	kbPrefab* prefab;
	if (g_ResourceManager.AddPrefab(m_SelectedObjects[0]->GetGameEntity(), PackageName, FolderName, PrefabName, false, &prefab) == false) {
		int shouldOverwrite = fl_ask("Prefab with that name and path already exist.  Overwrite?");

		if (shouldOverwrite == 0)
			return;

		if (g_ResourceManager.AddPrefab(m_SelectedObjects[0]->GetGameEntity(), PackageName, FolderName, PrefabName, true, &prefab) == false) {
			fl_alert("Unable to add prefab");
			return;
		}
	}

	m_pResourceTab->AddPrefab(prefab, PackageName, FolderName, PrefabName);
	//g_ResourceManager.DumpPackageInfo();
	//g_ResourceManager.SavePackages();

	fl_alert("Prefab added successfully");
}

/// kbEditor::InsertSelectedPrefabIntoScene
void kbEditor::InsertSelectedPrefabIntoScene(Fl_Widget*, void* pUserdata) {

	const kbPrefab* const prefabToCreate = g_Editor->m_pResourceTab->GetSelectedPrefab();
	if (prefabToCreate == nullptr) {
		return;
	}

	const kbCamera* const editorCamera = g_Editor->m_pMainTab->GetEditorWindowCamera();
	if (editorCamera == nullptr) {
		return;
	}

	const kbVec3 entityLocation = editorCamera->m_Position + (editorCamera->m_Rotation.ToMat4()[2] * 4.0f).ToVec3();

	for (int i = 0; i < prefabToCreate->NumGameEntities(); i++) {
		kbGameEntity* const pNewEntity = new kbGameEntity(prefabToCreate->m_GameEntities[i], false);
		kbEditorEntity* const pEditorEntity = new kbEditorEntity(pNewEntity);
		pEditorEntity->SetPosition(entityLocation);
		g_Editor->m_GameEntities.push_back(pEditorEntity);
	}

	g_Editor->m_pResourceTab->RefreshEntitiesTab();
}

/// kbEditor::ViewModeChoiceCB
void kbEditor::ViewModeChoiceCB(Fl_Widget*, void* pUserData) {
	const int viewModeChoice = g_Editor->m_pViewModeChoice->value();
	g_pRenderer->SetViewMode((kbRenderer_DX11::kbViewMode_t)viewModeChoice);
}

/// kbEditor::ClearOutputBuffer
void kbEditor::ClearOutputBuffer(Fl_Widget*, void* pUseData) {
	g_OutputBuffer->text("");
	g_StyleBuffer->text("");
}

/// kbDialogBox::Run
bool kbDialogBox::Run() {
	blk::error_check(m_Fields.size() > 0, "Dialog box has no fields");
	blk::error_check(gCurrentDialogBox != nullptr, "Run called on an invalid dialog box");

	const int ButtonHeight = fl_height() + kbEditor::PanelBorderSize() * 2;
	const int popUpWidth = 600;
	const int popUpHeight = ((int)m_Fields.size() + 2) * kbEditor::LineSpacing() + ButtonHeight;
	int dx, dy, w, h;
	int MaxNameLength = 0;

	for (size_t i = 0; i < m_Fields.size(); i++) {
		fl_text_extents(m_Fields[i].m_FieldName.c_str(), dx, dy, w, h);

		if (w > MaxNameLength) {
			MaxNameLength = w;
		}
	}

	m_PopUpWindow = new Fl_Window(Fl::event_x(), Fl::event_y(), popUpWidth, popUpHeight);
	const int StartX = 2 * kbEditor::PanelBorderSize() + MaxNameLength;
	const int StartY = kbEditor::PanelBorderSize();

	for (int i = 0; i < m_Fields.size(); i++) {
		m_Fields[i].m_Input = new Fl_Input(StartX, StartY + kbEditor::LineSpacing() * i, popUpWidth - (MaxNameLength + kbEditor::PanelBorderSize() * 5), 20, m_Fields[i].m_FieldName.c_str());
	}

	int MaxButtonNameLen = 0;
	fl_text_extents(m_AcceptButtonName.c_str(), dx, dy, MaxButtonNameLen, h);
	fl_text_extents(m_CancelButtonName.c_str(), dx, dy, w, h);

	if (w > MaxButtonNameLen) {
		MaxButtonNameLen = w;
	}

	class Fl_Button* const pAcceptButton = new Fl_Button(StartX, StartY + kbEditor::LineSpacing(4), MaxButtonNameLen + kbEditor::PanelBorderSize(2), ButtonHeight, m_AcceptButtonName.c_str());
	Fl_Button* const pCancelButton = new Fl_Button(StartX + MaxButtonNameLen + kbEditor::PanelBorderSize() * 4, StartY + kbEditor::LineSpacing(4), MaxButtonNameLen, ButtonHeight, m_CancelButtonName.c_str());

	pAcceptButton->callback(AcceptButtonClicked);
	pCancelButton->callback(CancelButtonClicked);

	m_PopUpWindow->show();

	while (m_PopUpWindow->shown()) { Fl::wait(); }

	for (size_t i = 0; i < m_Fields.size(); i++) {
		m_Fields[i].m_FieldValue = m_Fields[i].m_Input->value();
	}

	delete m_PopUpWindow;
	m_PopUpWindow = nullptr;
	gCurrentDialogBox = nullptr;

	return m_bAccepted;
}