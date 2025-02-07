/// kbPropertiesTab.cpp
///
/// 2016-2025 blk 1.0

#include "kbCore.h"
#include "kbVector.h"
#include "kbWidget.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"
#include "kbPropertiesTab.h"
#include "FL/FL_Scroll.h"
#include "kbResourceTab.h"

#pragma warning(push)
#pragma warning(disable:4312)
#include <fl/fl_ask.h>
#include <fl/fl_check_button.h>
#include <fl/fl_choice.h>
#include <fl/fl_text_display.h>
#pragma warning(pop)

kbPropertiesTab* g_pPropertiesTab = nullptr;

/**
 *	propertiesTabCBData_t::propertiesTabCBData_t
 */
propertiesTabCBData_t::propertiesTabCBData_t(
	kbEditorEntity* const pEditorEntity,
	const kbGameEntityPtr* const inGameEntityPtr,
	kbComponent* const pComponent,
	kbComponent* const pParentComponent,
	const kbResource** pResource,
	const kbString variableName,
	void* const pVariableValue,
	const kbTypeInfoType_t variableType,
	const std::string& structName,
	const void* const pArray,
	const int arrayIdx) {

	m_pEditorEntity = pEditorEntity;
	if (inGameEntityPtr != nullptr) {
		m_GameEntityPtr = *inGameEntityPtr;
	}
	m_pComponent = pComponent;
	m_pParentComponent = pParentComponent;
	m_pResource = pResource;
	m_VariableName = variableName;
	m_pVariablePtr = pVariableValue;
	m_VariableType = variableType;
	m_StructName = structName;
	m_pArray = pArray;
	m_ArrayIndex = arrayIdx;
}

///  kbPropertiesTab::kbPropertiesTab
kbPropertiesTab::kbPropertiesTab(int widgetX, int widgetY, int widgetWidth, int widgetHeight) :
	kbWidget(widgetX, widgetY, widgetWidth, widgetHeight),
	Fl_Tabs(widgetX, widgetY, widgetWidth, widgetHeight),
	m_pTempPrefabEntity(nullptr),
	m_bRefreshNextUpdate(false) {

	m_pPropertiesTab = new Fl_Tabs(x(), y(), widgetWidth, h());

	m_pEntityProperties = new Fl_Group(x() + kbEditor::PanelBorderSize(), y() + kbEditor::TabHeight(), widgetWidth, widgetHeight, "Entity Info");
	m_pEntityProperties->end();

	m_pResourceProperties = new Fl_Group(0, y() + kbEditor::TabHeight(), DisplayWidth(), widgetHeight, "Resource Info");
	m_pResourceProperties->end();

	m_pPropertiesTab->end();

	// register events
	g_Editor->RegisterUpdate(this);
	g_Editor->RegisterEvent(this, WidgetCB_EntitySelected);
	g_Editor->RegisterEvent(this, WidgetCB_PrefabSelected);
	g_Editor->RegisterEvent(this, WidgetCB_EntityDeselected);
	g_Editor->RegisterEvent(this, WidgetCB_ComponentCreated);
	g_Editor->RegisterEvent(this, WidgetCB_ResourceSelected);

	g_pPropertiesTab = this;
}

///  kbPropertiesTab::Update
void kbPropertiesTab::Update() {
	kbWidget::Update();

	if (m_bRefreshNextUpdate) {
		RefreshEntity();
		m_bRefreshNextUpdate = false;
	}
}

///  kbPropertiesTab::EventCB
void kbPropertiesTab::EventCB(const widgetCBObject* const widgetCBObject) {
	switch (widgetCBObject->widgetType) {
	case WidgetCB_EntitySelected:
	{
		if (m_pTempPrefabEntity != nullptr) {
			m_pTempPrefabEntity->SetGameEntity(nullptr);
			delete m_pTempPrefabEntity;
			m_pTempPrefabEntity = nullptr;
		}

		widgetCBEntitySelected* const pEntitySelectedCB = (widgetCBEntitySelected*)widgetCBObject;
		m_SelectedEntities = g_Editor->GetSelectedObjects();//pEntitySelectedCB->entitiesSelected;
		RefreshEntity();
	}
	break;

	case WidgetCB_EntityDeselected:
	{
		if (m_pTempPrefabEntity != nullptr) {
			m_pTempPrefabEntity->SetGameEntity(nullptr);
			delete m_pTempPrefabEntity;
			m_pTempPrefabEntity = nullptr;
		}

		m_SelectedEntities.clear();
		RefreshEntity();
	}
	break;

	case WidgetCB_ComponentCreated:
	{
		RefreshEntity();
	}
	break;

	case WidgetCB_ResourceSelected:
	{
		m_CurrentlySelectedResource = static_cast<const widgetCBResourceSelected*>(widgetCBObject)->resourceFileName;
	}
	break;

	case WidgetCB_PrefabSelected:
	{
		m_SelectedEntities.clear();
		m_pTempPrefabEntity = new kbEditorEntity(const_cast<kbGameEntity*>(g_Editor->GetCurrentlySelectedPrefab()->GetGameEntity(0)));
		RefreshEntity();
	}
	break;
	}
}

