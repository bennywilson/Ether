//===================================================================================================
// kbResourceManager.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbMaterial.h"
#include "kbModel.h"
#include "kbSoundManager.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbGameEntity.h"
#include <iostream>
#include <experimental/filesystem>

kbResourceManager g_ResourceManager;

namespace fs = std::experimental::filesystem;

/**
 *	kbLoadResourceJob
 */
class kbLoadResourceJob : public kbJob {
public:
	kbLoadResourceJob() :
		m_Resource( nullptr ) { }

	virtual void Run() {
		m_Resource->Load();
	}

	kbResource * m_Resource;
};

/**
 *	kbResource::Reload
 */
void kbResource::Load() {

	const float loadStartTime = g_GlobalTimer.TimeElapsedSeconds();

	if ( m_bIsLoaded == false ) { 
		if ( Load_Internal() ) {
			m_bIsLoaded = true;
		}
	}

	const float curLoadTime = g_GlobalTimer.TimeElapsedSeconds() - loadStartTime;
	static float totalLoadTime = 0.0f;
	totalLoadTime += curLoadTime;
	// kbLog( "It took %f seconds to load %s.  Total resource load time = %f", curLoadTime, GetFullFileName().c_str(), totalLoadTime );
}

/**
 *	kbResource::Release
 */
void kbResource::Release() {

	Release_Internal();
	m_bIsLoaded = false;
}

/**
 *	kbResourceManager::kbResourceManager
 */
kbResourceManager::kbResourceManager() {

	m_hGameAssetDirectory = CreateFile( "./assets/",
									GENERIC_READ | FILE_LIST_DIRECTORY, 
									FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
									nullptr, 
									OPEN_EXISTING,
									FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
									nullptr );


	m_hEngineAssetDirectory = CreateFile("../../kbEngine/assets/",
		GENERIC_READ | FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		nullptr);

	ZeroMemory( &m_Ovl, sizeof(m_Ovl) );
//	m_Ovl.hEvent = ::CreateEvent( nullptr, FALSE, FALSE, nullptr );
}

/**
 *	kbResourceManager::~kbResourceManaber
 */
kbResourceManager::~kbResourceManager() {
	Shutdown();
}

/**
 *	kbResourceManager::RenderSync
 */
void kbResourceManager::RenderSync() {
	for ( int i = 0; i < m_ResourcesToLoad.size(); i++ ) {
		m_ResourcesToLoad[i]->Load();
	}
	m_ResourcesToLoad.clear();

	UpdateHotReloads();
}

/**
 *	kbResourceManager::UpdateHotReloads
 */
void kbResourceManager::UpdateHotReloads() {

	static std::vector<std::wstring> queuedFiles;

	static float lastUpdateTimeSecs = 0;
	const float totalSeconds = g_GlobalTimer.TimeElapsedSeconds();
	if ( totalSeconds < lastUpdateTimeSecs + 0.05f ) {
		return;
	}
	lastUpdateTimeSecs = totalSeconds;

	// Handle queued up modified files
	if ( queuedFiles.size() > 0 ) {

		for ( int i = 0; i < queuedFiles.size(); i++ ) {
			const std::wstring & fileName = queuedFiles[i];
			FileModifiedCB( fileName );
		}

		for ( int i = 0; i < m_FunctionCallbacks.size(); i++ ) {
			m_FunctionCallbacks[i].m_pFunc( CBR_FileModified );
		}
	}
	queuedFiles.clear();

	static int states[] = {0,0};
	HANDLE handles[] = { m_hGameAssetDirectory, m_hEngineAssetDirectory };
	static byte * buffers[2] = { new byte[2048], new byte[2048] };
	DWORD numBytes = 0;

	for ( int i = 0; i < 2; i++ ) {
		if ( states[i] == 0 ) {
			BOOL result = ReadDirectoryChangesW( handles[i],
												 buffers[i],
												 2048,
												 TRUE,
												 FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
												 &numBytes,
												 &m_Ovl[i],
												 nullptr );

			if ( result == false ) {
				continue;
			}

			states[i] = 1;
		}

		FILE_NOTIFY_INFORMATION * pCurInfo = (FILE_NOTIFY_INFORMATION*)buffers[i];
		byte * pByteCurInfo = buffers[i];

		if ( GetOverlappedResult( handles[i], &m_Ovl[i], &numBytes, FALSE ) ) {
			while ( pCurInfo->Action != 0 ) {
				states[i] = 0;

				const DWORD FileNameLength = (pCurInfo->FileNameLength) / 2;
				std::wstring fileName;

				fileName.resize( FileNameLength );
				bool bHasTilda = false;
				for ( DWORD i = 0; i < FileNameLength; i++ ) {

					fileName[i] = pCurInfo->FileName[i];
					if (fileName[i] == '~' ) {
						bHasTilda = true;
						break;
					}
				}

				if ( bHasTilda == false && GetFileExtension( fileName ).empty() == false ) {
					std::replace( fileName.begin(), fileName.end(), '/', '\\' );
					std::wstring fullFileName;
					if ( i == 0 ) {
						fullFileName = L".\\assets\\" + fileName;
					} else {
						fullFileName = L"..\\..\\kbEngine\\assets\\" + fileName;
					}

					if ( VectorContains( queuedFiles, fullFileName ) == false ) {
						queuedFiles.push_back( fullFileName );
					} else {
						static int breakhere = 0;
						breakhere++;
					}
				}

				pCurInfo->Action = 0;
				pByteCurInfo += pCurInfo->NextEntryOffset;
				pCurInfo = (FILE_NOTIFY_INFORMATION*)pByteCurInfo;
			}
		}
	}
}

