/// kbEditor.h
///
// 2016-2025 blk 1.0

#pragma once

#include "kbWidget.h"
#include "kbGame.h"
#include "kbUndoAction.h"

#pragma warning(push)
#pragma warning(disable:4312)
#include <fl/fl.h>
#include <fL/fl_draw.h>
#include <fl/fl_input.h>
#pragma warning(pop)

class kbWidget;
class kbEditorEntity;
class Fl_Widget;

enum widgetCBType_t;

 /// kbEditor
class kbEditor : Fl_Window {
public:
	kbEditor();
	~kbEditor();

	void UnloadMap();
	void LoadMap(const std::string& mapName);
	void SetGame(class kbGame* pGame) { m_pGame = pGame; }

	void Update();
	virtual int	handle(int theEvent);

	HWND main_viewport_hwnd() const;

	const bool IsRunning() const { return m_bIsRunning; }
	const bool IsRunningGame() const { return m_pGame != nullptr && m_pGame->IsPlaying(); }

	void RegisterUpdate(kbWidget* const widget) { m_UpdateWidgets.push_back(widget); }
	void RegisterEvent(kbWidget* const widget, const widgetCBType_t eventType) { m_EventReceivers[eventType].push_back(widget); }
	void BroadcastEvent(const class widgetCBObject& cbObject);
		 
	void SetMainCameraPos(const kbVec3& newCamPos);
	kbVec3 GetMainCameraPos() const;

	void SetMainCameraRot(const kbQuat& newCamRot);
	kbQuat GetMainCameraRot() const;

	void AddEntity(kbEditorEntity* const pEditorEntity);
	void SelectEntities(std::vector< kbEditorEntity* >& entitiesToSelect, bool AppendToSelectedList);
	void DeselectEntities();
		 
	void PushUndoAction(kbUndoAction* pUndoAction) { m_UndoStack.Push(pUndoAction); }
	void DeleteEntities(std::vector<kbEditorEntity*>& editorEntityList);

	std::vector<kbEditorEntity*>& GetGameEntities() { return m_GameEntities; }
	std::vector<kbEditorEntity*>& GetSelectedObjects() { return m_SelectedObjects; }

	const kbPrefab* GetCurrentlySelectedPrefab() const;

	const widgetCBInputObject& GetInput() const { return m_WidgetInputObject; }

	bool IsGameUpdating() const { return m_bGameUpdating; }

private:
	void SaveLevel_Internal(const std::string& fileName, const bool bForceSave);

	std::string	m_CurrentLevelFileName;

	std::vector<kbWidget*> m_UpdateWidgets;
	std::map<widgetCBType_t, std::vector< kbWidget*>> m_EventReceivers;
	std::vector<kbEditorEntity*> m_GameEntities;
	std::vector<kbEditorEntity*> m_SelectedObjects;
	std::vector<kbEditorEntity*> m_RemovedEntities;

	kbUndoStack	m_UndoStack;

	kbGame* m_pGame = nullptr;
	kbEditorWindow* m_pGameWindow = nullptr;
	class Fl_Choice* m_pSpeedChoice = nullptr;

	class Fl_Input* m_pXFormInput = nullptr;

	// widgets
	class kbMainTab* m_pMainTab = nullptr;
	class kbResourceTab* m_pResourceTab = nullptr;
	class Fl_Text_Display* m_pOutputText = nullptr;
	class kbPropertiesTab* m_pPropertiesTab = nullptr;
	class Fl_Choice* m_pViewModeChoice = nullptr;

	kbTimer	m_Timer;

	// input
	widgetCBInputObject	m_WidgetInputObject;

	bool m_bIsRunning = false;
	bool m_bRightMouseButtonDragged = false;
	bool m_bGameUpdating = false;

	// Stores a copy of the current undo action's id.  The level is dirty if the two values don't match
	uint64_t m_UndoIDAtLastSave = 0;

	// internal functions and callbacks
	void ShutDown();

	static void NewLevel(Fl_Widget*, void*);
	static void OpenLevel(Fl_Widget*, void*);
	static void SaveLevelAs(Fl_Widget*, void*);
	static void SaveLevel(Fl_Widget*, void*);
			    