///  kbPropertiesTab::CheckButtonCB
void kbPropertiesTab::CheckButtonCB(Fl_Widget* widget, void* voidPtr) {

	propertiesTabCBData_t* const userData = static_cast<propertiesTabCBData_t*>(voidPtr);
	blk::error_check(userData != nullptr, "bPropertiesTab::CheckButtonCB() - NULL userData");

	Fl_Check_Button* const pCheckButton = static_cast<Fl_Check_Button*>(widget);

	/*
		kbGameEntity *const pGameEntity = (kbGameEntity*)( pModifiedComponent->IsA( kbGameComponent::GetType() ) ? ( pModifiedComponent->GetOwner() ) : ( nullptr ) );

		if ( pGameEntity != nullptr && pGameEntity->GetComponent(0) == pModifiedComponent ) {
			// Refresh all components if the transform component was modified
			kbTransformComponent *const pTransformComponent = (kbTransformComponent*)pGameEntity->GetComponent(0);

			for ( int i = 0; i < pGameEntity->NumComponents(); i++ ) {
				kbComponent *const pCurComp = pGameEntity->GetComponent(i);
				if ( pCurComp->IsEnabled() ) {
					pCurComp->Enable( false );
					pCurComp->Enable( true );
				}
			}
		} else if ( userData->m_pComponent->IsEnabled() ) {
			userData->m_pComponent->Enable( false );
			userData->m_pComponent->Enable( true );
		}
	*/
	const char buttonVal = pCheckButton->value();

	*((bool*)userData->m_pVariablePtr) = (bool)buttonVal;

	userData->m_pComponent->EditorChange(userData->m_VariableName.stl_str());


	kbComponent* const pModifiedComponent = userData->m_pComponent;
	if (pModifiedComponent->IsA(kbTransformComponent::GetType())) {

	}

	PropertyChangedCB(userData->m_GameEntityPtr);
}

///  kbPropertiesTab::PointerButtonCB
void kbPropertiesTab::PointerButtonCB(Fl_Widget* widget, void* voidPtr) {

	propertiesTabCBData_t* const userData = static_cast<propertiesTabCBData_t*>(voidPtr);
	blk::error_check(userData != nullptr, "kbPropertiesTab::PointerButtonCB() - null user data passed in");

	const std::string* const fieldName = (std::string*)userData->m_pVariablePtr;
	if (userData->m_VariableType == KBTYPEINFO_GAMEENTITY) {
		const kbPrefab* const pPrefab = g_Editor->GetCurrentlySelectedPrefab();
		kbGameEntityPtr& pEntityPtr = *(kbGameEntityPtr*)userData->m_pVariablePtr;

		kbGameEntity* const pEntity = g_pResourceTab->GetSelectedGameEntity().GetEntity();
		if (pEntity || pPrefab == nullptr) {
			pEntityPtr.SetEntity(const_cast<kbGameEntity*>(g_pResourceTab->GetSelectedGameEntity().GetEntity()));
		}
		else {
			pEntityPtr.SetEntity(const_cast<kbGameEntity*>(pPrefab->GetGameEntity(0)));
		}

		if (fieldName == nullptr) {
			blk::warn("kbPropertiesTab::PointerButtonCB() - Field name is null!");
		}
		else {
			if (userData->m_pComponent != nullptr) {
				userData->m_pComponent->EditorChange(*fieldName);
				if (userData->m_pParentComponent != nullptr) {
					userData->m_pParentComponent->EditorChange(*fieldName);
				}
			}
		}

		g_pPropertiesTab->RefreshEntity();
		return;
	}

	if (g_pPropertiesTab->m_CurrentlySelectedResource.empty()) {
		return;
	}

	kbResource* const pResource = (kbResource*)g_ResourceManager.GetResource(g_pPropertiesTab->m_CurrentlySelectedResource.c_str(), true, true);

	// Don't do anything if the selected resource is not the right type, or if it's the same resource that's already present
	if (pResource->GetType() != userData->m_VariableType || *userData->m_pResource == pResource) {
		return;
	}

	*userData->m_pResource = pResource;

	userData->m_pComponent->EditorChange(*fieldName);
	if (userData->m_pParentComponent != nullptr) {
		userData->m_pParentComponent->EditorChange(*fieldName);
	}

	g_pPropertiesTab->RefreshEntity();

	PropertyChangedCB(userData->m_GameEntityPtr);
}

///  kbPropertiesTab::ClearPointerButtonCB
void kbPropertiesTab::ClearPointerButtonCB(Fl_Widget* widget, void* voidPtr) {

	propertiesTabCBData_t* const userData = static_cast<propertiesTabCBData_t*>(voidPtr);
	blk::error_check(userData != nullptr, "kbPropertiesTab::ClearPointerButtonCB() - null user data passed in");

	const std::string* const fieldName = (std::string*)userData->m_pVariablePtr;
	if (userData->m_VariableType == KBTYPEINFO_GAMEENTITY) {
		const kbPrefab* const pPrefab = g_Editor->GetCurrentlySelectedPrefab();
		kbGameEntityPtr& pEntityPtr = userData->m_GameEntityPtr;

		pEntityPtr.SetEntity(nullptr);

		userData->m_pComponent->EditorChange(*fieldName);
		if (userData->m_pParentComponent != nullptr) {
			userData->m_pParentComponent->EditorChange(*fieldName);
		}

		g_pPropertiesTab->RefreshEntity();
		return;
	}

	*userData->m_pResource = nullptr;

	userData->m_pComponent->EditorChange(*fieldName);
	if (userData->m_pParentComponent != nullptr) {
		userData->m_pParentComponent->EditorChange(*fieldName);
	}

	g_pPropertiesTab->RefreshEntity();

	PropertyChangedCB(userData->m_GameEntityPtr);
}

///  IsNumeric
bool IsNumeric(const char* const cString) {
	const int fieldTextLen = (int)strlen(cString);
	bool bHasDecimalSpot = false;

	for (int i = 0; i < fieldTextLen; i++) {
		if (cString[i] == '.') {
			if (bHasDecimalSpot) {
				return false;
			}
			bHasDecimalSpot = true;
			continue;
		}
		else if (cString[i] == 'f') {
			if (i < fieldTextLen - 1) {
				return false;
			}
		}

		if (i == 0 && cString[i] == '-' && fieldTextLen > 1) {
			continue;
		}

		if (cString[i] < '0' || cString[i] > '9') {
			g_pPropertiesTab->RequestRefreshNextUpdate();
			return false;
		}
	}

	return true;
}