/**
 *	kbResourceManager::GetResource
 */
kbResource * kbResourceManager::GetResource( const std::string & fullFileName, const bool bLoadImmediately, const bool bLoadIfNotFound ) {

	if ( strcmp( fullFileName.c_str(), "nullptr" ) == 0 ) {
		return nullptr;
	}

	std::string convertedFileName = fullFileName;
	std::replace( convertedFileName.begin(), convertedFileName.end(), '/', '\\' );
	std::transform( convertedFileName.begin(), convertedFileName.end(), convertedFileName.begin(), ::tolower );
	const kbString fileNameString( convertedFileName );
	return GetResource( fileNameString, bLoadImmediately, bLoadIfNotFound );
}
/**
 *	kbResourceManager::GetResource
 */
kbResource * kbResourceManager::GetResource( const kbString & fullFileName, const bool bLoadImmediately, const bool bLoadIfNotFound ) {

	auto mapEntry = m_ResourcesMap.find( fullFileName );
	if ( mapEntry != m_ResourcesMap.end() ) {

		kbResource *const pResource = mapEntry->second;
		if ( bLoadImmediately && pResource->m_bIsLoaded == false ) {
			pResource->Load();
		}

		return pResource;
	} else if ( bLoadIfNotFound == false ) {
		return nullptr;
	}

	if ( fullFileName.GetLength() < 5 ) {
		kbWarning( "kbResourceManager::AddResource() - Invalid file name %s", fullFileName.c_str() );
		return nullptr;
	}

	kbResource * pResource = nullptr;
	std::string fileExt = GetFileExtension( fullFileName.c_str() );

	const std::string & stlFileName = fullFileName.stl_str();
	if ( stlFileName.find(".kbanim.ms3d") != std::string::npos ) {
		pResource = new kbAnimation();
	} else if ( fileExt == "ms3d" || fileExt == "fbx" || fileExt == "diablo3" ) {
	   pResource = new kbModel();
	} else if ( fileExt == "kbshader" ) {
	   pResource = new kbShader();
	} else if ( fileExt == "tif" || fileExt == "jpg" || fileExt == "tga" || fileExt == "bmp" || fileExt == "gif" || fileExt == "png" || fileExt == "dds" ) {
		pResource = new kbTexture();
	} else if ( fileExt == "kbanim" ) {
		pResource = new kbAnimation();
	} else if ( fileExt == "wav" ) {
		pResource = new kbWaveFile();
	}

	if ( pResource == nullptr ) {
		return nullptr;
	}

//	fs::path p = fs::canonical( fullFileName.c_str() );
	//StringFromWString( pResource->m_FullFileName, p.c_str() );
	pResource->m_FullFileName = stlFileName;
	pResource->m_FullName = fullFileName;//kbString( pResource->m_FullFileName );

	size_t pos = stlFileName.find_last_of( "/" );
	if ( pos != std::string::npos ) {
		pResource->m_Name = &stlFileName.c_str()[pos+1];
	} else {
		pResource->m_Name = stlFileName;
	}

	if ( bLoadImmediately ) {
		pResource->Load();
	}

	m_ResourcesMap[fullFileName] = pResource;

	return pResource;
}

