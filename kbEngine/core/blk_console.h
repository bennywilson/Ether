/// kbConsole.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "kbInputManager.h"

/// kbConsoleVarManager
class kbConsoleVarManager {
public:
	static kbConsoleVarManager* GetConsoleVarManager();
	static void DeleteConsoleVarManager();
	static class kbConsoleVariable* GetConsoleVar(const kbString&);

	void Initialize();

	void Update();

	void RegisterConsoleVar(kbString key, kbConsoleVariable* value) {
		m_ConsoleVarMap[key] = value;
	}

	const std::map<kbString, kbConsoleVariable* >& GetConsoleVarMap() const {
		return m_ConsoleVarMap;
	}

private:
	std::map<kbString, kbConsoleVariable*> m_ConsoleVarMap;
};

/// kbConsoleVariable
class kbConsoleVariable : public kbInputCallback {
public:
	enum varType_t {
		Console_Int,
		Console_Bool,
		Console_Float,
	};

	union varValue {
		varValue(const bool bInValue) : m_bValue(bInValue) { }
		varValue(const float fInValue) : m_fValue(fInValue) { }
		varValue(const int iInValue) : m_iValue(iInValue) { }

		int									m_iValue;
		bool								m_bValue;
		float								m_fValue;
	};

	template<typename U>
	kbConsoleVariable(std::string variableName, const U variableValue, const varType_t varType, const char* description, const char* inputKeys) :
		m_VarType(varType),
		m_Description(description),
		m_CurrentVal(variableValue) {

		std::transform(variableName.begin(), variableName.end(), variableName.begin(), ::tolower);

		kbConsoleVarManager* const pConsoleVarMgr = kbConsoleVarManager::GetConsoleVarManager();

		const kbString varName(variableName);
		if (pConsoleVarMgr->GetConsoleVarMap().find(varName) != pConsoleVarMgr->GetConsoleVarMap().end()) {
			blk::error("kbConsoleVariable %s already registered", variableName.c_str());
			return;
		}

		pConsoleVarMgr->RegisterConsoleVar(varName, this);//>GetConsoleVarMap()[variableName] = this;

		m_InputKeys = inputKeys;
	}

	void Initialize();

	int GetInt() const { return m_CurrentVal.m_iValue; }
	bool GetBool() const { return m_CurrentVal.m_bValue; }
	float GetFloat() const { return m_CurrentVal.m_fValue; }

	void SetInt(const int newInt) { m_CurrentVal.m_iValue = newInt; }
	void SetBool(const bool newBool) { m_CurrentVal.m_bValue = newBool; }
	void SetFloat(const float newFloat) { m_CurrentVal.m_fValue = newFloat; }

	varType_t GetType() const { return m_VarType; }

	const std::string& GetInputKeys() const { return m_InputKeys; }
	const std::string& GetDescription() const { return m_Description; }

private:
	virtual void InputKeyPressedCB(const int cbParam) override;
	virtual const char* GetInputCBName() const override;

	varValue m_CurrentVal;
	varType_t m_VarType;

	std::string	m_InputKeys;
	std::string m_Description;
};

/// kbCommandProcessor
class kbCommandProcessor {
public:
	virtual bool ProcessCommand(const std::string& dcommand) = 0;
};

/// kbConsole
class kbConsole {
public:
	kbConsole();
	~kbConsole();

	void SetActive(const bool bIsActive);
	bool IsActive() const { return m_bIsActive; }

	void Update(const float DT, const kbInput_t& Input);

	const std::string& GetCurrentCommandString() const { return m_CurrentCommand; }

	void RegisterCommandProcessor(kbCommandProcessor* const cmdProcessor) { m_CommandProcessors.push_back(cmdProcessor); }
	void RemoveCommandProcessor(kbCommandProcessor* const cmdProcessor) { m_CommandProcessors.erase(std::remove(m_CommandProcessors.begin(), m_CommandProcessors.end(), cmdProcessor), m_CommandProcessors.end()); }

private:
	std::string	m_CurrentCommand;
	std::vector<int> m_BufferedInputs;
	std::vector<kbCommandProcessor*> m_CommandProcessors;

	int	m_CommandHistoryIdx;
	static int const MaxCommandHistoryEntries = 8;
	std::vector<std::string> m_CommandHistory;

	float m_TimeSinceLastUpdate;
	bool m_bIsActive;
};
