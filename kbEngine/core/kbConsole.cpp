/// kbConsole.cpp
///
/// 2016-2025 blk 1.0

#include <fstream>
#include "kbConsole.h"

/// kbConsoleVarManager::Initialize
void kbConsoleVariable::Initialize() {
	if (m_InputKeys.size() > 0) {
		g_pInputManager->MapKeysToCallback(m_InputKeys, this, 0, GetDescription() + ".  Also a CVar");
	}
}

/// kbConsoleVarManager::InputKeyPressedCB
void kbConsoleVariable::InputKeyPressedCB(const int cbParam) {
	if (m_VarType == Console_Bool) {
		m_CurrentVal.m_bValue = !m_CurrentVal.m_bValue;
	}
}

/// kbConsoleVarManager::GetInputCBName
const char* kbConsoleVariable::GetInputCBName() const {
	return "Console variable";
}

/// kbConsoleVarManager::GetConsoleVarManager
static kbConsoleVarManager* g_pConsoleVarManager = nullptr;
kbConsoleVarManager* kbConsoleVarManager::GetConsoleVarManager() {
	if (g_pConsoleVarManager == nullptr) {
		g_pConsoleVarManager = new kbConsoleVarManager();
	}

	return g_pConsoleVarManager;
}

/// kbConsoleVarManager::DeleteConsoleVarManager
void kbConsoleVarManager::DeleteConsoleVarManager() {
	delete g_pConsoleVarManager;
	g_pConsoleVarManager = nullptr;
}

/// kbConsoleVarManager::GetConsoleVar
kbConsoleVariable* kbConsoleVarManager::GetConsoleVar(const kbString& variableName) {
	kbConsoleVarManager* const pConsoleVarMgr = kbConsoleVarManager::GetConsoleVarManager();

	std::map<kbString, kbConsoleVariable* >::iterator it = pConsoleVarMgr->m_ConsoleVarMap.find(variableName);
	if (it == pConsoleVarMgr->m_ConsoleVarMap.end()) {
		return NULL;
	}

	return (*it).second;
}

/// kbConsoleVarManager::Initialize
void kbConsoleVarManager::Initialize() {
	std::map<kbString, kbConsoleVariable*>::iterator it = m_ConsoleVarMap.begin();
	while (it != m_ConsoleVarMap.end()) {
		it->second->Initialize();
		++it;
	}
}

/// kbConsoleVarManager::Update
void kbConsoleVarManager::Update() { }

/// kbConsole::kbConsole
const int StartingCommandHistoryIdx = -999;
kbConsole::kbConsole() :
	m_TimeSinceLastUpdate(0.0f),
	m_CommandHistoryIdx(0),
	m_bIsActive(false) {

	std::fstream commandHistoryFile;
	commandHistoryFile.open("logs/commandHistory.txt", std::fstream::in);
	if (commandHistoryFile.is_open()) {
		commandHistoryFile.seekg(0, commandHistoryFile.end);
		const size_t length = commandHistoryFile.tellg();
		commandHistoryFile.seekg(0, commandHistoryFile.beg);

		char* buffer = new char[length];
		commandHistoryFile.read(buffer, length);
		std::string sBuffer = buffer;
		delete[] buffer;

		size_t endString = sBuffer.find_first_of("\n", 0);
		size_t curString = 0;

		while (endString != std::string::npos) {
			std::string command = sBuffer.substr(curString, endString - curString);
			m_CommandHistory.push_back(command);

			curString = endString + 1;
			endString = sBuffer.find_first_of("\n", curString);
		}

		m_CommandHistoryIdx = StartingCommandHistoryIdx;
	}

	commandHistoryFile.close();
}

/// kbConsole::~kbConsole
kbConsole::~kbConsole() {
	std::fstream commandHistoryFile;
	commandHistoryFile.open("logs/commandHistory.txt", std::fstream::out);
	if (commandHistoryFile.is_open()) {
		for (int i = 0; i < m_CommandHistory.size(); i++) {
			commandHistoryFile.write(m_CommandHistory[i].c_str(), m_CommandHistory[i].length());
			commandHistoryFile.write("\n", 1);
		}
	}

}