	static void	Undo(Fl_Widget*, void*);
	static void	Redo(Fl_Widget*, void*);
	static void	Close(Fl_Widget*, void*);
	static void	CreateGameEntity(Fl_Widget*, void*);
	static void	AddComponent(Fl_Widget*, void*);
	static void	TranslationButtonCB(Fl_Widget*, void*);
	static void	RotationButtonCB(Fl_Widget*, void*);
	static void	ScaleButtonCB(Fl_Widget*, void*);
	static void	XPlusAdjustButtonCB(Fl_Widget*, void*);
	static void	YPlusAdjustButtonCB(Fl_Widget*, void*);
	static void	ZPlusAdjustButtonCB(Fl_Widget*, void*);
	static void	XNegAdjustButtonCB(Fl_Widget*, void*);
	static void	YNegAdjustButtonCB(Fl_Widget*, void*);
	static void	ZNegAdjustButtonCB(Fl_Widget*, void*);
	static void	AdjustCameraSpeedCB(Fl_Widget*, void*);
	static void	ToggleIconsCB(Fl_Widget*, void*);
	static void	OutputCB(kbOutputMessageType_t, const char*);
	static void	PlayGameFromHere(Fl_Widget*, void*);
	static void	StopGame(Fl_Widget*, void*);
	static void	DeleteEntitiesCB(Fl_Widget*, void*);
	static void	ViewModeChoiceCB(Fl_Widget*, void*);



	void RightClickOnMainTab();
	void RightClickOnOutputWindow();

	static void DuplicateEntity(Fl_Widget*, void*);
	static void ReplaceCurrentlySelectedPrefab(Fl_Widget*, void*);
	static void AddEntityAsPrefab(Fl_Widget*, void*);
	void AddEntityAsPrefab_Internal(const std::string& PackageName, const std::string& FolderName, const std::string& PrefabeName);
	static void InsertSelectedPrefabIntoScene(Fl_Widget*, void*);
	static void ClearOutputBuffer(Fl_Widget*, void* pUseData);

public:
	static const int TabHeight() { return 25; }
	static const int PanelBorderSize(int Multiplier = 1) { return 5 * Multiplier; }
	static const int LineSpacing(int Multiplier = 1) { return Multiplier * (fl_height() + PanelBorderSize()); }
};

extern kbEditor* g_Editor;

///  kbDialogBox
class kbDialogBox {
public:
	kbDialogBox(const char* const title, const char* const acceptButtonName, const char* const cancelButtonName) :
		m_PopUpWindow(nullptr),
		m_Title(title),
		m_AcceptButtonName(acceptButtonName),
		m_CancelButtonName(cancelButtonName),
		m_LineNumber(0),
		m_bAccepted(false) {

		if (gCurrentDialogBox != nullptr) {
			blk::error("Dialog box already open.  Only one allowed at a time");
		}

		gCurrentDialogBox = this;
	}

	~kbDialogBox() {
		gCurrentDialogBox = nullptr;
	}

	void AddTextField(const char* const field) {
		kbDialogBoxField newField;
		newField.m_FieldName = field;
		m_Fields.push_back(newField);
	}

	bool Run();

	const std::string& GetFieldEntry(const int fieldIdx) const {
		if (fieldIdx < 0 || fieldIdx >= m_Fields.size()) {
			blk::error("Bad field idx %d for dialog box", fieldIdx);
		}

		return m_Fields[fieldIdx].m_FieldValue;
	}

private:

	Fl_Window* m_PopUpWindow;
	int            m_LineNumber;

	struct kbDialogBoxField {
		std::string    m_FieldName;
		std::string    m_FieldValue;
		Fl_Input* m_Input;
	};

	std::string m_Title;
	std::string m_AcceptButtonName;
	std::string m_CancelButtonName;
	std::vector<kbDialogBoxField> m_Fields;
	bool m_bAccepted;

	void Exit() {
		m_PopUpWindow->hide();
	}

	static void AcceptButtonClicked(Fl_Widget*, void* userdata) {
		gCurrentDialogBox->m_bAccepted = true;
		gCurrentDialogBox->Exit();
	}

	static void CancelButtonClicked(Fl_Widget*, void* userdata) {
		gCurrentDialogBox->m_bAccepted = false;
		gCurrentDialogBox->Exit();
	}

	static kbDialogBox* gCurrentDialogBox;
};
