//===================================================================================================
// kbGame.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include <sstream>
#include <iomanip>
#include "kbGame.h"
#include "DX11/kbRenderer_DX11.h"		// TODO

kbGame * g_pGame = nullptr;

kbConsoleVariable g_ShowPerfTimers( "showperftimers", false, kbConsoleVariable::Console_Bool, "Display game/engine perf timers", "ctrl p" );
kbConsoleVariable g_ShowEntityInfo( "showentityinfo", false, kbConsoleVariable::Console_Bool, "Show entity info?", "" );
kbConsoleVariable g_DumpEntityInfo( "dumpentityinfo", false, kbConsoleVariable::Console_Bool, "Dump entity info?", "" );
kbConsoleVariable g_TimeScale( "timescale", (float)1.0f, kbConsoleVariable::Console_Float, "Dilate time", "" );
kbConsoleVariable g_EnableHelpScreen( "help", false, kbConsoleVariable::Console_Bool, "Display help screen", "ctrl h" );
kbConsoleVariable g_ShowFPS( "showfps", false, kbConsoleVariable::Console_Bool, "Show FPS", "ctrl f" );

/**
 *	kbGame::kbGame
 */
kbGame::kbGame() :
	m_Hwnd( nullptr ),
	m_pLocalPlayer( nullptr ),
	m_pLevelComp( nullptr ),
	m_bIsPlaying( false ),
	m_bIsRunning( true ),
	m_bHasFirstSyncCompleted( false ),
	m_pParticleManager( new kbParticleManager() ),
	m_DeltaTimeScale( 1.0f ),
	m_CurFrameDeltaTime( 0.0f ) {

	g_pGame = this;
	m_Console.RegisterCommandProcessor( this );
}

/**
 *	kbGame::~kbGame
 */
kbGame::~kbGame() {
	delete m_pParticleManager;
	m_pParticleManager = nullptr;
	m_Console.RemoveCommandProcessor( this );
}

/**
 *  kbGame::InitGame
 */
void kbGame::InitGame( HWND hwnd, const int backBufferWidth, const int backBufferHeight, const std::vector< const kbGameEntity * > & gameEntityList ) {

	m_Hwnd = hwnd;

	m_InputManager.Init( m_Hwnd );
	kbConsoleVarManager::GetConsoleVarManager()->Initialize();

	InitGame_Internal();
}

/**
 *  kbGame::LoadMap
 */