///  kbPropertiesTab::TextFieldCB
void kbPropertiesTab::TextFieldCB(Fl_Widget* widget, void* voidPtr) {
	propertiesTabCBData_t* const userData = static_cast<propertiesTabCBData_t*>(voidPtr);
	blk::error_check(userData != nullptr, "kbPropertiesTab::TextFieldCB() - NULL userData passed in");

	Fl_Input* const inputField = (Fl_Input*)widget;
	std::string inputValue = inputField->value();
	bool isByte = false;
	if (userData->m_VariableType != KBTYPEINFO_KBSTRING) {
		if (IsNumeric(inputValue.c_str()) == false) {
			if (inputValue[inputValue.size() - 1] == 'B' || inputValue[inputValue.size() - 1] == 'b') {
				inputValue.pop_back();
				if (IsNumeric(inputValue.c_str()) == false) {
					return;
				}
				isByte = true;
			}
		}
	}

	void* prevValuePtr = nullptr;
	void* curValuePtr = nullptr;

	const std::string currentValue = inputValue;
	inputField->undo();
	const std::string prevValue = inputValue;
	inputField->value(currentValue.c_str());

	const float divisor = (isByte) ? (255.0f) : (1.0f);
	if (userData->m_VariableType == KBTYPEINFO_VECTOR4 || userData->m_VariableType == KBTYPEINFO_VECTOR) {
		float& componentVar = *(float*)userData->m_pVariablePtr;

		// [TODO][HACK] - Ehm...Are these news cleared up somewhere?
		prevValuePtr = new float((float)atof(prevValue.c_str()));
		curValuePtr = new float((float)atof(currentValue.c_str()) / divisor);

		componentVar = *(float*)curValuePtr;

	}
	else if (userData->m_VariableType == KBTYPEINFO_INT) {
		int& componentVar = *(int*)userData->m_pVariablePtr;
		prevValuePtr = new int(atoi(prevValue.c_str()));
		curValuePtr = new int(atoi(currentValue.c_str()));

		componentVar = (int)atoi(inputField->value());
	}
	else if (userData->m_VariableType == KBTYPEINFO_FLOAT) {
		float& componentVar = *(float*)userData->m_pVariablePtr;
		prevValuePtr = new float((float)atof(prevValue.c_str()));
		curValuePtr = new float((float)atof(currentValue.c_str()) / divisor);

		componentVar = *(float*)curValuePtr;

	}
	else if (userData->m_VariableType == KBTYPEINFO_KBSTRING) {
		kbString& curString = *(kbString*)userData->m_pVariablePtr;
		curString = inputField->value();

		prevValuePtr = new kbString(prevValue.c_str());
		curValuePtr = new kbString(currentValue.c_str());
	}

	g_Editor->PushUndoAction(new kbUndoVariableAction(userData->m_VariableType, prevValuePtr, curValuePtr, userData->m_pVariablePtr));

	kbComponent* const pModifiedComponent = userData->m_pComponent;
	kbGameEntity* const pGameEntity = (kbGameEntity*)(pModifiedComponent->IsA(kbGameComponent::GetType()) ? (pModifiedComponent->GetOwner()) : (nullptr));

	if (pGameEntity != nullptr && pGameEntity->GetComponent(0) == pModifiedComponent) {
		// Refresh all components if the transform component was modified
		kbTransformComponent* const pTransformComponent = (kbTransformComponent*)pGameEntity->GetComponent(0);
		for (int i = 0; i < pGameEntity->NumComponents(); i++) {
			kbComponent* const pCurComp = pGameEntity->GetComponent(i);
			if (pCurComp->IsEnabled()) {
				pCurComp->Enable(false);
				pCurComp->Enable(true);
			}
		}
	}
	else if (userData->m_pComponent->IsEnabled()) {
		userData->m_pComponent->Enable(false);
		userData->m_pComponent->Enable(true);
	}

	userData->m_pComponent->EditorChange(userData->m_VariableName.stl_str());
	if (userData->m_pParentComponent != nullptr) {
		userData->m_pParentComponent->EditorChange(userData->m_VariableName.stl_str());
	}

	delete prevValuePtr;
	delete curValuePtr;

	PropertyChangedCB(userData->m_GameEntityPtr);
}

/**
 *	kbPropertiesTab::ArrayExpandCB
 */
void kbPropertiesTab::ArrayExpandCB(Fl_Widget* widet, void* userData) {

	blk::error_check(userData != nullptr, "kbPropertiesTab::ArrayExpandCB() - NULL userData passed in");

	varMetaData_t* entry = static_cast<varMetaData_t*>(userData);
	entry->bExpanded = !entry->bExpanded;

	g_pPropertiesTab->RefreshEntity();

	PropertyChangedCB(kbGameEntityPtr());
}

/**
 *	kbPropertiesTab::ArrayResizeCB
 */
