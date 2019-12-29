//===================================================================================================
// kbResourceTab.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <queue>
#include "kbCore.h"
#include "kbWidget.h"
#include "kbGameEntityHeader.h"
#include "kbResourceTab.h"
#include "kbPropertiesTab.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

kbResourceTab * g_pResourceTab = nullptr;


/**
 *	GetAbsoluteFolderName_Recursive
 */
std::string GetAbsoluteFolderName_Recursive( const kbResourceTabFile_t *const pCurTab, const kbResourceTabFile_t *const pTargetTab ) {

	if ( pCurTab == pTargetTab ) {
		return pTargetTab->m_FolderName;
	}

	for ( int i = 0; i < pCurTab->m_SubFolderList.size(); i++ ) {
		std::string retVal = GetAbsoluteFolderName_Recursive( &pCurTab->m_SubFolderList[i], pTargetTab );
		if ( retVal.empty() == false ) {
			return pCurTab->m_FolderName + retVal;
		}
	}

	for ( int i = 0; i < pCurTab->m_ResourceList.size(); i++ ) {
		std::string retVal = GetAbsoluteFolderName_Recursive( &pCurTab->m_ResourceList[i], pTargetTab );
		if ( retVal.empty() == false ) {
			return pCurTab->m_FolderName + retVal;
		}
	}
	return {};
}

/**
 *	GetAbsoluteFolderName
 */
std::string	GetAbsoluteFolderName( const std::vector<kbResourceTabFile_t> & searchList, const kbResourceTabFile_t *const resourceTabFile ) {

	for ( int i = 0; i < searchList.size(); i++ ) {
		std::string fullPath = GetAbsoluteFolderName_Recursive( &searchList[i], resourceTabFile );
		if ( fullPath.empty() == false ) {
			return fullPath;
		}
	}

	return {};
}

/**
 *	ResourceSelectedCB - Called when user selects a resource in the resource tab
 */
void kbResourceTab::ResourceSelectedCB( Fl_Widget * widget, void * userData ) {
	Fl_Select_Browser *const selectBrowser = static_cast< Fl_Select_Browser * >( widget );
	kbResourceTab *const pResourceTab = static_cast< kbResourceTab * >( userData );

	const int selectedItemIndex = selectBrowser->value() - 1;
    if ( selectedItemIndex == -1 ) {
        return;
    }

	if ( Fl::event_button() == FL_LEFT_MOUSE && selectedItemIndex >= 0 ) {
		kbResourceTabFile_t * pResourceItem = pResourceTab->m_SelectBrowserIdx[selectedItemIndex];

		if ( pResourceItem->m_pResource != nullptr ) {
			const char * fileName = pResourceItem->m_pResource->GetFullFileName().c_str();
			kbResource * pResource = g_ResourceManager.GetResource( fileName, false, false );

			widgetCBResourceSelected resourceCBObject( WidgetCB_ResourceSelected );
			resourceCBObject.resourceFileName = pResource->GetFullFileName();
			g_Editor->BroadcastEvent( resourceCBObject );
		} else if ( pResourceItem->m_pPrefab != nullptr ) {
			widgetCBResourceSelected resourceCBObject( WidgetCB_PrefabSelected );
			g_Editor->BroadcastEvent( resourceCBObject );
		} else {
			pResourceItem->m_bExpanded = !pResourceItem->m_bExpanded;
		}
	}

	if ( Fl::event_button() == FL_RIGHT_MOUSE ) {
		int folderIdx = selectedItemIndex;
		while( folderIdx > 0 && GetFileExtension( g_pResourceTab->m_SelectBrowserIdx[selectedItemIndex]->m_FolderName ) != "kbPkg" ) {
			folderIdx--;
		}

		const std::string SavePackageOption = "Save Package " + g_pResourceTab->m_SelectBrowserIdx[selectedItemIndex]->m_FolderName;
		Fl_Menu_Item rclick_menu[] = {
			{ SavePackageOption.c_str(),  0, SavePackageCB, ( void * ) (INT_PTR)folderIdx },		// Cast to INT_PTR then to void * fixes compile warning C4312
			{ "Save All Changed Packages",  0, SavePackageCB, ( void * ) (INT_PTR)-1 },
			{ "Delete Resource", 0, DeleteResouceCB, ( void * ) (INT_PTR)selectedItemIndex },
			{ 0 }};

		// Gray out save package option if it's not dirty
		if ( selectedItemIndex < 0 || g_pResourceTab->m_SelectBrowserIdx[selectedItemIndex]->m_bIsDirty == false ) {
			rclick_menu[0].deactivate();
		}

		{//if ( selectedItemIndex < 0 ) {
			rclick_menu[2].deactivate();
		}
		const Fl_Menu_Item * m = rclick_menu->popup( Fl::event_x(), Fl::event_y(), 0, 0, 0 );
		if ( m ) {
			m->do_callback( 0, m->user_data() );
		}

	}

	const int scrollPos = pResourceTab->m_pResourceSelectBrowser->position();
	pResourceTab->RefreshResourcesTab();
	pResourceTab->m_pResourceSelectBrowser->position( scrollPos );
	selectBrowser->select( selectedItemIndex + 1 );
}

