/// kbUndoAction.cpp
///
/// 2016-2025 kbEngine 2.0

#include <vector>
#include "kbCore.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"
#include "kbUndoAction.h"

const int g_UndoStackSize = 15;
extern bool g_bEditorIsUndoingAnAction;

/**
 *	kbUndoStack::kbUndoStack
 */
kbUndoStack::kbUndoStack() {
	Reset();
}

/**
 *	kbUndoStack::GetLastDirtyActionId
 */
UINT64 kbUndoStack::GetLastDirtyActionId() const {
	if (m_StackCurrent < 0) {
		return UINT64_MAX;
	}


	for (int i = 0; i < m_StackLength; i++) {
		int curIdx = m_StackCurrent - i;
		if (curIdx < 0) {
			curIdx += g_UndoStackSize;
		}

		if (m_Stack[curIdx]->MarksMapAsDirty()) {
			return m_Stack[curIdx]->m_UndoActionId;
		}
	}


	return UINT64_MAX;
}

/**
 *	kbUndoStack::Reset
 */
void kbUndoStack::Reset() {
	for (int i = 0; i < m_Stack.size(); i++) {
		if (m_Stack[i] != nullptr) {
			delete m_Stack[i];
			m_Stack[i] = nullptr;
		}
	}
	m_Stack.clear();

	m_StackTop = -1;
	m_StackLength = 0;
	m_StackCurrent = -1;

	m_Stack.resize(g_UndoStackSize);
	m_NextUndoActionId = UINT64_MAX;
	std::vector<kbEditorEntity*> emptyList;
	//	Push( new kbUndoSelectActor( emptyList ) );
}

/**
 *  kbUndoStack::Push
 */
void kbUndoStack::Push(kbUndoAction* const action) {

	m_StackCurrent++;

	if (m_StackCurrent >= g_UndoStackSize) {
		m_StackCurrent = 0;
	}

	if (m_StackLength < g_UndoStackSize) {
		m_StackLength++;
	}

	m_StackTop = m_StackCurrent;

	if (m_Stack[m_StackCurrent] != nullptr) {
		delete m_Stack[m_StackCurrent];
	}

	if (m_NextUndoActionId == UINT64_MAX) {
		m_NextUndoActionId = 0;
	}

	action->m_UndoActionId = m_NextUndoActionId++;
	m_Stack[m_StackCurrent] = action;

	//blk::log( "Push() ------------------------------------------" );
	DumpStack();
}

/**
 *  kbUndoStack::Undo
 */
void kbUndoStack::Undo() {

	if (m_StackLength == 0) {
		fl_message("Undo buffer is empty");
		return;
	}

	g_bEditorIsUndoingAnAction = true;
	m_Stack[m_StackCurrent]->UndoAction();
	m_Stack[m_StackCurrent]->m_bHasBeenRedone = false;
	g_bEditorIsUndoingAnAction = false;

	m_StackLength--;
	m_StackCurrent--;
	if (m_StackCurrent < 0) {
		m_StackCurrent = g_UndoStackSize - 1;
	}

	//blk::log( "Undo() ------------------------------------------" );
	DumpStack();
}

/**
 *	kbUndoStack::Redo
 */
void kbUndoStack::Redo() {

	if (m_StackTop == m_StackCurrent) {
		fl_message("No more actions to redo");
		return;
	}

	m_StackCurrent++;
	m_StackLength++;
	if (m_StackCurrent >= g_UndoStackSize) {
		m_StackCurrent = 0;
	}

	g_bEditorIsUndoingAnAction = true;
	m_Stack[m_StackCurrent]->RedoAction();
	m_Stack[m_StackCurrent]->m_bHasBeenRedone = true;
	g_bEditorIsUndoingAnAction = false;


	blk::log("Redo() ------------------------------------------");
	DumpStack();
}

/**
 *	kbUndoStack::DumpStack
 */
