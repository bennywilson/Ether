//===================================================================================================
// kbPropertiesTab.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBPROPERTIESTAB_H_
#define _KBPROPERTIESTAB_H_

class kbEditorEntity;

struct propertiesTabCBData_t {

	propertiesTabCBData_t(	kbEditorEntity *const pEditorEntity,
							const kbGameEntityPtr *const inGameEntityPtr,
							kbComponent *const pComponent,
                            kbComponent *const pParentComponent,
							const kbResource ** pResource,
							const  kbString variableName,
							void *const pVariableValue,
							const kbTypeInfoType_t variableType,
							const std::string & structName,
							const void *const pArray,
							const int arrayIdx = -1 );

	kbEditorEntity *		m_pEditorEntity;
	kbGameEntityPtr			m_GameEntityPtr;
	kbComponent *			m_pComponent;
    kbComponent *           m_pParentComponent;
	const kbResource **		m_pResource;
	kbString				m_VariableName;
	void *					m_pVariablePtr;
	kbTypeInfoType_t		m_VariableType;
	std::string				m_StructName;
	const void *			m_pArray;
	int						m_ArrayIndex;
};

/**
 *	kbPropertiesTab
 */
class kbPropertiesTab : public Fl_Tabs, kbWidget {

//---------------------------------------------------------------------------------------------------
public:
												kbPropertiesTab( int x, int y, int w, int h );

	virtual void								EventCB( const widgetCBObject *const widgetCBObject ) override;

	virtual void								Update() override;

	void										RequestRefreshNextUpdate() { m_bRefreshNextUpdate = true; }

	std::vector<kbEditorEntity *> &				GetSelectedEntities() { return m_SelectedEntities; }
	kbEditorEntity *							GetTempPrefabEntity() { return m_pTempPrefabEntity; }

private:

	void										RefreshEntity();
	void										RefreshComponent( kbEditorEntity *const pEntity, kbComponent *const pComponent, kbComponent *const pParentComponent, int & startX, int & curY, const int inputHeight, const bool bIsStruct = false, const void *const arrayPtr = nullptr, const int arrayIndex = -1);
	void										RefreshProperty( kbEditorEntity *const pEntity, const std::string & propertyName, const kbTypeInfoType_t propertyType, const std::string & structName, kbComponent *const pComponent, const byte *const byteOffsetToVar,  kbComponent *const pParentComponent, int & x, int & y, const int inputHeight, const void *const arrayPtr = nullptr, const int arrayIndex = -1 );

	unsigned int								FontSize()	const { return 10; }
	unsigned int								LineSpacing() const { return FontSize() + 5; }

	Fl_Tabs *									m_pPropertiesTab;
	Fl_Group *									m_pEntityProperties;
	Fl_Group *									m_pResourceProperties;

	std::vector<kbEditorEntity *>				m_SelectedEntities;
	std::string									m_CurrentlySelectedResource;
	std::vector<propertiesTabCBData_t>			m_CallBackData;

	kbEditorEntity *							m_pTempPrefabEntity;
	bool										m_bRefreshNextUpdate;

	static void									CheckButtonCB( Fl_Widget * widget, void * voidPtr );
	static void									PointerButtonCB( Fl_Widget * widget, void * voidPtr );
	static void									ClearPointerButtonCB( Fl_Widget * widget, void * voidPtr );
	static void									TextFieldCB( Fl_Widget * widget, void * voidPtr );
	static void									ArrayExpandCB( Fl_Widget * widet, void * voidPtr );
	static void									ArrayResizeCB( Fl_Widget * widget, void * voidPtr );
	static void									EnumCB( Fl_Widget * widget, void * voidPtr );
	static void									DeleteComponent( Fl_Widget * widget, void * voidPtr );
	static void									InsertArrayStruct( Fl_Widget * widget, void * voidPtr );
	static void									DeleteArrayStruct( Fl_Widget * widget, void * voidPtr );
	static void									PropertyChangedCB( const kbGameEntityPtr gameEntityPtr );		// Each call back should this before returning
};

extern kbPropertiesTab * g_pPropertiesTab;

#endif