void ClearDirtyFlags( kbResourceTabFile_t *const pkbResourceTabFile_t ) {
	if ( pkbResourceTabFile_t == nullptr ) {
		return;
	}

	pkbResourceTabFile_t->m_bIsDirty = false;
	for ( int i = 0; i < pkbResourceTabFile_t->m_ResourceList.size(); i++ ) {
		pkbResourceTabFile_t->m_ResourceList[i].m_bIsDirty = false;
	}

	for ( int i = 0; i < pkbResourceTabFile_t->m_SubFolderList.size(); i++ ) {
		ClearDirtyFlags( &pkbResourceTabFile_t->m_SubFolderList[i] );
	}
}

/**
 *	SavePackageCB - Called when user selects a resource in the resource tab
 */
void kbResourceTab::SavePackageCB( Fl_Widget * widget, void * userData ) {

	const int index = (int) (INT_PTR)userData;		// Cast to INT_PTR then to void * fixes compile warning C4312

	if ( index == -1 ) {
		for ( int i = 0; i < g_pResourceTab->m_SelectBrowserIdx.size(); i++ ) {
			if ( g_pResourceTab->m_SelectBrowserIdx[i]->m_bIsDirty == false || g_pResourceTab->m_SelectBrowserIdx[i]->m_FolderName.empty() ) {
				continue;
			}

			if ( GetFileExtension( g_pResourceTab->m_SelectBrowserIdx[i]->m_FolderName ) == "kbPkg" ) {
				g_ResourceManager.SavePackage( g_pResourceTab->m_SelectBrowserIdx[i]->m_FolderName );
				ClearDirtyFlags( g_pResourceTab->m_SelectBrowserIdx[i] );
				
			}
		}
	} else if ( index < g_pResourceTab->m_SelectBrowserIdx.size() ) {
		g_pResourceTab->m_SelectBrowserIdx[index]->m_bIsDirty = false;
		g_ResourceManager.SavePackage( g_pResourceTab->m_SelectBrowserIdx[index]->m_FolderName );
		ClearDirtyFlags( g_pResourceTab->m_SelectBrowserIdx[index] );
	}

	g_pResourceTab->RefreshResourcesTab();
}

/**
 *	DeleteResouceCB - Called when user selects a resource in the resource tab
 */
void kbResourceTab::DeleteResouceCB( Fl_Widget * widget, void * userData ) {
	/*Fl_Select_Browser *const selectBrowser = g_pResourceTab->m_pEntitySelectBrowser;
	kbResourceTab *const pResourceTab = static_cast< kbResourceTab * >( userData );

	const int selectedItemIndex = (INT_PTR)userData;
    if ( selectedItemIndex == -1 ) {
        return;
    }

	kbResourceTabFile_t * pResourceItem = pResourceTab->m_SelectBrowserIdx[selectedItemIndex];

	if ( pResourceItem->m_pResource != nullptr ) {
		const int areYouSure = fl_ask( "Really delete %s", pResourceItem->m_pResource->GetFullName().c_str() );
		if ( areYouSure == 1 ) {
			const char * fileName = pResourceItem->m_pResource->GetFullFileName().c_str();
			DeleteFile( fileName );
		}
	}*/

	g_pResourceTab->RefreshResourcesTab();
}

/**
 *	kbResourceTab
 */
