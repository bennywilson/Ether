//==============================================================================
// kbPropertiesTab.cpp
//
// 2016-2017 kbEngine 2.0
//==============================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbWidget.h"
#include "kbResourceTab.h"
#include "kbEditor.h"
#include "kbGameEntityHeader.h"
#include "kbEditorEntity.h"
#include "kbPropertiesTab.h"

#pragma warning(push)
#pragma warning(disable:4312)
#include "FL/FL_Input.h"
#include "FL/FL_Draw.h"
#include "FL/FL_Scroll.h"
#pragma warning(pop)

kbPropertiesTab * g_pPropertiesTab = nullptr;

/**
 *	OnEntityTransformedChanged
 */
void OnEntityTransformedChanged( void * param ) {
	widgetCBEntityTransformed newCB;
	newCB.entitiesMoved.push_back( static_cast< kbEditorEntity * >( param ) );

	g_Editor->BroadcastEvent( newCB );
}

/**
 *	kbPropertiesTab
 */
kbPropertiesTab::kbPropertiesTab( int widgetX, int widgetY, int widgetWidth, int widgetHeight ) :
	kbWidget( widgetX, widgetY, widgetWidth, widgetHeight ),
	Fl_Tabs( widgetX, widgetY, widgetWidth, widgetHeight ),
	m_pTempPrefabEntity( NULL ) {

	m_pPropertiesTab  = new Fl_Tabs( x(), y(), widgetWidth, h() );
    
	m_pEntityProperties = new Fl_Group( x() + kbEditor::PanelBorderSize(), y() + kbEditor::TabHeight(), widgetWidth, widgetHeight, "Entity Info" );
	m_pEntityProperties->end();

	m_pResourceProperties = new Fl_Group( 0, y() + kbEditor::TabHeight(), DisplayWidth(), widgetHeight, "Resource Info" );
	m_pResourceProperties->end();

	m_pPropertiesTab->end();

	// register events
	g_Editor->RegisterUpdate( this );
	g_Editor->RegisterEvent( this, WidgetCB_EntitySelected );
	g_Editor->RegisterEvent( this, WidgetCB_PrefabSelected );
	g_Editor->RegisterEvent( this, WidgetCB_EntityDeselected );
	g_Editor->RegisterEvent( this, WidgetCB_ComponentCreated );
	g_Editor->RegisterEvent( this, WidgetCB_ResourceSelected );

	g_pPropertiesTab = this;

	//m_CallBackData.reserve( 256 );
}

/**
 *	kbPropertiesTab::Update
 */
void kbPropertiesTab::Update() {
	kbWidget::Update();

	if (m_bRefreshNextUpdate)
	{
		RefreshEntity();
		m_bRefreshNextUpdate = false;
	}
}

/**
 *	kbPropertiesTab::EventCB
 */
void kbPropertiesTab::EventCB( const widgetCBObject * widgetCBObject ) {
	
	switch( widgetCBObject->widgetType ) {
		
		case WidgetCB_EntitySelected : {

			if ( m_pTempPrefabEntity != NULL ) {
				m_pTempPrefabEntity->SetGameEntity( NULL );
				delete m_pTempPrefabEntity;
				m_pTempPrefabEntity = NULL;
			}

			widgetCBEntitySelected * pEntitySelectedCB = ( widgetCBEntitySelected * ) widgetCBObject;
			m_SelectedEntities = g_Editor->GetSelectedObjects();//pEntitySelectedCB->entitiesSelected;
			RefreshEntity();
		}
		break;

		case WidgetCB_EntityDeselected : {
			if ( m_pTempPrefabEntity != NULL ) {
				m_pTempPrefabEntity->SetGameEntity( NULL );
				delete m_pTempPrefabEntity;
				m_pTempPrefabEntity = NULL;
			}

			m_SelectedEntities.clear();
			RefreshEntity();
		}
		break;

		case WidgetCB_ComponentCreated : {
			RefreshEntity();
		}
		break;

		case WidgetCB_ResourceSelected : {
			m_CurrentlySelectedResource = static_cast< const widgetCBResourceSelected * >( widgetCBObject )->resourceFileName;
		}
		break;

		case WidgetCB_PrefabSelected : {
			m_SelectedEntities.clear();
			m_pTempPrefabEntity = new kbEditorEntity( const_cast<kbGameEntity*>( g_Editor->GetCurrentlySelectedPrefab()->GetGameEntity(0) ) );
			RefreshEntity();
		}
		break;
	}
}

/**
 *	kbPropertiesTab::CheckButtonCB
 */