/// kbConsole::SetActive
void kbConsole::SetActive(const bool bIsActive) {
	m_bIsActive = bIsActive;
}

/// kbConsole::Update
void kbConsole::Update(const float DT, const kbInput_t& Input) {

	if (Input.KeyState[192].m_Action == kbInput_t::KA_JustPressed) {
		m_bIsActive = !m_bIsActive;
	}

	if (m_bIsActive == false) {
		return;
	}

	m_TimeSinceLastUpdate = 0.0f;
	// DEL = 46
	// Enter = 13
	const float curTimeSec = g_GlobalTimer.TimeElapsedSeconds();
	const float minTimeBetweenPresses = 1.0f;

	for (int i = 0; i < 256; i++) {
		if (Input.KeyState[i].m_Action == kbInput_t::KA_JustPressed || (Input.KeyState[i].m_Action == kbInput_t::KA_Down && curTimeSec > Input.KeyState[i].m_LastActionTimeSec + minTimeBetweenPresses)) {
			if (i >= 65 && i <= 90) {						// A through Z ----------------------------------------------------------- */
				m_CurrentCommand += (char)i + 32;
			}
			else if (i == 46 || i == 8) {				// Del Pressed ----------------------------------------------------------- */				
				if (m_CurrentCommand.size() > 0) {
					m_CurrentCommand.pop_back();
				}
			}
			else if (i == 32) {							// Space Pressed -------------------------------------------------------- */
				m_CurrentCommand += " ";
			}
			else if (i == 13) {							// Enter Pressed -------------------------------------------------------- */
				for (int icmd = 0; icmd < m_CommandProcessors.size(); icmd++) {
					m_CommandProcessors[icmd]->ProcessCommand(m_CurrentCommand);
				}

				if (m_CurrentCommand.length() > 0 && m_CurrentCommand != "exit") {

					if (m_CommandHistory.size() == MaxCommandHistoryEntries) {
						m_CommandHistory.erase(m_CommandHistory.begin());
					}

					m_CommandHistory.push_back(m_CurrentCommand);
					m_CommandHistoryIdx = (int)m_CommandHistory.size();
				}

				m_CurrentCommand.clear();
			}
			else if (i >= 48 && i <= 57) {				// 0 - 9 Pressed -------------------------------------------------------- */
				m_CurrentCommand += std::to_string(i - 48);
			}
			else if (i == VK_UP) {
				if (m_CommandHistory.size() > 0) {
					if (m_CommandHistoryIdx == StartingCommandHistoryIdx) {
						m_CommandHistoryIdx = (int)m_CommandHistory.size() - 1;
					}
					else {
						m_CommandHistoryIdx--;
						if (m_CommandHistoryIdx < 0) {
							m_CommandHistoryIdx = (int)m_CommandHistory.size() - 1;
						}
					}
					m_CurrentCommand = m_CommandHistory[m_CommandHistoryIdx].c_str();
				}
			}
			else if (i == VK_DOWN) {					// Down Arrow ----------------------------------------------------------- */
				if (m_CommandHistory.size() > 0) {
					if (m_CommandHistoryIdx == StartingCommandHistoryIdx) {
						m_CommandHistoryIdx = (int)m_CommandHistory.size() - 1;
					}
					else {
						m_CommandHistoryIdx++;
						if (m_CommandHistoryIdx >= m_CommandHistory.size()) {
							m_CommandHistoryIdx = 0;
						}
					}
					m_CurrentCommand = m_CommandHistory[m_CommandHistoryIdx].c_str();
				}
			}
			else if (i == 190) {						// Period -------------------------------------------------------------- */
				m_CurrentCommand += ".";
			}
			else if (i == VK_OEM_MINUS) {				// Minus --------------------------------------------------------------- */
				m_CurrentCommand += "-";
			}
		}
	}
}