kbResourceTab::kbResourceTab( int x, int y, int w, int h ) :
	kbWidget( x, y, w, h ),
	Fl_Tabs( x, y, w, h ) {

	const int Top_Border = y + kbEditor::TabHeight();
	const int Display_Width = DisplayWidth();
	const int Display_Height = h - kbEditor::TabHeight();

	Fl_Tabs *const resourceTabs  = new Fl_Tabs( x, y, w, Display_Height );

	{
		Fl_Group *const resourceGroup = new Fl_Group( x, Top_Border, w, Display_Height, "Resources" );
		m_pResourceSelectBrowser = new Fl_Select_Browser( 5, Top_Border + 5, Display_Width, Display_Height , "" );
		m_pResourceSelectBrowser->callback( &ResourceSelectedCB, this );
		m_pResourceSelectBrowser->textsize( FontSize() );
		resourceGroup->end();
	}

	{
		Fl_Group *const resourceGroup = new Fl_Group( 0, Top_Border, Display_Width, Display_Height, "Entities" );
		m_pEntitySelectBrowser = new Fl_Select_Browser( 5, Top_Border + 5, Display_Width, Display_Height , "" );
		m_pEntitySelectBrowser->callback( *EntitySelectedCB, this );
		m_pEntitySelectBrowser->textsize( FontSize() );
	    resourceGroup->end();
	}

	resourceTabs->end();

	end();

	g_Editor->RegisterUpdate( this );
	g_Editor->RegisterEvent( this, WidgetCB_EntityModified );
	g_Editor->RegisterEvent( this, WidgetCB_PrefabModified );
	g_pResourceTab = this;

	g_ResourceManager.RegisterCB( ResourceManagerCB, kbResourceManager::CBR_FileModified );
}

/**
 *	kbResourceTab::~kbResourceTab
 */
kbResourceTab::~kbResourceTab() {
	g_ResourceManager.UnregisterCB( ResourceManagerCB );
}

/**
 *	kbResourceTab::EventCB
 */
void kbResourceTab::EventCB( const widgetCBObject * widgetCBObject ) {
	
	switch( widgetCBObject->widgetType ) {
		
		case WidgetCB_EntityModified : {
			RefreshEntitiesTab();
			break;
		};

		case WidgetCB_PrefabModified : {
			const widgetCBGeneric *const widgetCB = static_cast<const widgetCBGeneric*>( widgetCBObject );

			int value = m_pResourceSelectBrowser->value() - 1;

			if ( g_pPropertiesTab != nullptr && g_pPropertiesTab->GetTempPrefabEntity() != nullptr ) {
				for ( int i = 0; i < m_SelectBrowserIdx.size(); i++ ) {
					if ( m_SelectBrowserIdx[i]->m_pPrefab != nullptr && m_SelectBrowserIdx[i]->m_pPrefab->GetGameEntity( 0 ) == g_pPropertiesTab->GetTempPrefabEntity()->GetGameEntity() ) {
						value = i;
						break;
					}
				}
			}

			if ( value < 0 || value >= m_SelectBrowserIdx.size() || m_SelectBrowserIdx[value]->m_pPrefab == nullptr ) {
				return;
			}
			// Todo: Hack.  Find a better way to mark folders as dirty
			if ( m_SelectBrowserIdx[value]->m_pPrefab->GetGameEntity( 0 ) != nullptr && m_SelectBrowserIdx[value]->m_pPrefab->GetGameEntity( 0 ) == widgetCB->m_Value ) {
				g_pResourceTab->m_SelectBrowserIdx[value]->m_bIsDirty = true;
				while( value > 0 && GetFileExtension( g_pResourceTab->m_SelectBrowserIdx[value]->m_FolderName ) != "kbPkg" ) {
					value--;
				}
				if ( value >= 0 ) {
					g_pResourceTab->m_SelectBrowserIdx[value]->m_bIsDirty = true;
				}
				RefreshResourcesTab();
			}
		}
		break;
	}
}

/**
 *	kbResourceTab::PostRendererInit
 */
void kbResourceTab::PostRendererInit() {
	RebuildResourceFolderListText();
	RefreshEntitiesTab();
}

/**
 *	kbResourceTab::RebuildResourceFolderListText
 */