void kbPropertiesTab::CheckButtonCB( Fl_Widget * widget, void * voidPtr ) {
	PropertyChangedCB();

	if ( voidPtr == NULL ) {
		// warn or something
		return;
	}

	Fl_Check_Button * pCheckButton = static_cast< Fl_Check_Button * >( widget );
	propertiesTabCBData_t * userData = static_cast< propertiesTabCBData_t * >( voidPtr );
	bool & componentVar = *(bool*)userData->pField;
	char buttonVal = pCheckButton->value();

	const bool wasEnabled = userData->pComponent->IsEnabled();
	componentVar = !componentVar;
	if ( wasEnabled != userData->pComponent->IsEnabled() ) {
		componentVar = !componentVar;
		userData->pComponent->Enable( buttonVal > 0 );
		return;
	}

	if ( buttonVal > 0 ) {
		componentVar = true;
	} else {
		componentVar = false;
	}

	userData->pComponent->Enable( false );
	if ( wasEnabled ) {
		userData->pComponent->Enable( true );
	}
}

/**
 *	kbPropertiesTab::PointerButtonCB
 */
void kbPropertiesTab::PointerButtonCB( Fl_Widget * widget, void * voidPtr ) {
	PropertyChangedCB();

	if ( voidPtr == NULL ) {
		// warn or something
		return;
	}

	propertiesTabCBData_t *const userData = static_cast< propertiesTabCBData_t * >( voidPtr );
	const std::string *const fieldName = ( std::string * ) userData->pField;
	if ( userData->fieldType == KBTYPEINFO_GAMEENTITY ) {
		const kbPrefab *const pPrefab = g_Editor->GetCurrentlySelectedPrefab();
		kbGameEntityPtr & pEntityPtr = *( kbGameEntityPtr * ) userData->pVar;

		if ( pPrefab == NULL ) {
			if ( g_Editor->GetSelectedObjects().size() > 0 ) {
				pEntityPtr.SetEntity( g_Editor->GetSelectedObjects()[0]->GetGameEntity() );
			} else {
				pEntityPtr.SetEntity( NULL );
			}
		} else {
			pEntityPtr.SetEntity( const_cast<kbGameEntity*>( pPrefab->GetGameEntity(0) ) );
		}

		userData->pComponent->EditorChange( *fieldName );
		g_pPropertiesTab->RefreshEntity();
		return;
	}

	if ( g_pPropertiesTab->m_CurrentlySelectedResource.empty() ) {
		return;
	}

	kbResource * pResource = (kbResource*)g_ResourceManager.GetResource( g_pPropertiesTab->m_CurrentlySelectedResource.c_str() , true );

	// Don't do anything if the selected resource is not the right type, or if it's the same resource that's already present
	if ( pResource->GetType() != userData->fieldType || *userData->pResource == pResource )
		return;

	*userData->pResource = pResource;

	userData->pComponent->EditorChange( *fieldName );

	g_pPropertiesTab->RefreshEntity();
}

bool IsNumeric( const char *const cString ) {
	const int fieldTextLen = (int)strlen( cString );
	bool bHasDecimalSpot = false;

	for ( int i = 0; i < fieldTextLen; i++ ) {
		if ( cString[i] == '.' ) {
			if ( bHasDecimalSpot ) {
				return false;
			}
			bHasDecimalSpot = true;
			continue;
		} else if ( cString[i] == 'f' ) {
			if ( i < fieldTextLen - 1 ) {
				return false;
			}
		}

		if ( i == 0 && cString[i] == '-' && fieldTextLen > 1 ) {
			continue;
		}

		if ( cString[i] < '0' || cString[i] > '9' ) {
			g_pPropertiesTab->RequestRefreshNextUpdate();
			return false;
		}
	}

	return true;
}

/**
 *	kbPropertiesTab::TextFieldCB
 */
