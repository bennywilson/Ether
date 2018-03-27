//===================================================================================================
// kbPropertiesTab.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBPROPERTIESTAB_H_
#define _KBPROPERTIESTAB_H_


struct propertiesTabCBData_t
{
	propertiesTabCBData_t() :
		pEditorEntity( NULL ),
		pComponent( NULL ),
		pResource( NULL ),
		pField( NULL ),
		fieldType( KBTYPEINFO_NONE ),
		pArray( NULL ),
		arrayIndex( -1 ) { }

	kbEditorEntity *	pEditorEntity;
	kbComponent *		pComponent;
	kbResource **		pResource;
	void *				pField;
	void *				pVar;
	kbTypeInfoType_t	fieldType;
	std::string			structName;
	void *				pArray;
	int					arrayIndex;
};

/*
 *	kbPropertiesTab
 */
class kbPropertiesTab : public Fl_Tabs, kbWidget {
public:

						kbPropertiesTab( int x, int y, int w, int h );

	virtual void		EventCB( const widgetCBObject * widgetCBObject );

	virtual void		Update();

	void				RequestRefreshNextUpdate() { m_bRefreshNextUpdate = true; }

	std::vector< class kbEditorEntity * > & GetSelectedEntities() { return m_SelectedEntities; }
	kbEditorEntity *	GetTempPrefabEntity() { return m_pTempPrefabEntity; }

private:

	void				RefreshEntity();
	void				RefreshComponent( kbEditorEntity *const pEntity, kbComponent *const pComponent, int & startX, int & curY, const int inputHeight, const bool bIsStruct = false, void * arrayPtr = NULL, const int arrayIndex = -1);
	void				RefreshProperty( kbEditorEntity *const pEntity, const std::string & propertyName, const kbTypeInfoType_t propertyType, const std::string & structName, kbComponent * pComponent, byte * byteOffsetToVar, int & x, int & y, const int inputHeight, void * arrayPtr = NULL, const int arrayIndex = -1 );

	unsigned int		FontSize()	const { return 13; }
	unsigned int		LineSpacing() const { return FontSize() + 5; }

	Fl_Tabs *			m_pPropertiesTab;
	Fl_Group *			m_pEntityProperties;
	Fl_Group *			m_pResourceProperties;

	static void			CheckButtonCB( Fl_Widget * widget, void * voidPtr );
	static void			PointerButtonCB( Fl_Widget * widget, void * voidPtr );
	static void			TextFieldCB( Fl_Widget * widget, void * voidPtr );
	static void			ArrayExpandCB( Fl_Widget * widet, void * voidPtr );
	static void			ArrayResizeCB( Fl_Widget * widget, void * voidPtr );
	static void			EnumCB( Fl_Widget * widget, void * voidPtr );
	static void			DeleteComponent( Fl_Widget * widget, void * voidPtr );
	static void			InsertArrayStruct( Fl_Widget * widget, void * voidPtr );
	static void			DeleteArrayStruct( Fl_Widget * widget, void * voidPtr );
	static void			PropertyChangedCB();		// Each call back should this before returning

	std::vector< class kbEditorEntity * >	m_SelectedEntities;
	std::string								m_CurrentlySelectedResource;
	std::vector< propertiesTabCBData_t >	m_CallBackData;

	kbEditorEntity *						m_pTempPrefabEntity;
	bool									m_bRefreshNextUpdate;
};

extern kbPropertiesTab * g_pPropertiesTab;

#endif