void kbResourceTab::RebuildResourceFolderListText() {

	std::vector<kbResourceTabFile_t> backUp = m_ResourceFolderList;
	std::vector<std::string> expandedDirectoryList;
	std::vector<std::string> dirtyDirectoryList;
	
	// kbLog( "---------------------------------" );
	if ( m_ResourceFolderList.size() > 0 ) {
		std::queue<kbResourceTabFile_t*> q;
		for ( int i = 0; i < m_ResourceFolderList.size(); i++ ) {
			q.push( &backUp[i] );
		}

		while( q.empty() == false ) {

			kbResourceTabFile_t *const pCurTab = q.front();
			q.pop();

			if ( pCurTab->m_bExpanded == false && pCurTab->m_bIsDirty == false ) {
				continue;
			}

			const std::string fullFolderName = GetAbsoluteFolderName( backUp, pCurTab );

			if ( pCurTab->m_bExpanded ) {
				expandedDirectoryList.push_back( fullFolderName );
			}

			if ( pCurTab->m_bIsDirty ) {
				dirtyDirectoryList.push_back( fullFolderName );
			}

			// kbLog( "---> Adding dir %s\n", pCurTab->m_FolderName.c_str() );
			for ( int j = 0; j < pCurTab->m_SubFolderList.size(); j++ ) {
				q.push( &pCurTab->m_SubFolderList[j] );
			}

			for ( int j = 0; j < pCurTab->m_ResourceList.size(); j++ ) {
				q.push( &pCurTab->m_ResourceList[j] );
			}

		}
	}

	TCHAR szFullPattern[MAX_PATH];
	GetCurrentDirectory( MAX_PATH, szFullPattern );
	std::string filePath = szFullPattern;

	std::vector< std::string > fileList;

	g_pResourceTab->m_ResourceFolderList.clear();
	m_ResourceFolderList.push_back( kbResourceTabFile_t() );
	m_ResourceFolderList[m_ResourceFolderList.size() - 1].m_FolderName = "Game Packages";

	// Find game directory assets
	m_ResourceFolderList.push_back( kbResourceTabFile_t() );
	m_ResourceFolderList[m_ResourceFolderList.size() - 1].m_FolderName = "Game Resources";

	filePath = "./assets/";
	FindResourcesRecursively( filePath, m_ResourceFolderList[m_ResourceFolderList.size() - 1] );

	// find engine assets
	m_ResourceFolderList.push_back( kbResourceTabFile_t() );
	m_ResourceFolderList[m_ResourceFolderList.size() - 1].m_FolderName = "Engine Resources";

	filePath = "../../kbEngine/assets/";
	FindResourcesRecursively( filePath, m_ResourceFolderList[m_ResourceFolderList.size() - 1] );

	if ( backUp.size() > 0 ) {
		kbLog( "Rebuilding folder info...\n" );
		std::queue<kbResourceTabFile_t*> q;
		for ( int i = 0; i < m_ResourceFolderList.size(); i++ ) {
			q.push( &m_ResourceFolderList[i] );
		}

		while( q.empty() == false ) {
			kbResourceTabFile_t *const pCurTab = q.front();
			q.pop();

			const std::string curFolderName = GetAbsoluteFolderName( m_ResourceFolderList, pCurTab );

			if ( VectorContains( expandedDirectoryList, curFolderName ) ) {
				pCurTab->m_bExpanded = true;

				for ( int i = 0; i < pCurTab->m_SubFolderList.size(); i++ ) {
					q.push( &pCurTab->m_SubFolderList[i] );
				}

				for ( int j = 0; j < pCurTab->m_ResourceList.size(); j++ ) {
					q.push( &pCurTab->m_ResourceList[j] );
				}
			}

			if ( VectorContains( dirtyDirectoryList, curFolderName ) ) {
				pCurTab->m_bIsDirty = true;
			}

		}
	}

	RefreshResourcesTab();
}

/**
 *	kbResourceTab::FindResourcesRecursively
 */