void kbPropertiesTab::TextFieldCB( Fl_Widget * widget, void * voidPtr ) {
	PropertyChangedCB();

	if ( voidPtr == NULL ) {
		// warn or something
		return;
	}

	propertiesTabCBData_t * userData = static_cast< propertiesTabCBData_t * >( voidPtr );
	Fl_Input * inputField = ( Fl_Input * ) widget;

	if ( userData->fieldType != KBTYPEINFO_KBSTRING )
	{
		if ( IsNumeric( inputField->value() ) == false ) {
	      return;
	   }
	}

	void * prevValuePtr = NULL;
	void * curValuePtr = NULL;

	const std::string currentValue = inputField->value();
	inputField->undo();
	const std::string prevValue = inputField->value();
	inputField->value( currentValue.c_str() );

	if ( userData->fieldType == KBTYPEINFO_VECTOR4 || userData->fieldType == KBTYPEINFO_VECTOR ) {
		float & componentVar = *(float*)userData->pField;
		prevValuePtr = new float( (float)atof( prevValue.c_str() ) );
		curValuePtr = new float( (float)atof( currentValue.c_str() ) );

		componentVar = *(float*)curValuePtr;
		
		if ( userData->pComponent->IsEnabled() )
		{
			userData->pComponent->Enable( false );
			userData->pComponent->Enable( true );
		}
	} else if ( userData->fieldType == KBTYPEINFO_INT ) {
		int & componentVar = *( int * ) userData->pField;
		prevValuePtr = new int( (int)atoi( prevValue.c_str() ) );
		curValuePtr = new int( (int)atoi( currentValue.c_str() ) );

		componentVar = ( int ) atoi( inputField->value() );

		if ( userData->pComponent->IsEnabled() ) {
			userData->pComponent->Enable( false );
			userData->pComponent->Enable( true );
		}
	} else if ( userData->fieldType == KBTYPEINFO_FLOAT ) {
		float & componentVar = *(float*)userData->pField;
		prevValuePtr = new float( (float)atof( prevValue.c_str() ) );
		curValuePtr = new float( (float)atof( currentValue.c_str() ) );

		componentVar = *(float*)curValuePtr;

		if ( userData->pComponent->IsEnabled() )
		{
			userData->pComponent->Enable( false );
			userData->pComponent->Enable( true );
		}
	} else if ( userData->fieldType == KBTYPEINFO_KBSTRING ) {
		kbString & curString = *(kbString*)userData->pField;
		curString = inputField->value();

		prevValuePtr = new kbString( prevValue.c_str() );
		curValuePtr = new kbString( currentValue.c_str() );
	}

	g_Editor->PushUndoAction( new kbUndoVariableAction( userData->fieldType, prevValuePtr, curValuePtr, userData->pField ) );

	delete prevValuePtr;
	delete curValuePtr;
}

/**
 *	kbPropertiesTab::ArrayExpandCB
 */
void kbPropertiesTab::ArrayExpandCB( Fl_Widget * widet, void * voidPtr ) {
	PropertyChangedCB();

	if ( voidPtr == NULL ) {
		return;
	}

	varMetaData_t * entry = static_cast< varMetaData_t * >( voidPtr );
	entry->bExpanded = !entry->bExpanded;

	g_pPropertiesTab->RefreshEntity();
}

/**
 *	kbPropertiesTab::ArrayResizeCB
 */
void kbPropertiesTab::ArrayResizeCB( Fl_Widget * widget, void * voidPtr ) {
	PropertyChangedCB();

	if ( voidPtr == NULL ) {
		return;
	}

	propertiesTabCBData_t * userData = static_cast< propertiesTabCBData_t * >( voidPtr );

	const Fl_Input *const inputField = ( Fl_Input * ) widget;
	const char *const inputText = inputField->value();

	if ( IsNumeric( inputText ) == false ) {
		return;
	}
	
	const int fieldValue = atoi( inputText );

	if ( fieldValue < 0 || fieldValue > 32 ) {
		kbWarning( "Array value is not between 0 and 32" );
		g_pPropertiesTab->RequestRefreshNextUpdate();
		return;
	}

	switch( userData->fieldType ) {
		case KBTYPEINFO_SHADER : {
			std::vector< class kbShader * >	* shaderList = ( std::vector< class kbShader * > *)( userData->pField );
			shaderList->resize( fieldValue ); 
			break;
		}

		default : {
			g_NameToTypeInfoMap->ResizeVector( userData->pField, userData->structName, fieldValue );
			break;
		 }
	}

	if ( userData->pComponent->IsEnabled() )
	{
		userData->pComponent->Enable( false );
		userData->pComponent->Enable( true );
	}

	g_pPropertiesTab->RequestRefreshNextUpdate();
}

/**
 *	kbPropertiesTab::EnumCB
 */
void kbPropertiesTab::EnumCB( Fl_Widget * widget, void * voidPtr ) {
	PropertyChangedCB();

	if ( voidPtr == NULL ) {
		return;
	}

	propertiesTabCBData_t * userData = static_cast< propertiesTabCBData_t * >( voidPtr );

	Fl_Choice * pDropDown = ( Fl_Choice * ) widget;
	int dropDownValue = pDropDown->value();

	int & componentVar = *(int*)userData->pField;
	componentVar = dropDownValue;

	userData->pComponent->Enable( false );
	userData->pComponent->Enable( true );

	g_pPropertiesTab->RequestRefreshNextUpdate();
}

/**
 *	kbPropertiesTab::PropertyChangedCB
 */
