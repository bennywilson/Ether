/// kbResourceManager.h
///
/// 2016-2025 blk 1.0

#pragma once

/// kbResource
class kbResource {
	friend class kbResourceManager;

public:
	kbResource() { m_LastLoadTime = -1.0f, m_bIsLoaded = false; }
	virtual	~kbResource() = 0 { }

	virtual kbTypeInfoType_t GetType() const = 0;

	float GetLastLoadTime() const { return m_LastLoadTime; }

	void Load();
	void Release();		// note: It's preferable to use SAFE_RELEASE( kbResourceInstance ) instead of calling this directly

	const std::string& GetName() const { return m_Name; }
	const std::string& GetFullFileName() const { return m_FullFileName; }	// todo: deprecate
	const kbString& GetFullName() const { return m_FullName; }

protected:
	///  todo: remove
	virtual bool Load_Internal() { blk::warn("Make pure virtual"); return false; }
	virtual void Release_Internal() {}

	// todo: make pure virtual
	virtual bool load_internal() { blk::warn("Make pure virtual"); return false; }
	virtual void release_internal() { blk::warn("Make pure virtual"); }

	std::string	m_Name;
	std::string	m_FullFileName;
	kbString m_FullName;

	float m_LastLoadTime;

	bool m_bIsLoaded;
};

/// kbPackage
class kbPackage {
	friend class kbResourceManager;
	friend class kbFile;

public:
	const size_t NumFolders() const { return m_Folders.size(); }
	const std::string& GetFolderName(const int idx) const { return m_Folders[idx].m_FolderName; }
	const std::vector< class kbPrefab* >& GetPrefabsForFolder(const int idx) const { return m_Folders[idx].m_pPrefabs; }
	const std::string& GetPackageName() const { return m_PackageName; }
	const kbPrefab* GetPrefab(const std::string& PrefabName) const;

private:
	kbPackage() { }
	~kbPackage();

	std::string									m_PackageName;

	struct kbFolder {
		std::string								m_FolderName;
		std::vector<class kbPrefab*>			m_pPrefabs;
	};
	std::vector<kbFolder>						m_Folders;
};

/// kbResourceManager
class kbResourceManager {
public:
	kbResourceManager();
	~kbResourceManager();

	void RenderSync();

	kbResource* GetResource(const std::string& fullFileName, const bool bLoadImmediately, const bool bLoadIfNotFound);
	kbResource* GetResource(const kbString& fullFileName, const bool bLoadImmediately, const bool bLoadIfNotFound);

	kbResource* AsyncLoadResource(const kbString& stringName);

	bool AddPrefab(class kbGameEntity* pEntity, const std::string& package, const std::string& folder, const std::string& file, const bool bOverwrite, kbPrefab** prefab = NULL);
	void UpdatePrefab(const kbPrefab* const pPrefab, std::vector<kbGameEntity*>& pEntityList);

	kbPackage* GetPackage(const std::string& FullPackageName, const bool bLoadImmediately = true);
	void SavePackage(const std::string& PackageName);
	const kbGameEntity* GetGameEntityFromGUID(const kbGUID& GUID);

	const std::vector<kbPackage*>& GetPackageList() const { return m_pPackages; }

	void DumpPackageInfo();

	void Shutdown();

	enum CallbackReason {
		CBR_None = 0,
		CBR_FileModified,
		CBR_Max_Num_Reasons
	};
	typedef void (*ResourceManagerCB)(const CallbackReason reason);
	void RegisterCB(ResourceManagerCB  pFuncCB, const CallbackReason Reason);
	void UnregisterCB(ResourceManagerCB pFuncCB);

private:
	void UpdateHotReloads();

	void FileModifiedCB(const std::wstring& fileName);

	std::unordered_map<kbString, kbResource*, kbStringHash>	m_ResourcesMap;
	std::vector<kbResource*> m_ResourcesToLoad;		// Loaded during render sync

	std::vector<kbPackage*>	m_pPackages;
	std::map<kbGUID, const kbGameEntity*> m_GuidToEntityMap;

	std::vector<class kbLoadResourceJob*> m_LoadResourceJobs;

	// Hot reloading
	HANDLE m_hGameAssetDirectory;
	HANDLE m_hEngineAssetDirectory;
	OVERLAPPED m_Ovl[2];

	struct CallbackInfo {
		CallbackInfo(ResourceManagerCB inFunc, const CallbackReason reason) : m_pFunc(inFunc), m_CBReason(reason) { }

		ResourceManagerCB m_pFunc;
		CallbackReason m_CBReason;

		bool operator==(const CallbackInfo& rhs) const { return m_pFunc == rhs.m_pFunc && m_CBReason == rhs.m_CBReason; }
	};
	std::vector<CallbackInfo> m_FunctionCallbacks;
};

extern kbResourceManager g_ResourceManager;