void kbPropertiesTab::ArrayResizeCB(Fl_Widget* widget, void* voidPtr) {

	propertiesTabCBData_t* const userData = static_cast<propertiesTabCBData_t*>(voidPtr);
	blk::error_check(userData != nullptr, "kbPropertiesTab::ArrayResizeCB() - NULL userData passed in");

	const Fl_Input* const inputField = (Fl_Input*)widget;
	const char* const inputText = inputField->value();

	if (IsNumeric(inputText) == false) {
		return;
	}

	const int fieldValue = atoi(inputText);

	if (fieldValue < 0 || fieldValue > 128) {
		blk::warn("Array value is not between 0 and 128");
		g_pPropertiesTab->RequestRefreshNextUpdate();
		return;
	}

	switch (userData->m_VariableType) {
	case KBTYPEINFO_SHADER:
	{
		std::vector<kbShader*>* shaderList = (std::vector<kbShader*> *)(userData->m_pVariablePtr);
		shaderList->resize(fieldValue);
		break;
	}

	case KBTYPEINFO_TEXTURE:
	{
		std::vector<kbTexture*>* textureList = (std::vector<kbTexture*> *)(userData->m_pVariablePtr);
		textureList->resize(fieldValue);
		break;
	}

	default:
	{
		g_NameToTypeInfoMap->ResizeVector(userData->m_pVariablePtr, userData->m_StructName, fieldValue);
		break;
	}
	}

	if (userData->m_pComponent->IsEnabled()) {
		userData->m_pComponent->Enable(false);
		userData->m_pComponent->Enable(true);
	}

	userData->m_pComponent->EditorChange(userData->m_VariableName.stl_str());
	if (userData->m_pParentComponent != nullptr) {
		userData->m_pParentComponent->EditorChange(userData->m_VariableName.stl_str());
	}

	g_pPropertiesTab->RequestRefreshNextUpdate();
	PropertyChangedCB(userData->m_GameEntityPtr);
}

/**
 *	kbPropertiesTab::EnumCB
 */
void kbPropertiesTab::EnumCB(Fl_Widget* widget, void* voidPtr) {

	propertiesTabCBData_t* const userData = static_cast<propertiesTabCBData_t*>(voidPtr);
	blk::error_check(userData != nullptr, "kbPropertiesTab::EnumCB() - NULL userData passed in");

	Fl_Choice* const pDropDown = (Fl_Choice*)widget;
	int dropDownValue = pDropDown->value();

	int& componentVar = *(int*)userData->m_pVariablePtr;
	componentVar = dropDownValue;

	userData->m_pComponent->Enable(false);
	userData->m_pComponent->EditorChange(userData->m_VariableName.c_str());
	userData->m_pComponent->Enable(true);

	g_pPropertiesTab->RequestRefreshNextUpdate();

	PropertyChangedCB(userData->m_GameEntityPtr);
}

/**
 *	kbPropertiesTab::PropertyChangedCB
 */
void kbPropertiesTab::PropertyChangedCB(const kbGameEntityPtr entityPtr) {
	if (g_pPropertiesTab->m_pTempPrefabEntity != nullptr) {
		g_Editor->BroadcastEvent(widgetCBGeneric(WidgetCB_PrefabModified, g_pPropertiesTab->m_pTempPrefabEntity->GetGameEntity()));
	}

	g_Editor->BroadcastEvent(widgetCBGeneric(WidgetCB_EntityModified, nullptr));
}

/**
 *	kbPropertiesTab::RefreshComponent
 */