void kbPropertiesTab::PropertyChangedCB() {
	if ( g_pPropertiesTab->m_pTempPrefabEntity != NULL ) {
		g_Editor->BroadcastEvent( widgetCBGeneric( WidgetCB_PrefabModified, g_pPropertiesTab->m_pTempPrefabEntity->GetGameEntity() ) );		
	}
}

/**
 *	kbPropertiesTab::RefreshComponent
 */
void kbPropertiesTab::RefreshComponent( kbEditorEntity *const pEntity, kbComponent *const pComponent, int & startX, int & curY, const int inputHeight, const bool bIsStruct, void * pArrayPtr, const int arrayIndex ) {
	const kbGameEntity *const pGameEntity = pEntity->GetGameEntity();

	byte *const componentBytePtr = ( byte* ) pComponent;

	kbTypeInfoHierarchyIterator iterator( pComponent );

	// Display Component class name ( kbStaticMeshComponent, kbSkeletalMeshComponent, etc );
	const char *const pComponentName = pComponent->GetComponentClassName();

	if ( bIsStruct == false ) {
		Fl_Text_Display *const propertyNameLabel = new Fl_Text_Display( startX + 10, curY, 0, inputHeight, pComponentName );
		propertyNameLabel->labelsize( FontSize() );
		propertyNameLabel->labelfont( FL_BOLD );
		propertyNameLabel->align( FL_ALIGN_RIGHT );
	}

	// Delete button
	if ( pComponent->IsA( kbTransformComponent::GetType() ) == false ) {
		if ( bIsStruct == false ) {
			Fl_Button *const DeleteButton = new Fl_Button( startX +  DisplayWidth() - FontSize() - Fl::scrollbar_size(), curY + FontSize() / 2, inputHeight/2,inputHeight/2,"X");
			DeleteButton->color(88+1);

			propertiesTabCBData_t cbData;
			cbData.pComponent = pComponent;
			cbData.pEditorEntity = pEntity;
			m_CallBackData.push_back( cbData );

			DeleteButton->callback( DeleteComponent, static_cast<void *>( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );
		} else {
			Fl_Button *const InsertButton = new Fl_Button( startX +  DisplayWidth() - FontSize() - Fl::scrollbar_size() - 16, curY + FontSize() / 2, inputHeight/2,inputHeight/2,"+");
			InsertButton->color(0x00ff00ff);

			Fl_Button *const DeleteButton = new Fl_Button( startX +  DisplayWidth() - FontSize() - Fl::scrollbar_size(), curY + FontSize() / 2, inputHeight/2,inputHeight/2,"-");
			DeleteButton->color(88+1);

			propertiesTabCBData_t cbData;
			cbData.pEditorEntity = pEntity;
			cbData.pArray = pArrayPtr;
			cbData.structName = pComponentName;
			cbData.arrayIndex = arrayIndex;
			m_CallBackData.push_back( cbData );
			InsertButton->callback( InsertArrayStruct, static_cast<void *>( &m_CallBackData[ m_CallBackData.size() - 1 ] )  );
			DeleteButton->callback( DeleteArrayStruct, static_cast<void *>( &m_CallBackData[ m_CallBackData.size() - 1 ] )  );
		}
	}

	curY += LineSpacing();

	// Collect the members and sort them based on offset
	std::vector< kbTypeInfoHierarchyIterator::iteratorType > membersList;
	for ( auto pNextField = iterator.Begin(); iterator.IsDone() == false; pNextField = iterator.GetNextTypeInfoField() )
	{
		membersList.push_back( pNextField );
	}

	std::sort( membersList.begin(), membersList.end(), [](kbTypeInfoHierarchyIterator::iteratorType a, kbTypeInfoHierarchyIterator::iteratorType b ) {
		return a->second.Offset() < b->second.Offset();
	});

	// Iterate over the component's properties and display them
	for ( size_t j = 0; j < membersList.size(); j++ ) {

		auto pNextField = membersList[j];
		const char * varName = pNextField->first.c_str();

		if ( bIsStruct ) {
			if ( pNextField->first == "Enabled" ) {
				curY -= 2 * LineSpacing();
				continue;
			}
		}
		Fl_Text_Display * propertyNameLabel = new Fl_Text_Display( startX + 10, curY, 0, inputHeight, varName );
		propertyNameLabel->labelsize( FontSize() );
		propertyNameLabel->align( FL_ALIGN_RIGHT );

		byte * byteOffsetToVar = componentBytePtr + pNextField->second.Offset();

		if ( pNextField->second.IsArray() ) {

			varMetaData_t * propertyMetaData = pEntity->GetPropertyMetaData(pComponent, pNextField->second.Offset());

			if ( propertyMetaData == NULL )
				continue;

			// Expand / collapse button
			Fl_Button *b1 = NULL;
			if ( propertyMetaData->bExpanded == false ) {
				b1 = new Fl_Button( startX, curY + FontSize() / 2, inputHeight/2,inputHeight/2,"+");
				b1->color(0x00ff00ff);
				b1->callback( &ArrayExpandCB, static_cast< void * >( propertyMetaData ) );
			} else {
				b1 = new Fl_Button( startX, curY + FontSize() / 2, inputHeight/2,inputHeight/2,"-");
				b1->color(0xff0000ff);
				b1->callback( &ArrayExpandCB, static_cast< void * >( propertyMetaData ) );
			}

			const std::string propertyNameWithPadding = "LONGEST STRING";
			const int propertyNamePixelWidth = ( int )fl_width( propertyNameWithPadding.c_str() );

			Fl_Input * pArraySizeInput = new Fl_Input( startX + propertyNamePixelWidth, curY, FontSize() * 3, inputHeight );
			curY += LineSpacing();

			propertiesTabCBData_t cbData;
			cbData.pComponent = pComponent;
			cbData.fieldType = pNextField->second.Type();
			cbData.pField = ( void * ) byteOffsetToVar;
			cbData.structName = pNextField->second.GetStructName();

			m_CallBackData.push_back( cbData );

			pArraySizeInput->callback( &ArrayResizeCB, static_cast<void *>( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			switch( pNextField->second.Type() ) {

				case KBTYPEINFO_SHADER : {
					std::vector< class kbShader * >	* shaderList = ( std::vector< class kbShader * > *)( byteOffsetToVar );

					pArraySizeInput->value( std::to_string( shaderList->size()).c_str() );
					m_pEntityProperties->add( pArraySizeInput );

					if ( propertyMetaData && propertyMetaData->bExpanded ) {
						for ( int i = 0; i < shaderList->size(); i++ ) {
							RefreshProperty( pEntity, pNextField->first, pNextField->second.Type(), pNextField->second.GetStructName(), pComponent, (byte*)&(*shaderList)[i], startX, curY, inputHeight );
							curY += LineSpacing();
						}
					}
					break;
				}

				default:
					const size_t vectorSize = g_NameToTypeInfoMap->GetVectorSize( byteOffsetToVar, pNextField->second.GetStructName() );
					pArraySizeInput->value( std::to_string( vectorSize).c_str() );

					static std::vector<std::string> indexText;

					if ( indexText.size() < vectorSize )
					{
						const int prevSize = (int)indexText.size();
						indexText.resize( vectorSize );
						for ( int i = prevSize; i < vectorSize; i++ )
						{
							indexText[i] = "[";
							indexText[i] += std::to_string( i );
							indexText[i] += "]";
						}
					}

					if ( propertyMetaData && propertyMetaData->bExpanded ) {
						
						for ( int i = 0; i < vectorSize; i++ ) {
							Fl_Text_Display * propertyNameLabel = new Fl_Text_Display( startX + 24, curY + LineSpacing(), 0, inputHeight, indexText[i].c_str() );
							byte * curComponentByte = (byte*)g_NameToTypeInfoMap->GetVectorElement( byteOffsetToVar, pNextField->second.GetStructName(), i );
							startX += kbEditor::PanelBorderSize(5);
							RefreshProperty( pEntity, pNextField->first, pNextField->second.Type(), pNextField->second.GetStructName(), pComponent, curComponentByte, startX, curY, inputHeight, byteOffsetToVar, i );
							curY += LineSpacing();
							startX -= kbEditor::PanelBorderSize(5);
						}
					}
					break;
				}	
		} else {
			RefreshProperty( pEntity, pNextField->first, pNextField->second.Type(), pNextField->second.GetStructName(), pComponent, byteOffsetToVar, startX, curY, inputHeight );
			curY += LineSpacing();
		}
	}
}

/**
 *	kbPropertiesTab::RefreshEntity
 */
void kbPropertiesTab::RefreshEntity() {
	
	// note, must delete them both and readd them with the entity property first
	Fl::delete_widget( m_pEntityProperties );
	Fl::delete_widget( m_pResourceProperties );

	m_CallBackData.empty();
	size_t previousCapacity = m_CallBackData.capacity();

	m_pPropertiesTab->redraw();
	m_pEntityProperties = new Fl_Group( x() + kbEditor::PanelBorderSize(), y() + kbEditor::TabHeight() + kbEditor::PanelBorderSize(), w() - kbEditor::PanelBorderSize(2), h() - kbEditor::TabHeight(), "Entity Info" );
	Fl_Scroll * scroller = new Fl_Scroll( x() + kbEditor::PanelBorderSize(), y() + kbEditor::TabHeight()+ kbEditor::PanelBorderSize(), w() - kbEditor::PanelBorderSize(2), h() - kbEditor::TabHeight() - kbEditor::PanelBorderSize(2), "" );

	if ( m_SelectedEntities.size() == 1 || m_pTempPrefabEntity != NULL ) {	// todo: Don't display properties if multiple entities are selected
		int curY = y() + kbEditor::TabHeight() + kbEditor::PanelBorderSize();
		int startX = x() + kbEditor::PanelBorderSize();
		int inputWidth = 50;
		int inputHeight = 20;
		int ySpacing = 20;

		// TODO: Display properties for the first entity only for now.
		kbEditorEntity * pEntity = ( m_SelectedEntities.size() > 0 ) ? ( m_SelectedEntities[0] ) : ( m_pTempPrefabEntity );
		const kbGameEntity * pGameEntity = pEntity->GetGameEntity();

		for ( size_t i = 0; i < pGameEntity->NumComponents(); i++ ) {
			RefreshComponent( pEntity, const_cast< kbComponent * >( pGameEntity->GetComponent( i ) ), startX, curY, inputHeight );
			curY += LineSpacing();
		}
	}

	m_pPropertiesTab->add( m_pEntityProperties );

	m_pResourceProperties = new Fl_Group( 0, y() + kbEditor::TabHeight(), DisplayWidth(), h(), "Resource Info" );
	m_pPropertiesTab->add( m_pResourceProperties );
	scroller->end();
	Fl::wait();

	// hack - if the size of m_CallBackData has grown, recall this function so that pointers in m_CallBackData are valid
	if ( previousCapacity < m_CallBackData.capacity() )
	{
		m_CallBackData.reserve( m_CallBackData.capacity() * 2 );
		RefreshEntity();
	}
}

/**
 *	kbPropertiesTab::RefreshProperty
 */
void kbPropertiesTab::RefreshProperty( kbEditorEntity *const pEntity, const std::string & propertyName, const kbTypeInfoType_t propertyType, const std::string & structName, kbComponent * pComponent, byte * byteOffsetToVar,  int & xPos, int & yPos, const int inputHeight, void * pArrayPtr, const int arrayIndex ) {

	propertiesTabCBData_t cbData;
	cbData.pComponent = pComponent;
	cbData.fieldType = propertyType;

	const std::string propertyNameWithPadding = " LONGEST STRING";

	const int propertyNamePixelWidth = ( int )fl_width( propertyNameWithPadding.c_str() );
	int scrollBarSize = Fl::scrollbar_size();

	const int maxFieldWidth = x() + w() - (scrollBarSize + xPos + propertyNamePixelWidth + kbEditor::PanelBorderSize(2) );
	const int fourComponentFieldWidth = ( maxFieldWidth / 4 ) + ( kbEditor::PanelBorderSize() / 8 );
	const int threeComponentFieldWidth = ( maxFieldWidth / 3 ) + ( kbEditor::PanelBorderSize() / 8 );

	switch( propertyType )
	{
		case KBTYPEINFO_BOOL : {
			bool & boolean = *( bool * )byteOffsetToVar;
			cbData.pField = &boolean;
			m_CallBackData.push_back( cbData );

			Fl_Check_Button * button = new Fl_Check_Button( xPos + propertyNamePixelWidth, yPos, 25, inputHeight, "" );
			button->callback( &CheckButtonCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] )  );
			button->value( * ( (bool*) byteOffsetToVar ) );
			break;
		}

		case KBTYPEINFO_INT : {
			int & integer = *( int * )byteOffsetToVar;
			cbData.pField = &integer;
			m_CallBackData.push_back( cbData );
			
			int theW = x() + w();
			int dif = theW - xPos;

			Fl_Input * intInput = new Fl_Input( xPos + propertyNamePixelWidth, yPos, maxFieldWidth, inputHeight );
			intInput->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );
			char buffer[16];
			sprintf_s( buffer, "%d", * ( (int*) byteOffsetToVar ) );
			intInput->value( buffer );
			break;
		}

		case KBTYPEINFO_FLOAT : {
			float & numValue = *( float * )byteOffsetToVar;
			cbData.pField = &numValue;
			m_CallBackData.push_back( cbData );
			
			Fl_Input * floatInput = new Fl_Input( xPos + propertyNamePixelWidth, yPos, maxFieldWidth, inputHeight );
			floatInput->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );
						
			char buffer[16];
			floatInput->value( buffer );
			sprintf_s( buffer, "%.6f", * ( (float*) byteOffsetToVar ) );
			floatInput->value( buffer );
			break;
		}

		case KBTYPEINFO_STRUCT : {
			kbComponent *const pStruct = ( kbComponent *) byteOffsetToVar;
			yPos += inputHeight;
			RefreshComponent( pEntity, pStruct, xPos, yPos, inputHeight, true, pArrayPtr, arrayIndex );
			break;
		}

		case KBTYPEINFO_GAMEENTITY : {
			kbGameEntityPtr *const pEntityPtr = (kbGameEntityPtr*)byteOffsetToVar;
			kbGameEntity *const pEntity = pEntityPtr->GetEntity();
			if ( pEntity == NULL ) {
				Fl_Text_Display * propertyNameLabel = new Fl_Text_Display( xPos + propertyNamePixelWidth, yPos, 0, inputHeight, "NULL" );
				propertyNameLabel->textsize( FontSize() );
				propertyNameLabel->align( FL_ALIGN_RIGHT );
			} else {
				Fl_Text_Display * propertyNameLabel = new Fl_Text_Display( xPos + propertyNamePixelWidth, yPos, 0, inputHeight, pEntity->GetName().c_str() );
				propertyNameLabel->textsize( FontSize() );
				propertyNameLabel->align( FL_ALIGN_RIGHT );
			}

			Fl_Button *b1 = new Fl_Button( xPos + propertyNamePixelWidth - ( 5 + inputHeight / 2 ), yPos + (int)(inputHeight * 0.25f), inputHeight / 2,inputHeight / 2,">");
			b1->color(88+1);
			
			cbData.fieldType = propertyType;
			cbData.pVar = ( kbGameEntityPtr**)byteOffsetToVar;

			m_CallBackData.push_back( cbData );
			b1->callback( &PointerButtonCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );//static_cast< void * >( pComponent ) );
			break;
		}

		case KBTYPEINFO_SOUNDWAVE :
		case KBTYPEINFO_ANIMATION :
		case KBTYPEINFO_PTR :
		case KBTYPEINFO_TEXTURE :
		case KBTYPEINFO_STATICMODEL :
		case KBTYPEINFO_SHADER :
		{
			kbResource *const pResource = *( ( kbResource** ) byteOffsetToVar );
			if ( pResource != NULL ) {
				Fl_Text_Display * propertyNameLabel = new Fl_Text_Display( xPos + propertyNamePixelWidth, yPos, 0, inputHeight, pResource->GetName().c_str() );
				propertyNameLabel->textsize( FontSize() );
				propertyNameLabel->align( FL_ALIGN_RIGHT );
			}

			Fl_Button *b1 = new Fl_Button( xPos + propertyNamePixelWidth - ( 5 + inputHeight / 2 ), yPos + (int)(inputHeight * 0.25f), inputHeight / 2,inputHeight / 2,">");
			b1->color(88+1);
			
			cbData.pResource = ( kbResource** ) byteOffsetToVar;
			cbData.fieldType = propertyType;
			cbData.pField = const_cast< void* >( (void*)&propertyName );

			m_CallBackData.push_back( cbData );
			b1->callback( &PointerButtonCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );//static_cast< void * >( pComponent ) );
			break;
		}

		case KBTYPEINFO_VECTOR :
		{
			kbVec3 & vec = *( kbVec3 * )byteOffsetToVar;

			Fl_Input * X_Input = new Fl_Input( xPos + propertyNamePixelWidth, yPos, threeComponentFieldWidth, inputHeight );
			Fl_Input * Y_Input = new Fl_Input( xPos + propertyNamePixelWidth + threeComponentFieldWidth, yPos, threeComponentFieldWidth, inputHeight );
			Fl_Input * Z_Input = new Fl_Input( xPos + propertyNamePixelWidth + threeComponentFieldWidth * 2, yPos, threeComponentFieldWidth, inputHeight );

			cbData.pField = &vec.x;
			m_CallBackData.push_back( cbData );
			X_Input->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			cbData.pField = &vec.y;
			m_CallBackData.push_back( cbData );
			Y_Input->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			cbData.pField = &vec.z;
			m_CallBackData.push_back( cbData );
			Z_Input->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			X_Input->value( std::to_string( ( long double ) vec.x ).c_str() ); 
			Y_Input->value( std::to_string( ( long double ) vec.y ).c_str() ); 
			Z_Input->value( std::to_string( ( long double ) vec.z ).c_str() ); 

			break;
		}

		case KBTYPEINFO_VECTOR4 :
		{
			kbVec4 & color = *( kbVec4 * )byteOffsetToVar;

			Fl_Input * X_Input = new Fl_Input( xPos + propertyNamePixelWidth, yPos, fourComponentFieldWidth, inputHeight );
			Fl_Input * Y_Input = new Fl_Input( xPos + propertyNamePixelWidth + fourComponentFieldWidth, yPos, fourComponentFieldWidth, inputHeight );
			Fl_Input * Z_Input = new Fl_Input( xPos + propertyNamePixelWidth + fourComponentFieldWidth * 2, yPos, fourComponentFieldWidth, inputHeight );
			Fl_Input * W_Input = new Fl_Input( xPos + propertyNamePixelWidth + fourComponentFieldWidth * 3, yPos, fourComponentFieldWidth, inputHeight );

			cbData.pField = &color.x;
			m_CallBackData.push_back( cbData );
			X_Input->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			cbData.pField = &color.y;
			m_CallBackData.push_back( cbData );
			Y_Input->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			cbData.pField = &color.z;
			m_CallBackData.push_back( cbData );
			Z_Input->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			cbData.pField = &color.w;
			m_CallBackData.push_back( cbData );
			W_Input->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );

			X_Input->value( std::to_string( ( long double ) color.x ).c_str() ); 
			Y_Input->value( std::to_string( ( long double ) color.y ).c_str() ); 
			Z_Input->value( std::to_string( ( long double ) color.z ).c_str() ); 
			W_Input->value( std::to_string( ( long double ) color.w ).c_str() ); 

			break;
		}

		case KBTYPEINFO_ENUM : {
			const std::vector< std::string > * enumList = g_NameToTypeInfoMap->GetEnum( structName );
			Fl_Choice * pNewDropDown = new Fl_Choice( xPos + propertyNamePixelWidth, yPos, propertyNamePixelWidth, inputHeight );
			for ( int i = 0; i < enumList->size(); i++ ) {
				pNewDropDown->add( (*enumList)[i].c_str() );
			}

			int & enumIntValue = * ( (int*) byteOffsetToVar );
			pNewDropDown->value( enumIntValue );
			cbData.pField = &enumIntValue;
			m_CallBackData.push_back( cbData );
			pNewDropDown->callback( &EnumCB, static_cast< void * >( & m_CallBackData[ m_CallBackData.size() - 1 ] ) );
			break;
		}

		case KBTYPEINFO_KBSTRING : {
			kbString & string = *( kbString * )byteOffsetToVar;
			cbData.pField = &string;
			m_CallBackData.push_back( cbData );

			Fl_Input * stringInput = new Fl_Input( xPos + propertyNamePixelWidth, yPos, maxFieldWidth, inputHeight );
			stringInput->callback( &TextFieldCB, static_cast< void * >( &m_CallBackData[ m_CallBackData.size() - 1 ] ) );
			stringInput->value( string.c_str() );
			break;
		}
	}
}