void kbResourceTab::FindResourcesRecursively( const std::string & file, kbResourceTabFile_t & CurrentFolder ) {
	const size_t currentFolderStartPos = file.find_last_of( "/", file.length() - 2 );
	const size_t currentFolderEndPosPos = file.find_first_of( "/", currentFolderStartPos + 1 );//"file.find_last_of( "/", file.length() - 1 );

	const std::string currentFolderName = file.substr( currentFolderStartPos, currentFolderEndPosPos );
	if ( currentFolderName == "/CVS/" ) {
		return;
	}
	CurrentFolder.m_SubFolderList.push_back( kbResourceTabFile_t() );
	kbResourceTabFile_t & NewFolder = CurrentFolder.m_SubFolderList[ CurrentFolder.m_SubFolderList.size() - 1];
	NewFolder.m_FolderName = currentFolderName;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile;
	
	static char fullFileName[MAX_PATH];
	sprintf_s( fullFileName, "%s*", file.c_str() );

	hFindFile = FindFirstFile( fullFileName, &FindFileData );

	if ( hFindFile != INVALID_HANDLE_VALUE ) {
		do {
			if ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				if ( strcmp( FindFileData.cFileName, "." ) == 0 ||
					 strcmp( FindFileData.cFileName, ".." ) == 0 ) {
					continue;
				}

				std::string newFolder = file + FindFileData.cFileName + "/";
				FindResourcesRecursively( newFolder, NewFolder );
			} else {
				const char * ext = strrchr( FindFileData.cFileName, '.' );
		
				if ( ext == nullptr ) {
					continue;
				}

				const int MaxNumExtensions = 16;
				const char validExtensions[][MaxNumExtensions] = { ".fbx", ".dds", ".png", ".ms3d", ".kbMat", ".kbShader", ".jpg", ".tga", ".bmp", ".kbPkg", ".kbAnim", ".wav", ".diablo3", ".tif" };
				const int numExtensions = sizeof( validExtensions ) / ( sizeof( char ) * 16 );

				for ( int i = 0; i < numExtensions; i++ ) {

					if ( strcmp( ext, validExtensions[i] ) == 0 ) {
						if ( strcmp( ext, ".kbPkg" ) == 0 ) {
							// Add a package and its folders and prefab
							kbPackage *const pPackage = g_ResourceManager.GetPackage( file + FindFileData.cFileName, false );		// MATERIALHACK - Need to lazy load these packages when used in editor
							kbErrorCheck( pPackage != nullptr, "kbResourceTab::FindResourcesRecursively() - Failed to load package" );

							// Add Package
							m_ResourceFolderList[0].m_SubFolderList.push_back( kbResourceTabFile_t() );
							kbResourceTabFile_t & newPackageEntry = m_ResourceFolderList[0].m_SubFolderList[m_ResourceFolderList[0].m_SubFolderList.size() - 1];
							newPackageEntry.m_FolderName = FindFileData.cFileName;
							
							for ( int folderIdx = 0; folderIdx < pPackage->NumFolders(); folderIdx++ ) {
								// Add Folder
								newPackageEntry.m_SubFolderList.push_back( kbResourceTabFile_t() );
								kbResourceTabFile_t & newFolderEntry = newPackageEntry.m_SubFolderList[newPackageEntry.m_SubFolderList.size() - 1 ];
								newFolderEntry.m_FolderName = pPackage->GetFolderName( folderIdx );

								const std::vector< class kbPrefab * > & prefabsInFolder = pPackage->GetPrefabsForFolder( folderIdx );
								for ( int prefabIdx = 0; prefabIdx < prefabsInFolder.size(); prefabIdx++ ) {
									// Add prefab
									newFolderEntry.m_ResourceList.push_back( kbResourceTabFile_t() );
									kbResourceTabFile_t & newPrefabEntry = newFolderEntry.m_ResourceList[newFolderEntry.m_ResourceList.size() - 1];
									newPrefabEntry.m_pPrefab = prefabsInFolder[prefabIdx];
									newPrefabEntry.m_FolderName = prefabsInFolder[prefabIdx]->GetPrefabName();
								}
							}
						} else {
							// Add a kbResource
							NewFolder.m_ResourceList.push_back( kbResourceTabFile_t() );

							std::string fileName = file + FindFileData.cFileName ;
							StringToLower( fileName );

							NewFolder.m_ResourceList[NewFolder.m_ResourceList.size() - 1].m_pResource = g_ResourceManager.GetResource( fileName, false, true );
						}
						continue;
					}
				}
			}
		} while( FindNextFile( hFindFile, &FindFileData ) );

		FindClose( hFindFile );
	}
}

/**
 *	kbResourceTab::RefreshResourcesTab
 */