void kbPropertiesTab::RefreshComponent(kbEditorEntity* const pEntity, kbComponent* const pComponent, kbComponent* const pParentComponent, int& startX, int& curY, const int inputHeight, const bool bIsStruct, const void* const pArrayPtr, const int arrayIndex) {

	byte* const componentBytePtr = (byte*)pComponent;

	// Display Component class name ( kbStaticMeshComponent, kbSkeletalMeshComponent, etc );
	const char* const pComponentName = pComponent->GetComponentClassName();

	if (bIsStruct == false) {
		Fl_Text_Display* const propertyNameLabel = new Fl_Text_Display(startX + 10, curY, 0, inputHeight, pComponentName);
		propertyNameLabel->labelsize((int)(FontSize() * 1.25f));
		propertyNameLabel->labelfont(FL_BOLD);
		propertyNameLabel->align(FL_ALIGN_RIGHT);
	}

	// Delete button
	if (pComponent->IsA(kbTransformComponent::GetType()) == false) {
		if (bIsStruct == false) {
			Fl_Button* const DeleteButton = new Fl_Button(startX + DisplayWidth() - FontSize() - Fl::scrollbar_size(), curY + FontSize() / 2, inputHeight, inputHeight, "X");
			DeleteButton->color(88 + 1);

			propertiesTabCBData_t cbData(pEntity, nullptr, pComponent, pParentComponent, nullptr, kbString(""), nullptr, KBTYPEINFO_NONE, "", nullptr, -1);
			m_CallBackData.push_back(cbData);

			DeleteButton->callback(DeleteComponent, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
		}
		else {
			Fl_Button* const InsertButton = new Fl_Button(startX + DisplayWidth() - FontSize() - Fl::scrollbar_size() - 16, curY + FontSize() / 2, inputHeight, inputHeight, "+");
			InsertButton->color(0x00ff00ff);
			InsertButton->labelsize(2);

			Fl_Button* const DeleteButton = new Fl_Button(startX + DisplayWidth() - FontSize() - Fl::scrollbar_size(), curY + FontSize() / 2, inputHeight, inputHeight, "-");
			DeleteButton->color(88 + 1);
			DeleteButton->labelsize(2);

			propertiesTabCBData_t cbData(pEntity, nullptr, pComponent, pParentComponent, nullptr, kbString(""), nullptr, KBTYPEINFO_NONE, pComponentName, pArrayPtr, arrayIndex);
			m_CallBackData.push_back(cbData);

			InsertButton->callback(InsertArrayStruct, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
			DeleteButton->callback(DeleteArrayStruct, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
		}
	}

	curY += LineSpacing();

	// Collect the members and sort them based on offset
	std::vector< kbTypeInfoHierarchyIterator::iteratorType > membersList;
	kbTypeInfoHierarchyIterator iterator(pComponent);
	for (auto pNextField = iterator.Begin(); iterator.IsDone() == false; pNextField = iterator.GetNextTypeInfoField())
	{
		membersList.push_back(pNextField);
	}

	std::sort(membersList.begin(), membersList.end(), [](kbTypeInfoHierarchyIterator::iteratorType a, kbTypeInfoHierarchyIterator::iteratorType b) {
		return a->second.Offset() < b->second.Offset();
	});

	// Iterate over the component's properties and display them
	for (size_t j = 0; j < membersList.size(); j++) {

		auto pNextField = membersList[j];
		const char* const varName = pNextField->first.c_str();

		if (bIsStruct) {
			if (pNextField->first == "Enabled") {
				curY -= 2 * LineSpacing();
				continue;
			}
		}
		Fl_Text_Display* const propertyNameLabel = new Fl_Text_Display(startX + 10, curY, 0, inputHeight, varName);
		propertyNameLabel->labelsize(FontSize());
		propertyNameLabel->align(FL_ALIGN_RIGHT);

		const byte* const byteOffsetToVar = componentBytePtr + pNextField->second.Offset();

		if (pNextField->second.IsArray()) {

			varMetaData_t* const propertyMetaData = pEntity->GetPropertyMetaData(pComponent, pNextField->second.Offset());
			if (propertyMetaData == nullptr) {
				continue;
			}

			// Expand / collapse button
			Fl_Button* b1 = nullptr;
			if (propertyMetaData->bExpanded == false) {
				b1 = new Fl_Button(startX, curY + FontSize() / 2, FontSize(), FontSize(), "+");
				b1->color(0x00ff00ff);

			}
			else {
				b1 = new Fl_Button(startX, curY + FontSize() / 2, FontSize(), FontSize(), "-");
				b1->color(0xff0000ff);
			}
			b1->callback(&ArrayExpandCB, static_cast<void*>(propertyMetaData));
			b1->labelsize(FontSize());

			const std::string propertyNameWithPadding = "LONGEST STRING";
			const int propertyNamePixelWidth = (int)fl_width(propertyNameWithPadding.c_str());

			Fl_Input* const pArraySizeInput = new Fl_Input(startX + propertyNamePixelWidth + kbEditor::PanelBorderSize(1) - 1, curY, FontSize() * 2, inputHeight);
			pArraySizeInput->textsize(FontSize());
			pArraySizeInput->labelsize(FontSize());
			curY += LineSpacing();

			propertiesTabCBData_t cbData(pEntity, nullptr, pComponent, pParentComponent, nullptr, pNextField->first, (void*)byteOffsetToVar, pNextField->second.Type(), pNextField->second.GetStructName(), nullptr, -1);
			m_CallBackData.push_back(cbData);

			pArraySizeInput->callback(&ArrayResizeCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));

			switch (pNextField->second.Type()) {

			case KBTYPEINFO_SHADER:
			{
				const std::vector<class kbShader*>* const shaderList = (std::vector<class kbShader*> *)(byteOffsetToVar);

				pArraySizeInput->value(std::to_string(shaderList->size()).c_str());
				pArraySizeInput->textsize(FontSize());
				m_pEntityProperties->add(pArraySizeInput);

				if (propertyMetaData && propertyMetaData->bExpanded) {
					for (int i = 0; i < shaderList->size(); i++) {
						RefreshProperty(pEntity, pNextField->first, pNextField->second.Type(), pNextField->second.GetStructName(), pComponent, (byte*)&(*shaderList)[i], pParentComponent, startX, curY, inputHeight);
						curY += LineSpacing();
					}
				}
				break;
			}

			case KBTYPEINFO_TEXTURE:
			{
				const std::vector<class kbTexture*>* const textureList = (std::vector<class kbTexture*> *)(byteOffsetToVar);

				pArraySizeInput->value(std::to_string(textureList->size()).c_str());
				pArraySizeInput->textsize(FontSize());
				m_pEntityProperties->add(pArraySizeInput);

				if (propertyMetaData && propertyMetaData->bExpanded) {
					for (int i = 0; i < textureList->size(); i++) {
						RefreshProperty(pEntity, pNextField->first, pNextField->second.Type(), pNextField->second.GetStructName(), pComponent, (byte*)&(*textureList)[i], pParentComponent, startX, curY, inputHeight);
						curY += LineSpacing();
					}
				}
				break;
			}
			default:
				const size_t vectorSize = g_NameToTypeInfoMap->GetVectorSize(byteOffsetToVar, pNextField->second.GetStructName());
				pArraySizeInput->value(std::to_string(vectorSize).c_str());

				static std::vector<std::string> indexText;

				if (indexText.size() < vectorSize) {
					const int prevSize = (int)indexText.size();
					indexText.resize(vectorSize);
					for (int i = prevSize; i < vectorSize; i++) {
						indexText[i] = "[";
						indexText[i] += std::to_string(i);
						indexText[i] += "]";
					}
				}

				if (propertyMetaData && propertyMetaData->bExpanded) {

					for (int i = 0; i < vectorSize; i++) {
						Fl_Text_Display* propertyNameLabel = new Fl_Text_Display(startX + 24, curY + LineSpacing(), 0, inputHeight, indexText[i].c_str());
						byte* curComponentByte = (byte*)g_NameToTypeInfoMap->GetVectorElement(byteOffsetToVar, pNextField->second.GetStructName(), i);
						startX += kbEditor::PanelBorderSize(5);
						RefreshProperty(pEntity, pNextField->first, pNextField->second.Type(), pNextField->second.GetStructName(), pComponent, curComponentByte, pParentComponent, startX, curY, inputHeight, byteOffsetToVar, i);
						curY += LineSpacing();
						startX -= kbEditor::PanelBorderSize(5);
					}
				}
				break;
			}
		}
		else {
			RefreshProperty(pEntity, pNextField->first, pNextField->second.Type(), pNextField->second.GetStructName(), pComponent, byteOffsetToVar, pParentComponent, startX, curY, inputHeight);
			curY += LineSpacing();
		}
	}
}

/**
 *	kbPropertiesTab::RefreshEntity
 */
void kbPropertiesTab::RefreshEntity() {

	// note, must delete them both and readd them with the entity property first
	Fl::delete_widget(m_pEntityProperties);
	Fl::delete_widget(m_pResourceProperties);

	m_CallBackData.clear();
	size_t previousCapacity = m_CallBackData.capacity();

	m_pPropertiesTab->redraw();
	m_pEntityProperties = new Fl_Group(x() + kbEditor::PanelBorderSize(), y() + kbEditor::TabHeight() + kbEditor::PanelBorderSize(), w() - kbEditor::PanelBorderSize(2), h() - kbEditor::TabHeight(), "Entity Info");
	Fl_Scroll* const scroller = new Fl_Scroll(x() + kbEditor::PanelBorderSize(), y() + kbEditor::TabHeight() + kbEditor::PanelBorderSize(), w() - kbEditor::PanelBorderSize(2), h() - kbEditor::TabHeight() - kbEditor::PanelBorderSize(2), "");

	if (m_SelectedEntities.size() == 1 || m_pTempPrefabEntity != nullptr) {	// todo: Don't display properties if multiple entities are selected
		int curY = y() + kbEditor::TabHeight() + kbEditor::PanelBorderSize();
		int startX = x() + kbEditor::PanelBorderSize();
		int inputWidth = 50;
		int inputHeight = (int)(FontSize() * 1.5f);
		int ySpacing = 20;

		// TODO: Display properties for the first entity only for now.
		kbEditorEntity* pEntity = (m_SelectedEntities.size() > 0) ? (m_SelectedEntities[0]) : (m_pTempPrefabEntity);
		const kbGameEntity* pGameEntity = pEntity->GetGameEntity();

		for (size_t i = 0; i < pGameEntity->NumComponents(); i++) {
			RefreshComponent(pEntity, const_cast<kbGameComponent*>(pGameEntity->GetComponent(i)), nullptr, startX, curY, inputHeight);
			curY += LineSpacing();
		}
	}

	m_pPropertiesTab->add(m_pEntityProperties);

	m_pResourceProperties = new Fl_Group(0, y() + kbEditor::TabHeight(), DisplayWidth(), h(), "Resource Info");
	m_pPropertiesTab->add(m_pResourceProperties);
	scroller->end();
	Fl::wait();

	// hack - if the size of m_CallBackData has grown, recall this function so that pointers in m_CallBackData are valid
	if (previousCapacity < m_CallBackData.capacity()) {
		m_CallBackData.reserve(m_CallBackData.capacity() * 2);
		RefreshEntity();
	}
}

/**
 *	kbPropertiesTab::RefreshProperty
 */
void kbPropertiesTab::RefreshProperty(kbEditorEntity* const pEntity, const std::string& propertyName, const kbTypeInfoType_t propertyType, const std::string& structName, kbComponent* const pComponent, const byte* const byteOffsetToVar, kbComponent* const pParentComponent, int& xPos, int& yPos, const int inputHeight, const void* const pArrayPtr, const int arrayIndex) {

	propertiesTabCBData_t cbData(pEntity, nullptr, pComponent, pParentComponent, nullptr, propertyName, nullptr, propertyType, "", nullptr, -1);

	const std::string propertyNameWithPadding = " LONGEST STRING";

	const int propertyNamePixelWidth = (int)fl_width(propertyNameWithPadding.c_str());
	int scrollBarSize = Fl::scrollbar_size();

	const int maxFieldWidth = x() + w() - (scrollBarSize + xPos + propertyNamePixelWidth + kbEditor::PanelBorderSize(2));
	const int fourComponentFieldWidth = (maxFieldWidth / 4) + (kbEditor::PanelBorderSize() / 8);
	const int threeComponentFieldWidth = (maxFieldWidth / 3) + (kbEditor::PanelBorderSize() / 8);

	switch (propertyType) {
	case KBTYPEINFO_BOOL:
	{
		bool& boolean = *(bool*)byteOffsetToVar;
		cbData.m_pVariablePtr = &boolean;
		m_CallBackData.push_back(cbData);

		Fl_Check_Button* const button = new Fl_Check_Button(xPos + propertyNamePixelWidth - 2, yPos, inputHeight, inputHeight, "");
		button->labelsize(inputHeight);
		button->callback(&CheckButtonCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
		button->value(*((bool*)byteOffsetToVar));
		break;
	}

	case KBTYPEINFO_INT:
	{
		int& integer = *(int*)byteOffsetToVar;
		cbData.m_pVariablePtr = &integer;
		m_CallBackData.push_back(cbData);

		int theW = x() + w();
		int dif = theW - xPos;

		Fl_Input* intInput = new Fl_Input(xPos + propertyNamePixelWidth, yPos, maxFieldWidth, inputHeight);
		intInput->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
		char buffer[16];
		sprintf_s(buffer, "%d", *((int*)byteOffsetToVar));
		intInput->value(buffer);
		intInput->textsize(FontSize());
		break;
	}

	case KBTYPEINFO_FLOAT:
	{
		float& numValue = *(float*)byteOffsetToVar;
		cbData.m_pVariablePtr = &numValue;
		m_CallBackData.push_back(cbData);

		Fl_Input* floatInput = new Fl_Input(xPos + propertyNamePixelWidth, yPos, maxFieldWidth, inputHeight);
		floatInput->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
		floatInput->textsize(FontSize());
		char buffer[16];
		floatInput->value(buffer);
		sprintf_s(buffer, "%.6f", *((float*)byteOffsetToVar));
		floatInput->value(buffer);
		break;
	}

	case KBTYPEINFO_STRUCT:
	{
		kbComponent* const pStruct = (kbComponent*)byteOffsetToVar;
		yPos += inputHeight;
		RefreshComponent(pEntity, pStruct, pComponent, xPos, yPos, inputHeight, true, pArrayPtr, arrayIndex);
		break;
	}

	case KBTYPEINFO_GAMEENTITY:
	{
		kbGameEntityPtr* const pEntityPtr = (kbGameEntityPtr*)byteOffsetToVar;
		cbData.m_pVariablePtr = pEntityPtr;
		kbGameEntity* const pEntity = pEntityPtr->GetEntity();
		if (pEntity == nullptr) {
			Fl_Text_Display* propertyNameLabel = new Fl_Text_Display(xPos + propertyNamePixelWidth, yPos, 0, inputHeight, "nullptr");
			propertyNameLabel->textsize(FontSize());
			propertyNameLabel->align(FL_ALIGN_RIGHT);
		}
		else {
			Fl_Text_Display* propertyNameLabel = new Fl_Text_Display(xPos + propertyNamePixelWidth, yPos, 0, inputHeight, pEntity->GetName().c_str());
			propertyNameLabel->textsize(FontSize());
			propertyNameLabel->align(FL_ALIGN_RIGHT);
		}

		Fl_Button* const b1 = new Fl_Button(xPos + propertyNamePixelWidth - (5 + inputHeight / 2), yPos + (int)(inputHeight * 0.25f), inputHeight / 2, inputHeight / 2, ">");
		b1->color(88 + 1);
		b1->labelsize(FontSize());

		cbData.m_VariableType = propertyType;
		cbData.m_GameEntityPtr = *pEntityPtr;

		blk::log("Prop name is %s %d %d ", propertyName.c_str(), (UINT_PTR)pComponent, (UINT_PTR)pParentComponent);
		m_CallBackData.push_back(cbData);
		b1->callback(&PointerButtonCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));//static_cast< void * >( pComponent ) );
		break;
	}

	case KBTYPEINFO_SOUNDWAVE:
	case KBTYPEINFO_ANIMATION:
	case KBTYPEINFO_PTR:
	case KBTYPEINFO_TEXTURE:
	case KBTYPEINFO_STATICMODEL:
	case KBTYPEINFO_SHADER:
	{
		kbResource* const pResource = *((kbResource**)byteOffsetToVar);
		if (pResource != nullptr) {
			Fl_Text_Display* const propertyNameLabel = new Fl_Text_Display(xPos + propertyNamePixelWidth, yPos, 0, inputHeight, pResource->GetName().c_str());
			propertyNameLabel->textsize(FontSize());
			propertyNameLabel->labelsize(FontSize());
			propertyNameLabel->align(FL_ALIGN_RIGHT);
		}

		{
			Fl_Button* const b1 = new Fl_Button(xPos + propertyNamePixelWidth - 2 * (5 + inputHeight / 2), yPos + (int)(inputHeight * 0.25f), FontSize(), FontSize(), "-");
			b1->color(89);
			b1->labelsize((int)(FontSize() * 0.75f));

			cbData.m_pResource = (const kbResource**)byteOffsetToVar;
			cbData.m_VariableType = propertyType;
			cbData.m_pVariablePtr = const_cast<void*>((void*)&propertyName);

			m_CallBackData.push_back(cbData);
			b1->callback(&ClearPointerButtonCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));//static_cast< void * >( pComponent ) );
		}

		{
			Fl_Button* const b1 = new Fl_Button(xPos + propertyNamePixelWidth - (5 + inputHeight / 2), yPos + (int)(inputHeight * 0.25f), FontSize(), FontSize(), ">");
			b1->color(FL_GREEN);
			b1->labelsize((int)(FontSize() * 0.75f));

			cbData.m_pResource = (const kbResource**)byteOffsetToVar;
			cbData.m_VariableType = propertyType;
			cbData.m_pVariablePtr = const_cast<void*>((void*)&propertyName);

			m_CallBackData.push_back(cbData);
			b1->callback(&PointerButtonCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));//static_cast< void * >( pComponent ) );
		}

		break;
	}

	case KBTYPEINFO_VECTOR:
	{
		Vec3& vec = *(Vec3*)byteOffsetToVar;

		Fl_Input* const X_Input = new Fl_Input(xPos + propertyNamePixelWidth, yPos, threeComponentFieldWidth, inputHeight);
		Fl_Input* const Y_Input = new Fl_Input(xPos + propertyNamePixelWidth + threeComponentFieldWidth, yPos, threeComponentFieldWidth, inputHeight);
		Fl_Input* const Z_Input = new Fl_Input(xPos + propertyNamePixelWidth + threeComponentFieldWidth * 2, yPos, threeComponentFieldWidth, inputHeight);

		cbData.m_pVariablePtr = &vec.x;
		m_CallBackData.push_back(cbData);
		X_Input->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));

		cbData.m_pVariablePtr = &vec.y;
		m_CallBackData.push_back(cbData);
		Y_Input->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));

		cbData.m_pVariablePtr = &vec.z;
		m_CallBackData.push_back(cbData);
		Z_Input->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));

		X_Input->textsize(FontSize());
		Y_Input->textsize(FontSize());
		Z_Input->textsize(FontSize());

		X_Input->value(std::to_string(vec.x).c_str());
		Y_Input->value(std::to_string(vec.y).c_str());
		Z_Input->value(std::to_string(vec.z).c_str());

		X_Input->position(0);
		Y_Input->position(0);
		Z_Input->position(0);

		break;
	}

	case KBTYPEINFO_VECTOR4:
	{
		Vec4& color = *(Vec4*)byteOffsetToVar;

		Fl_Input* X_Input = new Fl_Input(xPos + propertyNamePixelWidth, yPos, fourComponentFieldWidth, inputHeight);
		Fl_Input* Y_Input = new Fl_Input(xPos + propertyNamePixelWidth + fourComponentFieldWidth, yPos, fourComponentFieldWidth, inputHeight);
		Fl_Input* Z_Input = new Fl_Input(xPos + propertyNamePixelWidth + fourComponentFieldWidth * 2, yPos, fourComponentFieldWidth, inputHeight);
		Fl_Input* W_Input = new Fl_Input(xPos + propertyNamePixelWidth + fourComponentFieldWidth * 3, yPos, fourComponentFieldWidth, inputHeight);

		cbData.m_pVariablePtr = &color.x;
		m_CallBackData.push_back(cbData);
		X_Input->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));

		cbData.m_pVariablePtr = &color.y;
		m_CallBackData.push_back(cbData);
		Y_Input->callback(&TextFieldCB, static_cast<void*> (&m_CallBackData[m_CallBackData.size() - 1]));

		cbData.m_pVariablePtr = &color.z;
		m_CallBackData.push_back(cbData);
		Z_Input->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));

		cbData.m_pVariablePtr = &color.w;
		m_CallBackData.push_back(cbData);
		W_Input->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));

		X_Input->textsize(FontSize());
		Y_Input->textsize(FontSize());
		Z_Input->textsize(FontSize());
		W_Input->textsize(FontSize());

		X_Input->value(std::to_string(color.x).c_str());
		Y_Input->value(std::to_string(color.y).c_str());
		Z_Input->value(std::to_string(color.z).c_str());
		W_Input->value(std::to_string(color.w).c_str());

		X_Input->position(0);
		Y_Input->position(0);
		Z_Input->position(0);
		W_Input->position(0);

		break;
	}

	case KBTYPEINFO_ENUM:
	{
		const std::vector< std::string >* const enumList = g_NameToTypeInfoMap->GetEnum(structName);
		Fl_Choice* pNewDropDown = new Fl_Choice(xPos + propertyNamePixelWidth, yPos, propertyNamePixelWidth, inputHeight);
		for (int i = 0; i < enumList->size(); i++) {
			pNewDropDown->add((*enumList)[i].c_str());
		}
		pNewDropDown->textsize(FontSize());

		int& enumIntValue = *((int*)byteOffsetToVar);
		pNewDropDown->value(enumIntValue);
		cbData.m_pVariablePtr = &enumIntValue;
		m_CallBackData.push_back(cbData);
		pNewDropDown->callback(&EnumCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
		break;
	}

	case KBTYPEINFO_KBSTRING:
	{
		kbString& string = *(kbString*)byteOffsetToVar;
		cbData.m_pVariablePtr = &string;
		m_CallBackData.push_back(cbData);

		Fl_Input* stringInput = new Fl_Input(xPos + propertyNamePixelWidth, yPos, maxFieldWidth, inputHeight);
		stringInput->callback(&TextFieldCB, static_cast<void*>(&m_CallBackData[m_CallBackData.size() - 1]));
		stringInput->value(string.c_str());
		stringInput->textsize(FontSize());
		break;
	}
	}
}

