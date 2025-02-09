//===================================================================================================
// kbGame.h
//
//
/// 2016-2025 blk 1.0
//===================================================================================================
#pragma once

#include <windows.h>
#include "blk_core.h"
#include "blk_console.h"
#include "Matrix.h"
#include "kbCamera.h"
#include "kbParticleManager.h"
#include "kbInputManager.h"
#include "kbSoundManager.h"


 /// kbGame
class kbGame : public kbCommandProcessor {
public:
	kbGame();
	virtual	~kbGame();

	void Update();

	// Editor user
	void InitGame(HWND hwnd, const int width, const int height, const std::vector< const kbGameEntity* >& gameEntityList);
	void LoadMap(const std::string& mapName);
	void StopGame();

	bool IsPlaying() const { return m_bIsPlaying; }
	bool IsRunning() const { return m_bIsRunning; }
	void RequestQuitGame();

	const std::string& GetMapName() const { return m_MapName; }

	kbGameEntity* GetLocalPlayer() const { return m_pLocalPlayer; }

	const std::vector<kbGameEntity*>& GetGameEntities() const { return m_GameEntityList; }
	const std::vector<kbGameEntity*>& GetPlayersList() const { return m_GamePlayersList; }

	virtual kbGameEntity* CreatePlayer(const int netId, const kbGUID& prefabGUID, const Vec3& desiredLocation) = 0;
	kbGameEntity* CreateEntity(const kbGameEntity* const pPrefab, const bool bIsPlayer = false);
	void RemoveGameEntity(kbGameEntity* const pNewEntity);

	kbGameEntityPtr GetEntityByName(const kbString name);

	kbParticleManager& GetParticleManager() { return m_ParticleManager; }
	kbSoundManager& GetSoundManager() { return m_SoundManager; }

	bool ProcessCommand(const std::string& command);

	void SetDeltaTimeScale(const float newScale) { m_DeltaTimeScale = newScale; }

	float GetFrameDT() const { return m_CurFrameDeltaTime; }

	bool HasFirstSyncCompleted() const { return m_bHasFirstSyncCompleted; }

	// Hacks to get PIE style functionality
	virtual void HackEditorInit(HWND hwnd, std::vector<class kbEditorEntity*>& editorEntities) { }
	virtual void HackEditorUpdate(const float DT, kbCamera* const pCamera) { };
	virtual void HackEditorShutdown() { }

	template<typename T>
	T* GetLevelComponent() const {
		blk::error_check(m_pLevelComp != nullptr && m_pLevelComp->IsA(T::GetType()), "GetLevelComponent<T>() - Incorrect level component type");
		return (T*)m_pLevelComp;
	}

protected:
	virtual void init_internal() = 0;
	virtual void play_internal() = 0;
	virtual void stop_internal() = 0;
	virtual void preupdate_internal() { };
	virtual void postupdate_internal() { };
	virtual void level_loaded_internal() = 0;
	virtual void add_entity_internal(kbGameEntity* const pEntity) = 0;
	virtual void remove_entity_internal(kbGameEntity* const pEntity) = 0;

	const kbInput_t& get_input() const { return m_InputManager.get_input(); }
	bool is_console_active() const { return m_Console.IsActive(); }

	virtual void swap_entities_by_idx(const size_t idx1, const size_t idx2);

private:
	void display_debug_commands();

protected:
	HWND m_Hwnd;
	kbGameEntity* m_pLocalPlayer;
	kbTimer m_Timer;

	kbParticleManager m_ParticleManager;
	kbInputManager m_InputManager;
	kbSoundManager m_SoundManager;

private:
	std::string	m_MapName;
	kbLevelComponent* m_pLevelComp;

	std::vector<kbGameEntity*> m_GameEntityList;
	std::vector<kbGameEntity*> m_GamePlayersList;

	// List of entities to remove during the next kbGame::Update() call
	std::vector<kbGameEntity*> m_RemoveEntityList;

	kbConsole m_Console;

	float m_DeltaTimeScale;
	float m_CurFrameDeltaTime;

	bool m_bIsPlaying;
	bool m_bIsRunning;
	bool m_bQuitGameRequested;
	bool m_bHasFirstSyncCompleted;
};

extern kbGame* g_pGame;