void kbResourceTab::RefreshResourcesTab() {
	std::string spaces;
	m_pResourceSelectBrowser->clear();
	m_SelectBrowserIdx.clear();

	const int scrollPos = m_pResourceSelectBrowser->position();
	// Add packages to the UI
	
	// Add resources to the UI
	for ( int folderIdx = 0; folderIdx < m_ResourceFolderList.size(); folderIdx++ ) {
		RefreshResourcesTab_Recursive( m_ResourceFolderList[folderIdx], spaces );
	}
	m_pResourceSelectBrowser->redraw();
	this->redraw();
	Fl::wait();
}

/**
 *	kbResourceTab::RefreshResourcesTab_Recursive
 */
void kbResourceTab::RefreshResourcesTab_Recursive( kbResourceTabFile_t & currentFolder, std::string spaces ) {

	if ( currentFolder.m_ResourceList.size() == 0 && currentFolder.m_SubFolderList.size() == 0 ) {
		return;
	}

	std::string FolderName = spaces + ( currentFolder.m_bExpanded ? "- " : "+ " ) + currentFolder.m_FolderName;
	if ( currentFolder.m_bIsDirty ) {
		FolderName += " *";
	}

	m_pResourceSelectBrowser->add( FolderName.c_str() );
	int numItems = m_pResourceSelectBrowser->size();
	m_SelectBrowserIdx.push_back( &currentFolder );

	if ( currentFolder.m_bExpanded == true ) {
		spaces += "   ";

		for ( int folderIdx = 0; folderIdx < currentFolder.m_SubFolderList.size(); folderIdx++ ) {

			kbResourceTabFile_t & nextFolder = currentFolder.m_SubFolderList[folderIdx];
			RefreshResourcesTab_Recursive( nextFolder, spaces );
		}

		for ( int resourceIdx = 0; resourceIdx < currentFolder.m_ResourceList.size(); resourceIdx++ ) {
			if ( currentFolder.m_ResourceList[resourceIdx].m_pResource != nullptr ) {
				const std::string & resourceName = currentFolder.m_ResourceList[resourceIdx].m_pResource->GetName();
				const size_t fileNameStart = resourceName.find_last_of( '\\' );
				std::string displayName;
				if ( fileNameStart == std::string::npos ) {
					displayName = resourceName;
				} else {
					displayName = resourceName.substr( fileNameStart + 1 );
				}
				m_pResourceSelectBrowser->add( ( spaces + displayName ).c_str() );
			} else if ( currentFolder.m_ResourceList[resourceIdx].m_pPrefab != nullptr ) {
				std::string prefabName = ( spaces + currentFolder.m_ResourceList[resourceIdx].m_pPrefab->GetPrefabName() ).c_str();
				if ( currentFolder.m_ResourceList[resourceIdx].m_bIsDirty ) {
					prefabName += " *";
				}
				m_pResourceSelectBrowser->add( prefabName.c_str() );
			}
			m_SelectBrowserIdx.push_back( &currentFolder.m_ResourceList[resourceIdx] );
		}
		spaces.resize( spaces.length() - 1 );
	}
}

/**
 *	kbResourceTab::AddPrefab
 */