/**
 *	kbPropertiesTab::DeleteComponent
 */
void kbPropertiesTab::DeleteComponent(Fl_Widget* widget, void* voidptr) {
	propertiesTabCBData_t* const userData = static_cast<propertiesTabCBData_t*>(voidptr);

	kbComponent* const componentToDelete = userData->m_pComponent;
	std::string msg = "Delete ";
	msg += componentToDelete->GetComponentClassName();
	msg += "?";

	if (fl_ask(msg.c_str()) == 0) {
		return;
	}

	int componentIdx = -1;
	kbGameEntity* const pEntity = (kbGameEntity*)componentToDelete->GetOwner();	// ENTITY HACK
	for (componentIdx = 0; componentIdx < pEntity->NumComponents(); componentIdx++) {
		if (pEntity->GetComponent(componentIdx) == componentToDelete) {
			break;
		}
	}

	pEntity->RemoveComponent(componentToDelete);
	//componentToDelete->Enable( false );
	g_Editor->PushUndoAction(new kbUndoDeleteComponent(userData->m_pEditorEntity, componentToDelete, componentIdx));

	g_pPropertiesTab->RefreshEntity();
}

/**
 *	kbPropertiesTab::InsertArrayStruct
 */
void kbPropertiesTab::InsertArrayStruct(Fl_Widget* widget, void* voidPtr) {
	propertiesTabCBData_t* userData = static_cast<propertiesTabCBData_t*>(voidPtr);

	g_NameToTypeInfoMap->InsertVectorElement(userData->m_pArray, userData->m_StructName, userData->m_ArrayIndex);
	g_pPropertiesTab->RefreshEntity();
}

/**
 *	kbPropertiesTab::DeleteArrayStruct
 */
void kbPropertiesTab::DeleteArrayStruct(Fl_Widget* widget, void* voidPtr) {
	propertiesTabCBData_t* userData = static_cast<propertiesTabCBData_t*>(voidPtr);

	g_NameToTypeInfoMap->RemoveVectorElement(userData->m_pArray, userData->m_StructName, userData->m_ArrayIndex);
	g_pPropertiesTab->RefreshEntity();
}