/**
 *	kbResourceManager::AsyncLoadResource
 */
kbResource * kbResourceManager::AsyncLoadResource( const kbString & stringName ) {

	auto mapEntry = m_ResourcesMap.find( stringName );
	if ( mapEntry != m_ResourcesMap.end() ) {
		kbResource *const pResource = mapEntry->second;
		if ( pResource->m_bIsLoaded ) {
			return pResource;
		}

		// Check if resource is currently being loaded
		for ( int j = 0; j < m_LoadResourceJobs.size(); j++ ) {
			if ( m_LoadResourceJobs[j]->m_Resource == mapEntry->second ) {
				return nullptr;
			}
		}	

		// Create a new loading job for this resources
		kbLoadResourceJob *const pLoadJob = new kbLoadResourceJob();
		pLoadJob->m_Resource = pResource;
		m_LoadResourceJobs.push_back( pLoadJob );
		g_pJobManager->RegisterJob( pLoadJob );

		return nullptr;
	}

	kbWarning( "kbResourceManager::AsyncLoadResource() - Failed to kick off a job for %s", stringName.c_str() );
	return nullptr;
}

/**
 *	kbResourceManager::AddPrefab
 */
bool kbResourceManager::AddPrefab( kbGameEntity * pEntity, const std::string & PackageName, const std::string & Folder, const std::string & PrefabName, const bool bShouldOverwrite, kbPrefab ** prefab ) {

	const std::string fullPackageName = PackageName + ( ( GetFileExtension( PackageName ) == "kbPkg" ) ? ( "" ) : ( ".kbPkg" ) );

	kbPackage * pPackage = nullptr;
	for ( unsigned int i = 0; i < m_pPackages.size(); i++ ) {
		if ( m_pPackages[i]->m_PackageName == fullPackageName ) {
			pPackage = m_pPackages[i];
			break;
		}
	}

	if ( pPackage == nullptr ) {
		pPackage = new kbPackage();
		pPackage->m_PackageName = fullPackageName;
		m_pPackages.push_back( pPackage );
	}

	kbPrefab * pNewPrefab = nullptr;
	kbPackage::kbFolder * pFolder = nullptr;
	for ( unsigned int i = 0; i < pPackage->m_Folders.size(); i++ ) {
		if ( pPackage->m_Folders[i].m_FolderName == Folder ) {
			pFolder = &pPackage->m_Folders[i];

			for ( unsigned int j = 0; j < pFolder->m_pPrefabs.size(); j++ ) {
				if ( pFolder->m_pPrefabs[j]->m_PrefabName == PrefabName ) {
					if ( bShouldOverwrite ) {
						pNewPrefab = pFolder->m_pPrefabs[j];
						break;
					} else {
						return false;
					}
				}
			}
			break;
		}
	}

	if ( pFolder == nullptr ) {
		pPackage->m_Folders.push_back( kbPackage::kbFolder() );
		pFolder = &pPackage->m_Folders[pPackage->m_Folders.size() - 1];
		pFolder->m_FolderName = Folder;
	}

	if ( pNewPrefab == nullptr ) {
		pNewPrefab = new kbPrefab();
		pNewPrefab->m_PrefabName = PrefabName;
		pFolder->m_pPrefabs.push_back( pNewPrefab );
	} else {
		for ( int i = 0; i < pNewPrefab->m_GameEntities.size(); i++ ) {
			delete pNewPrefab->m_GameEntities[i];
		}
		pNewPrefab->m_GameEntities.clear();
	}

	kbGameEntity *const pNewEntity = new kbGameEntity( pEntity, true );
	pNewPrefab->m_GameEntities.push_back( pNewEntity );
	
	if ( prefab != nullptr ) {
		*prefab = pNewPrefab;
	}
	return true;
}

/**
 *
 */