void kbUndoStack::DumpStack() {
	int bottomIdx = m_StackCurrent - m_StackLength;
	if (bottomIdx < 0) {
		bottomIdx += g_UndoStackSize;
	}
	/*
		blk::log( "Stack Len = %d", m_StackLength );
		blk::log( "Stack cur = %d", m_StackCurrent );
		blk::log( "Stack top = %d", m_StackTop );

		for ( int i = g_UndoStackSize - 1; i >= 0; i-- ) {

			char buffer[32] = "";

			if ( i == m_StackTop ) {
				sprintf_s( buffer, "^^^^^^^^^^" );
			} else if ( i == bottomIdx ) {
				sprintf_s( buffer, "__________" );
			}

			if ( i == m_StackCurrent ) {
				sprintf_s( buffer, "%s <------------", buffer );
			}

			kbUndoAction *const undoAction = m_Stack[i];

			if ( dynamic_cast<kbUndoSelectActor*>( undoAction ) != NULL ) {
				blk::log( "%d: Undoing to Select %d actors.	%s",  i, ((kbUndoSelectActor*) undoAction)->NumSelected(), buffer);
			} else if ( dynamic_cast<kbUndoDeleteActor*>( undoAction ) != NULL ) {
				blk::log( "%d: Undoing delete %d actors	%s",  i, ((kbUndoDeleteActor*)undoAction)->NumDeleted(), buffer);
			} else if ( dynamic_cast<kbUndoDeleteComponent*>( undoAction ) != NULL ) {
				blk::log( "%d: Undoing delete component	%s", g_UndoStackSize - 1 - i, buffer );
			} else {
				blk::log( "%d: Undoing		%s", i, buffer );
			}
		}*/
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	kbUndoVariableAction
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

kbUndoVariableAction::kbUndoVariableAction(kbTypeInfoType_t type, void* bytePtrToUndoVar, void* bytePtrToRedoVar, void* pVariable) {

	m_pVariable = pVariable;
	m_VarType = type;

	switch (type) {
	case KBTYPEINFO_BOOL:
	{
		m_UndoBoolean = *(bool*)(bytePtrToUndoVar);
		m_RedoBoolean = *(bool*)(bytePtrToRedoVar);
		break;
	}

	case KBTYPEINFO_INT:
	case KBTYPEINFO_ENUM:
	{
		m_UndoInt = *(const int*)(bytePtrToUndoVar);
		m_RedoInt = *(const int*)(bytePtrToRedoVar);
		break;
	}

	case KBTYPEINFO_VECTOR4:
	case KBTYPEINFO_VECTOR:
	case KBTYPEINFO_FLOAT:
	{
		m_UndoFloat = *(float*)(bytePtrToUndoVar);
		m_RedoFloat = *(float*)(bytePtrToRedoVar);
		break;
	}

	case KBTYPEINFO_KBSTRING:
	{
		m_UndoString = *(kbString*)(bytePtrToUndoVar);
		m_RedoString = *(kbString*)(bytePtrToRedoVar);
		break;
	}

	case KBTYPEINFO_PTR:
	case KBTYPEINFO_STATICMODEL:
	case KBTYPEINFO_SHADER:
	case KBTYPEINFO_ANIMATION:
	{
		m_pUndoPtr = (void*)(bytePtrToUndoVar);
		m_pRedoPtr = (void*)(bytePtrToRedoVar);
		break;
	}
	}
}

/**
 *	kbUndoVariableAction::UndoAction
 */
void kbUndoVariableAction::UndoAction() {

	switch (m_VarType) {
	case KBTYPEINFO_BOOL:
	{

		break;
	}

	case KBTYPEINFO_INT:
	case KBTYPEINFO_ENUM:
	{

		break;
	}

	case KBTYPEINFO_VECTOR4:
	case KBTYPEINFO_VECTOR:
	case KBTYPEINFO_FLOAT:
	{
		float& floatVar = *(float*)m_pVariable;
		floatVar = m_UndoFloat;
		break;
	}

	case KBTYPEINFO_KBSTRING:
	{
		kbString& stringVar = *(kbString*)m_pVariable;
		stringVar = m_UndoString;
		break;
	}

	case KBTYPEINFO_PTR:
	case KBTYPEINFO_STATICMODEL:
	case KBTYPEINFO_SHADER:
	case KBTYPEINFO_ANIMATION:
	{

		break;
	}
	}
}


/**
 *	kbUndoVariableAction::RedoAction
 */
void kbUndoVariableAction::RedoAction() {
	switch (m_VarType) {
	case KBTYPEINFO_BOOL:
	{

		break;
	}

	case KBTYPEINFO_INT:
	case KBTYPEINFO_ENUM:
	{

		break;
	}

	case KBTYPEINFO_VECTOR4:
	case KBTYPEINFO_VECTOR:
	case KBTYPEINFO_FLOAT:
	{
		float& floatVar = *(float*)m_pVariable;
		floatVar = m_RedoFloat;
		break;
	}

	case KBTYPEINFO_KBSTRING:
	{
		kbString& stringVar = *(kbString*)m_pVariable;
		stringVar = m_RedoString;
		break;
	}

	case KBTYPEINFO_PTR:
	case KBTYPEINFO_STATICMODEL:
	case KBTYPEINFO_SHADER:
	case KBTYPEINFO_ANIMATION:
	{

		break;
	}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	kbUndoDeleteComponent
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *	kbUndoDeleteComponent::kbUndoDeleteComponent
 */
kbUndoDeleteComponent::kbUndoDeleteComponent(kbEditorEntity* const pEntity, kbComponent* const pComponentToDelete, int indexIntoComponentList) :
	m_pEditorEntity(pEntity),
	m_pComponent(pComponentToDelete),
	m_IndexIntoComponentList(indexIntoComponentList) {
}

/**
 *	kbUndoDeleteComponent::Cleanup
 */
void kbUndoDeleteComponent::Cleanup() {
	delete m_pComponent;
}

/**
 *	kbUndoDeleteComponent::UndoAction
 */
void kbUndoDeleteComponent::UndoAction() {
	m_pEditorEntity->GetGameEntity()->AddComponent(m_pComponent, m_IndexIntoComponentList);

	std::vector<kbEditorEntity*> entityList;
	entityList.push_back(m_pEditorEntity);

	g_Editor->DeselectEntities();
	g_Editor->SelectEntities(entityList, false);
}

/**
 *	kbUndoDeleteComponent::RedoAction
 */
void kbUndoDeleteComponent::RedoAction() {

	int componentIdx = -1;
	kbGameEntity* const pEntity = (kbGameEntity*)m_pComponent->GetOwner();	// ENTITY HACK
	for (componentIdx = 0; componentIdx < pEntity->NumComponents(); componentIdx++) {
		if (pEntity->GetComponent(componentIdx) == m_pComponent) {
			break;
		}
	}

	pEntity->RemoveComponent(m_pComponent);

	std::vector<kbEditorEntity*> entityList;
	entityList.push_back(m_pEditorEntity);

	g_Editor->SelectEntities(entityList, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	kbUndoDeleteActor
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *	kbUndoDeleteActor::kbUndoDeleteActor
 */
kbUndoDeleteActor::kbUndoDeleteActor(std::vector< DeletedActorInfo_t >& entitiesToDelete) :
	m_pEntitiesToDelete(entitiesToDelete) {

}

/**
 *	kbUndoDeleteActor::Cleanup
 */
void kbUndoDeleteActor::Cleanup() {
	for (int i = 0; i < m_pEntitiesToDelete.size(); i++) {
		delete m_pEntitiesToDelete[i].m_pEditorEntity;
	}
}

/**
 *	kbUndoDeleteActor::UndoAction
 */
void kbUndoDeleteActor::UndoAction() {

	std::vector<kbEditorEntity*> entityList;

	for (int i = 0; i < m_pEntitiesToDelete.size(); i++) {
		kbEditorEntity* const pEntity = m_pEntitiesToDelete[i].m_pEditorEntity;

		for (int j = 0; j < pEntity->GetGameEntity()->NumComponents(); j++) {
			if (m_pEntitiesToDelete[i].m_bComponentEnabled[j]) {
				pEntity->GetGameEntity()->GetComponent(j)->Enable(true);
			}
		}
		g_Editor->AddEntity(pEntity);
		entityList.push_back(pEntity);
	}
}

/**
 *	kbUndoDeleteActor::RedoAction
 */
void kbUndoDeleteActor::RedoAction() {

	for (int i = 0; i < m_pEntitiesToDelete.size(); i++) {
		kbEditorEntity* const pEntity = m_pEntitiesToDelete[i].m_pEditorEntity;
		g_Editor->GetGameEntities().erase(std::remove(g_Editor->GetGameEntities().begin(), g_Editor->GetGameEntities().end(), pEntity), g_Editor->GetGameEntities().end());
		for (int j = 0; j < pEntity->GetGameEntity()->NumComponents(); j++) {
			pEntity->GetGameEntity()->GetComponent(j)->Enable(true);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	kbUndoSelectActor
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *	kbUndoSelectActor::kbUndoSelectActor
 */
kbUndoSelectActor::kbUndoSelectActor(std::vector< kbEditorEntity* >& undoEntities, std::vector<kbEditorEntity*>& redoEntities) :
	m_UndoSelectedEntities(undoEntities),
	m_RedoSelectedEntities(redoEntities) {
}

/**
 *	kbUndoSelectActor::UndoAction
 */
void kbUndoSelectActor::UndoAction() {

	g_Editor->SelectEntities(m_UndoSelectedEntities, false);
}

/**
 *	kbUndoDeleteActor::RedoAction
 */
void kbUndoSelectActor::RedoAction() {

	g_Editor->DeselectEntities();
	g_Editor->SelectEntities(m_RedoSelectedEntities, false);
}

