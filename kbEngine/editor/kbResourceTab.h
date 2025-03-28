/// kbResourceTab.h
///
/// 2016-2025 blk 1.0

#pragma once

#pragma warning(push)
#pragma warning(disable:4312)
#include <fl/fl_tabs.h>
#pragma warning(pop)

/// kbResourceTabFile_t
struct kbResourceTabFile_t {
	kbResourceTabFile_t() : m_pPrefab(nullptr), m_pResource(nullptr), m_bExpanded(false), m_bIsDirty(false) { }

	kbPrefab* m_pPrefab;
	kbResource* m_pResource;
	std::string	m_FolderName;

	std::vector<kbResourceTabFile_t> m_SubFolderList;
	std::vector<kbResourceTabFile_t> m_ResourceList;

	bool m_bExpanded;
	bool m_bIsDirty;
};

/// kbResourceTab
class kbResourceTab : public Fl_Tabs, kbWidget {
public:
	kbResourceTab(int x, int y, int w, int h);
	~kbResourceTab();

	virtual void EventCB(const widgetCBObject* widgetCBObject);

	void PostRendererInit();

	kbPrefab* GetSelectedPrefab() const;
	kbGameEntityPtr GetSelectedGameEntity();

	void AddPrefab(kbPrefab* prefab, const std::string& PackageName, const std::string& Folder, const std::string& PrefabName);
	void MarkPrefabDirty(kbPrefab* prefab);

	void RefreshResourcesTab();
	void RefreshEntitiesTab();

private:

	void RebuildResourceFolderListText();
	unsigned int FontSize()	const { return 10; }

	Fl_Tabs* m_pOuterTab;

	Fl_Group* m_pResourceGroup;
	class Fl_Select_Browser* m_pResourceSelectBrowser;

	Fl_Group* m_pEntityGroup;
	Fl_Select_Browser* m_pEntitySelectBrowser;

	std::vector<kbResourceTabFile_t> m_ResourceFolderList;
	std::vector<kbResourceTabFile_t*> m_SelectBrowserIdx;		// Maps select browser entries to their corresponding kbResourceTabFile_t

	void FindResourcesRecursively(const std::string& file, kbResourceTabFile_t& CurrentFolder);
	void RefreshResourcesTab_Recursive(kbResourceTabFile_t& currentFolder, std::string spaces);

	struct EntitySelectItem_t {
		kbEditorEntity* m_pEntity;
	};
	std::vector<EntitySelectItem_t>	m_EntityList;

	// Callbacks
	static void	EntitySelectedCB(Fl_Widget* pWidget, void* pUserData);
	static void	ResourceSelectedCB(Fl_Widget* pWidget, void* pUserData);
	static void	SavePackageCB(Fl_Widget* pWidget, void* pUserData);
	static void	DeleteResouceCB(Fl_Widget* pWidget, void* pUserData);
	static void	DeleteCB(Fl_Widget* pWidget, void* pUserData);
	static void	ZoomToEntityCB(Fl_Widget* pWidget, void* pUserData);

	static void	ResourceManagerCB(const kbResourceManager::CallbackReason Reason);
};

extern kbResourceTab* g_pResourceTab;