void kbResourceTab::AddPrefab( kbPrefab * pPrefab, const std::string & PackageName, const std::string & FolderName, const std::string & PrefabName ) {
	kbResourceTabFile_t & rootPackage = m_ResourceFolderList[0];

	// Iterate over packages
	int prefabIdx = 0;
	for ( ; prefabIdx < rootPackage.m_SubFolderList.size(); prefabIdx++ ) {
		
		// Search packages
		kbResourceTabFile_t & packageFolder = rootPackage.m_SubFolderList[prefabIdx];
		if ( packageFolder.m_FolderName != PackageName ) {
			continue;
		}
 
		// Search sub-folders
		int subFolderIdx = 0;
		for ( ; subFolderIdx < packageFolder.m_SubFolderList.size(); subFolderIdx++ ) {
			kbResourceTabFile_t & subFolder = packageFolder.m_SubFolderList[subFolderIdx];
			if ( subFolder.m_FolderName != FolderName ) {
				continue;
			}

			// Search prefabs
			for ( int prefabIdx = 0; prefabIdx < subFolder.m_ResourceList.size(); prefabIdx++ ) {
				if ( PrefabName == subFolder.m_ResourceList[prefabIdx].m_pPrefab->GetPrefabName() ) {
					//kbError( "Prefab already exists" );
					kbResourceTabFile_t & itemToReplace = subFolder.m_ResourceList[prefabIdx];
					itemToReplace.m_FolderName = pPrefab->GetPrefabName();
					itemToReplace.m_pPrefab = pPrefab;
					RefreshResourcesTab();
					return;
				}

				// Add prefab
				subFolder.m_ResourceList.push_back( kbResourceTabFile_t() );
				kbResourceTabFile_t & newPrefab = subFolder.m_ResourceList[subFolder.m_ResourceList.size() - 1];
				newPrefab.m_FolderName = pPrefab->GetPrefabName();
				newPrefab.m_pPrefab = pPrefab;
				newPrefab.m_bIsDirty = true;
				packageFolder.m_bIsDirty = true;
				RefreshResourcesTab();
				return;
			}
		}

		// Add sub-folder and then prefab
		packageFolder.m_SubFolderList.push_back( kbResourceTabFile_t() );
		kbResourceTabFile_t & newSubFolder = packageFolder.m_SubFolderList[packageFolder.m_SubFolderList.size() - 1];
		newSubFolder.m_FolderName = FolderName;

		newSubFolder.m_ResourceList.push_back( kbResourceTabFile_t() );
		kbResourceTabFile_t & newPrefabEntry = newSubFolder.m_ResourceList[newSubFolder.m_ResourceList.size() - 1];
		newPrefabEntry.m_FolderName = pPrefab->GetPrefabName();
		newPrefabEntry.m_pPrefab = pPrefab;
		RefreshResourcesTab();
		return;
	}

	rootPackage.m_SubFolderList.push_back( kbResourceTabFile_t() );
	kbResourceTabFile_t & packageFolder = rootPackage.m_SubFolderList[rootPackage.m_SubFolderList.size() - 1];
	packageFolder.m_FolderName = PackageName;

	// Add sub-folder and then prefab
	packageFolder.m_SubFolderList.push_back( kbResourceTabFile_t() );
	kbResourceTabFile_t & newSubFolder = packageFolder.m_SubFolderList[packageFolder.m_SubFolderList.size() - 1];
	newSubFolder.m_FolderName = FolderName;

	newSubFolder.m_ResourceList.push_back( kbResourceTabFile_t() );
	kbResourceTabFile_t & newPrefabEntry = newSubFolder.m_ResourceList[newSubFolder.m_ResourceList.size() - 1];
	newPrefabEntry.m_FolderName = pPrefab->GetPrefabName();
	newPrefabEntry.m_pPrefab = pPrefab;

	RefreshResourcesTab();
}

/**
 *	kbResourceTab::GetSelectedPrefab
 */
kbPrefab * kbResourceTab::GetSelectedPrefab() const {
	const int value = m_pResourceSelectBrowser->value() - 1;

	if ( value < 0 || value >= m_SelectBrowserIdx.size() ) {
		return nullptr;
	}

	return m_SelectBrowserIdx[value]->m_pPrefab;
}

/**
 *	kbPackage::MarkPrefabDirty
 */
void kbResourceTab::MarkPrefabDirty( kbPrefab * prefab ) {
	int value = m_pResourceSelectBrowser->value() - 1;
	if ( value < 0 || value >= m_SelectBrowserIdx.size() ) {
		return;
	}

	if ( m_SelectBrowserIdx[value]->m_pPrefab == prefab ) {
		m_SelectBrowserIdx[value]->m_bIsDirty = true;

		while( value > 0 && GetFileExtension( g_pResourceTab->m_SelectBrowserIdx[value]->m_FolderName ) != "kbPkg" ) {
			value--;
		}
		if ( value >= 0 ) {
			g_pResourceTab->m_SelectBrowserIdx[value]->m_bIsDirty = true;
		}
	}

	RefreshResourcesTab();
}

/**
 *	kbResourceTab::ResourceManagerCB
 */
void kbResourceTab::ResourceManagerCB( const kbResourceManager::CallbackReason Reason ) {

	g_pResourceTab->RebuildResourceFolderListText();

}

/**
 *	kbResourceTab::RefreshEntitiesTab
 */