void kbResourceManager::UpdatePrefab( const kbPrefab *const pPrefab, std::vector<kbGameEntity*> & pEntityList ) {
	if ( pPrefab == nullptr || pEntityList.size() == 0 || pPrefab->GetGameEntity(0) == nullptr ) {
		return;
	}

	const kbGUID guid = pPrefab->GetGameEntity(0)->GetGUID();

	kbPrefab *const updatedPrefab = const_cast<kbPrefab*>( pPrefab );
	for ( int i = 0; i < updatedPrefab->m_GameEntities.size(); i++ ) {
		delete updatedPrefab->m_GameEntities[i];
	}
	updatedPrefab->m_GameEntities.clear();

	for ( int i = 0; i < pEntityList.size(); i++ ) {
		kbGameEntity *const pNewEntity = new kbGameEntity( pEntityList[i], true, &guid );
		updatedPrefab->m_GameEntities.push_back( pNewEntity );
		kbLog( "Update prefab %d.  GUID is %d %d %d %d", (INT_PTR)pNewEntity, guid.m_iGuid[0], guid.m_iGuid[1], guid.m_iGuid[2], guid.m_iGuid[3] );
	}

	// Hack
	for ( int i = 0; i < m_pPackages.size(); i++ ) {
		if ( m_pPackages[i] == nullptr ) {
			continue;
		}
		SavePackage( m_pPackages[i]->GetPackageName() );
	}
}

/**
 *	kbResourceManager::GetPackage()
 */
kbPackage * kbResourceManager::GetPackage( const std::string & FullPackageName, const bool bLoadImmediately ) {

	const size_t packageNamePos = FullPackageName.find_last_of( "/" );
	std::string packageName = FullPackageName.substr( packageNamePos + 1 );
	for ( int i = 0; i < m_pPackages.size(); i++ ) {
		if ( m_pPackages[i]->m_PackageName == packageName ) {
			return m_pPackages[i];
		}
	}

	kbFile newFile;
	newFile.Open( FullPackageName, kbFile::kbFileType_t::FT_Read );
	kbPackage *const pPackage = newFile.ReadPackage( bLoadImmediately );
	newFile.Close();

	for ( int iFolder = 0; iFolder < pPackage->m_Folders.size(); iFolder++ ) {

		const std::vector< class kbPrefab * > & PrefabList = pPackage->m_Folders[iFolder].m_pPrefabs;
		for ( int iPrefab = 0; iPrefab < PrefabList.size(); iPrefab++ ) {
			m_GuidToEntityMap[PrefabList[iPrefab]->GetGameEntity(0)->GetGUID()] = PrefabList[iPrefab]->GetGameEntity(0);	
		}
	}

	m_pPackages.push_back( pPackage );
	
	return pPackage;
}

/**
 *	kbResourceManager::GetGameEntityFromGUID
 */
const kbGameEntity * kbResourceManager::GetGameEntityFromGUID( const kbGUID & GUID ) {
	std::map<kbGUID, const kbGameEntity * >::iterator it = m_GuidToEntityMap.find( GUID );
	if ( it == m_GuidToEntityMap.end() ) {
		return nullptr;
	}

	return it->second;
}

/**
 *	kbResourceManager::SavePackage()
 */
void kbResourceManager::SavePackage( const std::string & PackageName ) {
	for ( int i = 0; i < m_pPackages.size(); i++ ) {
		if ( PackageName == m_pPackages[i]->GetPackageName() ) {
			kbFile newFile;
			std::string PackageName = "assets/Packages/" + m_pPackages[i]->m_PackageName;
			if ( GetFileExtension( PackageName ) != "kbPkg" ) {
				PackageName += ".kbPkg";
			}
			newFile.Open( PackageName, kbFile::kbFileType_t::FT_Write );
			newFile.WritePackage( *m_pPackages[i] );
			newFile.Close();
			break;
		}
	}	
}

/**
 *	kbResourceManager::DumpPackageInfo
 */
void kbResourceManager::DumpPackageInfo() {
	for ( int i = 0; i < m_pPackages.size(); i++ ) {
		kbLog( "Package %s", m_pPackages[i]->m_PackageName.c_str() );
		for ( int j = 0; j < m_pPackages[i]->m_Folders.size(); j++ ) {
			kbLog( "	Folder %s", m_pPackages[i]->m_Folders[j].m_FolderName.c_str() );

			for ( int l = 0; l < m_pPackages[i]->m_Folders[j].m_pPrefabs.size(); l++ ) {
				kbLog( "		%s ", m_pPackages[i]->m_Folders[j].m_pPrefabs[l]->m_PrefabName.c_str() );
			}
		}
	}
}