/**
 *	kbPropertiesTab::DeleteComponent
 */
void kbPropertiesTab::DeleteComponent( Fl_Widget * widget, void * voidptr ) {
	propertiesTabCBData_t * userData = static_cast< propertiesTabCBData_t * >( voidptr );

	kbComponent *const componentToDelete = userData->pComponent;
	std::string msg = "Delete ";
	msg += componentToDelete->GetComponentClassName();
	msg += "?";

	const int deleteComponent = fl_ask( msg.c_str() );
	if ( deleteComponent == 0 ) {
		return;
	}

	int componentIdx = -1;
	kbGameEntity *const pEntity = componentToDelete->GetParent();
	for ( componentIdx = 0; componentIdx < pEntity->NumComponents(); componentIdx++ ) {
		if ( pEntity->GetComponent(componentIdx) == componentToDelete ) {
			break;
		}
	}

	pEntity->RemoveComponent( componentToDelete );
	//componentToDelete->Enable( false );
	g_Editor->PushUndoAction( new kbUndoDeleteComponent( userData->pEditorEntity, componentToDelete, componentIdx ) );

	g_pPropertiesTab->RefreshEntity();
}

/**
 *	kbPropertiesTab::InsertArrayStruct
 */
void kbPropertiesTab::InsertArrayStruct( Fl_Widget * widget, void * voidPtr ) {
	propertiesTabCBData_t * userData = static_cast< propertiesTabCBData_t * >( voidPtr );

	g_NameToTypeInfoMap->InsertVectorElement( userData->pArray, userData->structName, userData->arrayIndex );
	g_pPropertiesTab->RefreshEntity();
}

/**
 *	kbPropertiesTab::DeleteArrayStruct
 */
void kbPropertiesTab::DeleteArrayStruct( Fl_Widget * widget, void * voidPtr ) {
	propertiesTabCBData_t * userData = static_cast< propertiesTabCBData_t * >( voidPtr );

	g_NameToTypeInfoMap->RemoveVectorElement( userData->pArray, userData->structName, userData->arrayIndex );
	g_pPropertiesTab->RefreshEntity();
}