void kbGame::LoadMap( const std::string & mapName ) {
	kbLog( "LoadMap() called on %s", mapName.c_str() );

	m_pLevelComp = nullptr;

	// Load map
	if ( mapName.empty() == false ) {

		TCHAR NPath[MAX_PATH];

		GetCurrentDirectory(MAX_PATH, NPath);

		WIN32_FIND_DATA fdFile;
		HANDLE hFind = nullptr;

		std::string LevelPath = NPath;
		LevelPath += "/Assets/Levels/";
		std::string curLevelFolder = "";

		m_MapName = mapName;
		if ( m_MapName.find( "." ) == std::string::npos ) {
			m_MapName += ".kbLevel";
		}	

		hFind = FindFirstFile( ( LevelPath + "*" ).c_str(), &fdFile );
		BOOL nextFileFound = ( hFind != INVALID_HANDLE_VALUE  );
		do {
			const std::string fullFilePath = LevelPath + curLevelFolder + m_MapName;
				
			kbFile inFile;		
			if ( inFile.Open( fullFilePath.c_str(), kbFile::FT_Read ) ) {

				StopGame();
				g_pRenderer->WaitForRenderingToComplete();

				for ( int i = 0; i < m_GameEntityList.size(); i++ ) {
					m_GameEntityList[i]->RenderSync();
				}

				m_pParticleManager->RenderSync();

				kbGameEntity * gameEntity = inFile.ReadGameEntity();
				while ( gameEntity != nullptr ) {

					if ( m_pLevelComp == nullptr ) {
						m_pLevelComp = gameEntity->GetComponent<kbLevelComponent>();
					}
					m_GameEntityList.push_back( gameEntity );
					gameEntity = inFile.ReadGameEntity();
				}
				inFile.Close();

				LevelLoaded_Internal();

				m_Timer.Reset();
				PlayGame_Internal();
				m_bIsPlaying = true;

				break;
			}

			if ( nextFileFound == FALSE ) {
				break;
			}

			do {
				if ( ( fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 && strcmp( fdFile.cFileName, "." ) != 0 && strcmp( fdFile.cFileName, ".." ) != 0 ) {
					curLevelFolder = "/";
					curLevelFolder += fdFile.cFileName;
					curLevelFolder += "/";
					break;
				}

			} while( nextFileFound = FindNextFile( hFind, &fdFile ) != FALSE );

			if ( nextFileFound != false ) {
				nextFileFound = FindNextFile( hFind, &fdFile );
			}
		} while( true );
	}
}

/**
 *  kbGame::StopGame
 */
void kbGame::StopGame() {
	StopGame_Internal();

	if ( g_pRenderer != nullptr ) {
		g_pRenderer->WaitForRenderingToComplete();
	}

	for ( int i = 0; i < m_GameEntityList.size(); i++ ) {
		delete m_GameEntityList[i];
	}
	m_GameEntityList.clear();
	m_GamePlayersList.clear();

	m_bIsPlaying = false;

	m_pLevelComp = nullptr;
}

/**
 *  kbGame::RequestQuitGame
 */
void kbGame::RequestQuitGame() {

	StopGame();
	m_bIsRunning = false;
}

/**
 *	kbGame::Update
 */
void kbGame::Update() {

	START_SCOPED_TIMER(GAME_THREAD);

	m_CurFrameDeltaTime = m_Timer.TimeElapsedSeconds() * m_DeltaTimeScale;
	m_Timer.Reset();

	static int NumFrames = 0;
	static float StartTime = g_GlobalTimer.TimeElapsedSeconds();

	NumFrames++;
	static float FPS = 0;

	if ( NumFrames > 100 ) {
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		FPS = (float)NumFrames / ( curTime - StartTime );
		NumFrames = 0;
		StartTime = curTime;
	}

	if ( g_TimeScale.GetFloat() > 0.0f ) {
		m_CurFrameDeltaTime *= g_TimeScale.GetFloat();
	}

	m_InputManager.Update( m_CurFrameDeltaTime );
	m_SoundManager.Update();

	PreUpdate_Internal();

	for ( int i = 0; i < m_GameEntityList.size(); i++ ) {
		m_GameEntityList[i]->Update( m_CurFrameDeltaTime );
	}

	PostUpdate_Internal();

	if ( g_pRenderer != nullptr ) {

		const float fontHeight = ( 16.0f ) / g_pRenderer->GetBackBufferHeight();

		g_pRenderer->EnableConsole( false );

		if ( m_Console.IsActive() ) {
			g_pRenderer->EnableConsole( true );
			g_pRenderer->DrawDebugText( m_Console.GetCurrentCommandString().c_str() + std::string( "_" ), 0, 0.75f - fontHeight, 0.0125f, 0.0125f, kbColor::green );

			if ( g_EnableHelpScreen.GetBool() ) {
				DisplayDebugCommands();
			}
		}
		g_pRenderer->SetRenderWindow( m_Hwnd );

		{
			START_SCOPED_TIMER( GAME_THREAD_IDLE );

			// Wait for rendering to complete, sync up any game objects that need it and kick off a new scene to render
			g_pRenderer->WaitForRenderingToComplete();
		}

		{
			START_SCOPED_TIMER( RENDER_SYNC );

			for ( int i = 0; i < m_GameEntityList.size(); i++ ) {
				m_GameEntityList[i]->RenderSync();
			}

			m_pParticleManager->RenderSync();
			g_pRenderer->RenderSync();


			for ( int i = 0; i < m_RemoveEntityList.size(); i++ ) {
				std::vector<kbGameEntity*>::iterator it;
				it = find( m_GameEntityList.begin(), m_GameEntityList.end(), m_RemoveEntityList[i] );

				if ( it != m_GameEntityList.end() ) {
					const int index = (int)( it - m_GameEntityList.begin() );
					std::swap( m_GameEntityList[index], m_GameEntityList.back() );
					m_GameEntityList.pop_back();
					delete m_RemoveEntityList[i];
				}
			}
		}

		if ( g_ShowFPS.GetBool() ) {
			std::string fpsString = "FPS: ";
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << FPS;
			fpsString += stream.str();
			g_pRenderer->DrawDebugText( fpsString, 0.85f, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );
		}
		if ( g_ShowPerfTimers.GetBool() ) {

			float curY = g_DebugLineSpacing + 0.1f;
			for ( int i = 0; i < (int)MAX_NUM_SCOPED_TIMERS; i++, curY += g_DebugLineSpacing ) {
				const kbScopedTimerData_t & timingData = GetScopedTimerData( (ScopedTimerList_t)i );
				std::string timing = timingData.m_ReadableName.stl_str();
				timing += ": ";
	
				std::stringstream stream;
				stream << std::fixed << std::setprecision(3) << timingData.GetFrameTime();
				timing += stream.str();
		
				g_pRenderer->DrawDebugText( timing, 0.25f, curY, g_DebugTextSize, g_DebugTextSize, kbColor::green );
			}
		} else if ( g_ShowEntityInfo.GetBool()  || g_DumpEntityInfo.GetBool() ) {
			float curY = g_DebugLineSpacing + 0.1f;
			std::string NumEntities = "Num Entities: ";
			NumEntities += std::to_string( (long long ) m_GameEntityList.size() );
			g_pRenderer->DrawDebugText( NumEntities, 0.25f, curY, g_DebugTextSize, g_DebugTextSize, kbColor::green );
			curY += g_DebugLineSpacing;
			std::map<std::string, int> componentMap;

			if ( g_DumpEntityInfo.GetBool() ) {
				kbLog( "=============================================================================" );
				kbLog( "Dumping entity and component info" );

			}

			for ( int i = 0; i < (int)m_GameEntityList.size(); i++ ) {
				const kbGameEntity *const pCurEntity = m_GameEntityList[i];

				if ( g_DumpEntityInfo.GetBool() ) {
					kbLog( "Entity [%d] - %s", i, pCurEntity->GetName().c_str() );
				}

				for ( int iComp = 0; iComp < pCurEntity->NumComponents(); iComp++ ) {
					kbComponent *const pComponent = pCurEntity->GetComponent( iComp );
					const std::string pComponentTypeName = pComponent->GetComponentClassName();
					componentMap[pComponentTypeName]++;

					if ( g_DumpEntityInfo.GetBool() ) {
						kbTransformComponent * pTransformComponent = pComponent->GetAs<kbTransformComponent>();
						if ( pTransformComponent != nullptr ) {
							kbLog( "	Component [%d] - %s %s", iComp, pComponentTypeName.c_str(), pTransformComponent->GetName().c_str() );
						} else {
							kbLog( "	Component [%d] - %s", iComp, pComponentTypeName.c_str());
						}
					}

				}
			}

			g_DumpEntityInfo.SetBool( false );

			for ( std::map<std::string,int>::iterator it = componentMap.begin(); it != componentMap.end(); ++it ) {
				std::string outputName = it->first;
				outputName += ": ";
				outputName += std::to_string( (long long) it->second );
				g_pRenderer->DrawDebugText( outputName, 0.25f, curY, g_DebugTextSize, g_DebugTextSize, kbColor::green );
				curY += g_DebugLineSpacing;

			}
		}

		UpdateScopedTimers();
		m_bHasFirstSyncCompleted = true;
		g_pRenderer->SetReadyToRender();

		m_RemoveEntityList.clear();
	} else {

		for ( int i = 0; i < m_RemoveEntityList.size(); i++ ) {
			std::vector<kbGameEntity*>::iterator it;
			it = find( m_GameEntityList.begin(), m_GameEntityList.end(), m_RemoveEntityList[i] );
		
			if ( it != m_GameEntityList.end() ) {
				const int index = (int)( it - m_GameEntityList.begin() );
				std::swap( m_GameEntityList[index], m_GameEntityList.back() );
				m_GameEntityList.pop_back();
				delete m_RemoveEntityList[i];
			}
		}

		m_RemoveEntityList.clear();
	}

	kbConsoleVarManager::GetConsoleVarManager()->Update();
	m_Console.Update( m_CurFrameDeltaTime, m_InputManager.GetInput() );
}

/**
 *	kbGame::CreateEntity
 */
kbGameEntity * kbGame::CreateEntity( const kbGameEntity *const pPrefab, const bool bIsPlayer ) {
	if ( pPrefab == nullptr ) {
		kbError( "kbGame::CreateEntity() - nullptr prefab passed in" );
		return nullptr;
	}

	kbGameEntity *const pSpawnedEntity = new kbGameEntity( pPrefab, false, nullptr );
	m_GameEntityList.push_back( pSpawnedEntity );

	if ( bIsPlayer ) {
		m_GamePlayersList.push_back( pSpawnedEntity );
	}

	AddGameEntity_Internal( pSpawnedEntity );

	return pSpawnedEntity;
}

/**
 *	kbGame::RemoveGameEntity
 */
void kbGame::RemoveGameEntity( kbGameEntity *const pEntityToRemove ) {

	RemoveGameEntity_Internal( pEntityToRemove );

	std::vector<kbGameEntity*>::iterator it;
	it = find( m_GameEntityList.begin(), m_GameEntityList.end(), pEntityToRemove );

	if ( it != m_GameEntityList.end() ) {
		pEntityToRemove->DisableAllComponents();
		m_RemoveEntityList.push_back( pEntityToRemove );
	} else {
		it = find( m_GamePlayersList.begin(), m_GamePlayersList.end(), pEntityToRemove );	
		if ( it != m_GamePlayersList.end() ) {
			pEntityToRemove->DisableAllComponents();
			m_RemoveEntityList.push_back( pEntityToRemove );
		}
	}

}

/**
 *	kbGame::SwapEntitiesByIdx
 */
void kbGame::SwapEntitiesByIdx( const size_t idx1, const size_t idx2 ) {
	if ( idx1 < 0 || idx1 >= m_GameEntityList.size() || idx2 < 0 || idx2 >= m_GameEntityList.size() ) {
		kbWarning( "kbGame::SwapEntitiesByIdx() - Invalid index(es) [%d], [%d]", idx1, idx2 );
		return;
	}

	std::swap( m_GameEntityList[idx1], m_GameEntityList[idx2] ); 
}

/**
 *	kbGame::ProcessCommand
 */
bool kbGame::ProcessCommand( const std::string & InCommand ) {

	std::string command = InCommand;
	std::transform( command.begin(), command.end(), command.begin(), ::tolower );
	
	// Get parameters
	std::vector<std::string> commandParams;

	size_t endString = command.find_first_of( " ", 0 );
	std::string finalCommand = command.substr( 0, endString );

	while ( endString != std::string::npos ) {
		std::string param = command.substr( endString + 1, command.size() );
		if ( param[0] != ' ' ) {
			std::transform( param.begin(), param.end(), param.begin(), ::tolower );
			commandParams.push_back( param );
		}

		endString = command.find_first_of( " ", endString + 1 ); 
	}

	kbConsoleVarManager *const pConsoleVarMgr = kbConsoleVarManager::GetConsoleVarManager();
	kbConsoleVariable *const pConsoleVar = pConsoleVarMgr->GetConsoleVar( kbString( finalCommand.c_str() ) );

	if ( finalCommand == "help" ) {
		g_EnableHelpScreen.SetBool( !g_EnableHelpScreen.GetBool() );
		return true;
	} else if ( pConsoleVar == nullptr ) {

		if ( finalCommand == "open" ) {
			if ( commandParams.size() > 0 ) {
				LoadMap( commandParams[0] );
			}
		} else if ( finalCommand == "exit" ) {
			RequestQuitGame();
		}
		return true;
	}

	if ( commandParams.size() == 0 ) {
		return false;
	}

	if ( pConsoleVar->GetType() == kbConsoleVariable::Console_Float ) {
		const float val = std::stof( commandParams[0] );
		pConsoleVar->SetFloat( val );
	} else {

		int val = 0;

		if ( commandParams[0].find_first_not_of( "0123456789" ) == std::string::npos ) {
			val = std::stoi( commandParams[0] );
		} else if ( commandParams[0] == "true" ) {
			val = 1;
		} else {
			val = 0;
		}

		pConsoleVar->SetInt( val );
	}

	return true;
}

/**
 *	kbGame::DisplayDebugCommands
 */
void kbGame::DisplayDebugCommands() {

	const float aspectRatio = (float)g_pRenderer->GetBackBufferWidth() / (float)g_pRenderer->GetBackBufferHeight();
	const float fontScreenSizeX = 0.02f;
	const float fontScreenSizeY = fontScreenSizeX * aspectRatio;
	const float spaceSize = fontScreenSizeY * 0.5f;

	g_pRenderer->DrawDebugText( "Help Screen", 0.0f, 0.0f, fontScreenSizeX, fontScreenSizeY, kbColor::green );

	float curScreenY = fontScreenSizeY + spaceSize;

	g_pRenderer->DrawDebugText( "CVars:", 0.0f, curScreenY, fontScreenSizeX, fontScreenSizeY, kbColor::red );
	curScreenY += spaceSize;

	auto consoleVarMap = kbConsoleVarManager::GetConsoleVarManager()->GetConsoleVarMap();
	auto consoleVarIt = consoleVarMap.begin();

	while( consoleVarIt != consoleVarMap.end() ) {
		const kbString consoleVarName = consoleVarIt->first;
		g_pRenderer->DrawDebugText( consoleVarName.stl_str() + ":" , 0.0f, curScreenY, fontScreenSizeX, fontScreenSizeY, kbColor::green );
		g_pRenderer->DrawDebugText( consoleVarIt->second->GetDescription(), 0.25f, curScreenY, fontScreenSizeX, fontScreenSizeY, kbColor::red );

		curScreenY += spaceSize;
		consoleVarIt++;
	}

	curScreenY += spaceSize;
	g_pRenderer->DrawDebugText( "Short-cut Keys:", 0.0f, curScreenY, fontScreenSizeX, fontScreenSizeY, kbColor::red );
	curScreenY += spaceSize;

	KeyComboMapType keyComboMap = g_pInputManager->GetKeyComboMap();
	KeyComboMapType::iterator it = keyComboMap.begin();

	while( it != keyComboMap.end() ) {
		g_pRenderer->DrawDebugText( it->second.m_HelpDescription + ": " , 0.0f, curScreenY, fontScreenSizeX, fontScreenSizeY, kbColor::green );
		g_pRenderer->DrawDebugText( it->second.m_KeyComboDisplayString, 0.5f, curScreenY, fontScreenSizeX, fontScreenSizeY, kbColor::red );
		it++;
		curScreenY += spaceSize;
	}
}