/**
 *	kbResourceManager::Shutdown
 */
void kbResourceManager::Shutdown() {
	for ( auto it = m_ResourcesMap.begin(); it != m_ResourcesMap.end(); ++it ) {
		kbResource *const pResource = it->second;
		pResource->Release();
		delete pResource;
	}
	m_ResourcesMap.clear();

	for ( unsigned int i = 0; i < m_pPackages.size(); i++ ) {
		delete m_pPackages[i];
	}
	m_pPackages.clear();

	CloseHandle( m_hGameAssetDirectory );
	m_hGameAssetDirectory = nullptr;

	CloseHandle( m_hEngineAssetDirectory );
	m_hEngineAssetDirectory = nullptr;
}

/**
 *	kbResourceManager::FileModifiedCB
 */
void kbResourceManager::FileModifiedCB( const std::wstring & fileName ) {

	std::wstring convertedFileName = fileName;
	std::transform( convertedFileName.begin(), convertedFileName.end(), convertedFileName.begin(), ::tolower );
	fs::path p = fs::canonical( convertedFileName.c_str() );

	// TODO HOT RELOADING
	for ( auto it = m_ResourcesMap.begin(); it != m_ResourcesMap.end(); ++it ) {

		kbResource *const pCurResource = it->second;
		fs::path resourcePath = fs::canonical( pCurResource->GetFullFileName() );
		if ( resourcePath.string() == p.string() ) {
			kbLog( "Hot reloading %s", p.string().c_str() );
			pCurResource->Release();
			pCurResource->Load();
			return;
		}
	}

	kbLog( "Loading %s", p.string().c_str() );
	GetResource( p.string(), true, true );
}

/**
 *	kbResourceManager::RegisterCB
 */
void kbResourceManager::RegisterCB( ResourceManagerCB pFuncCB, const CallbackReason Reason ) {
	kbErrorCheck( pFuncCB != nullptr && Reason >= 0 && Reason < CBR_Max_Num_Reasons, "ResourceManager::RegisterCB() - Null func passed in" );

	CallbackInfo newCBInfo( pFuncCB, Reason );

	kbErrorCheck( VectorContains( m_FunctionCallbacks, newCBInfo ) == false, "ResourceManager::RegisterCB() - Registering function/reason multiple times" );
	m_FunctionCallbacks.push_back( newCBInfo );
}

/**
 *	kbResourceManager::UnregisterCB
 */
void kbResourceManager::UnregisterCB( ResourceManagerCB pFuncCB ) {
	kbErrorCheck( pFuncCB != nullptr, "ResourceManager::UnregisterCB() - Null func passed in" );

	for ( int i = 0; i < m_FunctionCallbacks.size(); i++ ) {
		if ( m_FunctionCallbacks[i].m_pFunc == pFuncCB ) {
			VectorRemoveFastIndex( m_FunctionCallbacks, i );
			i--;
		}
	}
}

/**
 *	kbPackage::~kbPackage
 */
kbPackage::~kbPackage() {
	for ( int i = 0; i < m_Folders.size(); i++ ) {
		for ( int j = 0; j < m_Folders[i].m_pPrefabs.size(); j++ ) {
			delete m_Folders[i].m_pPrefabs[j];
		}
		m_Folders[i].m_pPrefabs.clear();
	}
	m_Folders.clear();
}

/**
 *	kbPackage::GetPrefab
 */
const kbPrefab * kbPackage::GetPrefab( const std::string & PrefabName ) const {
	for ( int iFolder = 0; iFolder < m_Folders.size(); iFolder++ ) {

		const std::vector<kbPrefab *> & PrefabList = m_Folders[iFolder].m_pPrefabs;
		for ( int iPrefab = 0; iPrefab < PrefabList.size(); iPrefab++ ) {
			if ( PrefabList[iPrefab]->GetPrefabName() == PrefabName ) {
				return PrefabList[iPrefab];
			}	
		}
	}

	return nullptr;
}
