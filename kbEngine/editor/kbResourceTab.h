//===================================================================================================
// kbResourceTab.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBRESOURCETAB_H_
#define _KBRESOURCETAB_H_

/**
 *	kbResourceTabFile_t
 */
struct kbResourceTabFile_t {
	kbResourceTabFile_t() : m_pPrefab( nullptr ), m_pResource( nullptr ), m_bExpanded( false ), m_bIsDirty( false ) { }

	kbPrefab *									m_pPrefab;
	kbResource *								m_pResource;
	std::string									m_FolderName;
	std::vector<kbResourceTabFile_t>			m_SubFolderList;
	std::vector<kbResourceTabFile_t>			m_ResourceList;

	bool										m_bExpanded;
	bool										m_bIsDirty;
};

/**
 *	kbResourceTab
 */
class kbResourceTab : public Fl_Tabs, kbWidget {

//---------------------------------------------------------------------------------------------------
public:

												kbResourceTab( int x, int y, int w, int h );

	virtual void								EventCB( const widgetCBObject * widgetCBObject );

	void										PostRendererInit();

	kbPrefab *									GetSelectedPrefab() const;

	void										AddPrefab( kbPrefab * prefab, const std::string & PackageName, const std::string & Folder, const std::string & PrefabName );
	void										MarkPrefabDirty( kbPrefab * prefab );

	void										RefreshResourcesTab();

private:

	Fl_Select_Browser *							m_pSelectBrowser;

	std::vector<kbResourceTabFile_t>			m_ResourceFolderList;
	std::vector<kbResourceTabFile_t*>			m_SelectBrowserIdx;		// Maps select browser entries to their corresponding kbResourceTabFile_t

	void										FindResourcesRecursively( const std::string & file, kbResourceTabFile_t & CurrentFolder );
	void										RefreshResourcesTab_Recursive( kbResourceTabFile_t & currentFolder, std::string spaces );

	// Callbacks
	static void									ResourceSelectedCB( Fl_Widget * widget, void * userData );
	static void									SavePackageCB( Fl_Widget * widget, void * userData );
};

#endif