void kbResourceTab::RefreshEntitiesTab() {
	g_pResourceTab->m_pEntitySelectBrowser->clear();
	m_EntityList.clear();

	std::vector<kbEditorEntity *> &	editorEntities = g_Editor->GetGameEntities();
	for ( int i = 0; i < editorEntities.size(); i++ ) {
		g_pResourceTab->m_pEntitySelectBrowser->add( editorEntities[i]->GetGameEntity()->GetName().c_str() );

		EntitySelectItem_t newItem;
		newItem.m_pEntity = editorEntities[i];
		m_EntityList.push_back( newItem );
	}

	g_pResourceTab->m_pEntitySelectBrowser->redraw();
	g_pResourceTab->redraw();
	Fl::wait();
}

/**
 *	kbResourceTab::EntitySelectedCB
 */
void kbResourceTab::EntitySelectedCB( Fl_Widget * pWidget, void * pUserData ) {

	const int selectedItemIndex = g_pResourceTab->m_pEntitySelectBrowser->value() - 1;

	if ( selectedItemIndex < 0 || selectedItemIndex >= g_pResourceTab->m_EntityList.size() ) {
		g_Editor->DeselectEntities();
		return;
	}

	if ( Fl::event_button() == FL_LEFT_MOUSE ) {

		std::vector<kbEditorEntity*> editorEntityList;
		editorEntityList.push_back( g_pResourceTab->m_EntityList[selectedItemIndex].m_pEntity );
		g_Editor->SelectEntities( editorEntityList, false );

	} else if ( Fl::event_button() == FL_RIGHT_MOUSE ) {
		
		const std::string DeleteEntity = "Delete entity " + g_pResourceTab->m_EntityList[selectedItemIndex].m_pEntity->GetGameEntity()->GetName().stl_str();
		const std::string ZoomToEntity = "Zoom to entity " + g_pResourceTab->m_EntityList[selectedItemIndex].m_pEntity->GetGameEntity()->GetName().stl_str();


		Fl_Menu_Item rclick_menu[] = {
			{ ZoomToEntity.c_str(), 0, ZoomToEntityCB, g_pResourceTab->m_EntityList[selectedItemIndex].m_pEntity },
			{ "", 0, nullptr },
			{ DeleteEntity.c_str(), 0, DeleteCB, g_pResourceTab->m_EntityList[selectedItemIndex].m_pEntity },
			{ 0 }};

		rclick_menu[1].deactivate();
		/*if ( selectedItemIndex < 0 || g_pResourceTab->m_SelectBrowserIdx[selectedItemIndex]->m_bIsDirty == false ) {
			rclick_menu[0].deactivate();
		}*/

		const Fl_Menu_Item *const m = rclick_menu->popup( Fl::event_x(), Fl::event_y(), 0, 0, 0 );
		if ( m != nullptr ) {
			m->do_callback( 0, m->user_data() );
		}

	}
}

/**
 *	kbResourceTab::DeleteCB
 */
void kbResourceTab::DeleteCB( Fl_Widget * pWidget, void * pUserData ) {

	const int areYouSure = fl_ask( "Really delete this entity?" );
	if ( areYouSure == 0 ) {
		return;
	}

	kbEditorEntity *const pEditorEntity = (kbEditorEntity*)pUserData;

	std::vector<kbEditorEntity *> entitiesToDelete;
	entitiesToDelete.push_back( pEditorEntity );
	g_Editor->DeleteEntities( entitiesToDelete );

	g_pResourceTab->RefreshEntitiesTab();
}

/**
 *	kbResourceTab::ZoomToEntityCB
 */
void kbResourceTab::ZoomToEntityCB( Fl_Widget * pWidget, void * pUserData ) {
	kbEditorEntity *const pEditorEntity = (kbEditorEntity*)pUserData;

	const float zoomDist = 75.0f;

	const kbVec3 camPos = g_Editor->GetMainCameraPos();
	kbVec3 vecTo = ( camPos - pEditorEntity->GetPosition() );
	vecTo.y = 0;
	if ( vecTo.Length() < zoomDist ) {
		return;
	}

	const kbVec3 finalPos = pEditorEntity->GetPosition() + vecTo.Normalized() * zoomDist;
	g_Editor->SetMainCameraPos( finalPos );

	kbMat4 newRot;
	newRot.LookAt( finalPos, pEditorEntity->GetPosition(), kbVec3::up );
	newRot.InvertFast();
	g_Editor->SetMainCameraRot( kbQuatFromMatrix( newRot ) );
}