//===================================================================================================
// kbWidgetCBObjects.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBWIDGETCBOBJECTS_H_
#define _KBWIDGETCBOBJECTS_H_

/**
 *	widgetCBType_t
 */
enum widgetCBType_t {
	WidgetCB_None,
	WidgetCB_Input,
	WidgetCB_ResourceSelected,
	WidgetCB_PrefabSelected,
	WidgetCB_EntitySelected,
	WidgetCB_EntityDeselected,
	WidgetCB_TranslationButtonPressed,
	WidgetCB_RotationButtonPressed,
	WidgetCB_ScaleButtonPressed,
	WidgetCB_EntityTransformed,
	WidgetCB_ComponentCreated,
	WidgetCB_GameStarted,
	WidgetCB_GameStopped,
	WidgetCB_PrefabModified,
};


/**
 *	widgetCBObject - base object that is passed to widgets during callbacks.
 */
class widgetCBObject {
public:
	widgetCBObject() :
	  widgetType( WidgetCB_None ) {

	 }

	widgetCBType_t widgetType;
};

/**
 *	widgetCBInputObject
 */
class widgetCBInputObject : public widgetCBObject {
public:
	widgetCBInputObject() {
		widgetType = WidgetCB_Input;
		Clear();
	}

	enum keyType_t {
		WidgetInput_Forward,
		WidgetInput_Back,
		WidgetInput_Left,
		WidgetInput_Right,
		WidgetInput_Shift,
		WidgetInput_Ctrl
	};

	void ClearKeys() {
		keys.clear();
	}

	void Clear() {
		keys.clear();
		mouseX = 0;
		mouseY = 0;
		mouseDeltaX = 0;
		mouseDeltaY = 0;
		leftMouseButtonPressed = 0;
		leftMouseButtonDown = 0;
		rightMouseButtonPressed = 0;
		rightMouseButtonDown = 0;
	}

	std::vector< keyType_t > keys;

	int mouseX;
	int mouseY;
	int mouseDeltaX;
	int mouseDeltaY;

	bool leftMouseButtonPressed;
	bool leftMouseButtonDown;
	bool rightMouseButtonPressed;
	bool rightMouseButtonDown;
};

/**
 *	widgetCBResourceSelected
 */
class widgetCBResourceSelected : public widgetCBObject {
public:
	widgetCBResourceSelected( const widgetCBType_t type ) {
		widgetType = type;
	}

	std::string resourceFileName;
};

/**
 *	widgetCBEntitySelected
 */
class widgetCBEntitySelected : public widgetCBObject {
public:
	widgetCBEntitySelected() {
		widgetType = WidgetCB_EntitySelected;
	}

	std::vector< class kbEditorEntity * > entitiesSelected;
};

/**
 *	widgetCBEntityDeselected
 */
class widgetCBEntityDeselected : public widgetCBObject {
public:
	widgetCBEntityDeselected() {
		widgetType = WidgetCB_EntityDeselected;
	}
};

/**
 *	widgetCBEntityTransformed
 */
class widgetCBEntityTransformed : public widgetCBObject {
public:
	widgetCBEntityTransformed() {
		widgetType = WidgetCB_EntityTransformed;
	}

	std::vector< class kbEditorEntity * > entitiesMoved;
};

/**
 *  widgetCBGeneric
 */
class widgetCBGeneric : public widgetCBObject {
public:
	widgetCBGeneric( const widgetCBType_t type, void * ptr ) {
		widgetType = type;
		m_Value = ptr;
	}
	
	void * m_Value;
};

#endif
