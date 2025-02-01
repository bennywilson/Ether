/// kbInputManager.h
///
/// 2017-2019 kbEngine 2.0

#pragma once

#include<Windows.h>
#include<Xinput.h>
#include<unordered_map>
#include "kbCore.h"
#include "kbVector.h"

/// kbInputCallback
class kbInputCallback {
	friend class kbInputManager;

private:
	virtual void InputKeyPressedCB(const int cbParam) = 0;
	virtual const char* GetInputCBName() const = 0;
};


///	KeyComboBitField_t - Represents a combination of pressed keys
struct KeyComboBitField_t {
	KeyComboBitField_t() :
		m_Bits0(0),
		m_Bits1(0),
		m_Ctrl(false),
		m_Shift(false) { }

	bool operator==(const KeyComboBitField_t& rhs) const {
		return m_Bits0 == rhs.m_Bits0 && m_Bits1 == rhs.m_Bits1 && m_Ctrl == rhs.m_Ctrl && m_Shift == rhs.m_Shift;
	}

	__int64	m_Bits0;
	__int64	m_Bits1;
	bool m_Ctrl;
	bool m_Shift;
};

///	KeyComboBitFieldHash_t
struct KeyComboBitFieldHash_t {
	std::size_t operator() (const KeyComboBitField_t& rhs) const {
		return rhs.m_Bits0 ^ rhs.m_Bits1 ^ ((__int64)rhs.m_Ctrl << 63) ^ ((__int64)rhs.m_Shift << 62);
	}
};

///	InputCallbackInfo_t - Callback used when a combination of keys matches a KeyComboBitField_t
struct InputCallbackInfo_t {
	InputCallbackInfo_t() :
		m_CallbackObject(nullptr),
		m_CallbackParam(0) { }

	kbInputCallback* m_CallbackObject;
	int	m_CallbackParam;
	std::string	m_HelpDescription;
	std::string	m_KeyComboDisplayString;
};

typedef std::unordered_map<KeyComboBitField_t, InputCallbackInfo_t, KeyComboBitFieldHash_t> KeyComboMapType;

typedef DWORD(WINAPI* LPXINPUTGETSTATE)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD(WINAPI* LPXINPUTSETSTATE)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
typedef DWORD(WINAPI* LPXINPUTGETCAPABILITIES)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
typedef void (WINAPI* LPXINPUTENABLE)(BOOL bEnable);
typedef DWORD(WINAPI* LPXINPUTGETSTATE)(DWORD dwUserIndex, XINPUT_STATE* pState);

/// IInputListener - inherit to make your class a listener for key combo presses
class IInputListener abstract {
	friend class kbInputManager;

protected:
	virtual void InputCB(const kbInput_t& input) = 0;
};

///  kbInputManager
class kbInputManager {
public:
	kbInputManager();
	~kbInputManager();

	const kbInput_t& GetInput() const { return m_Input; }

	void Init(HWND Hwnd);
	void Update(const float Delta);
	void UpdateKey(const uint keyPress);

	void MapKeysToCallback(const std::string& stringCombo, kbInputCallback* const pCB, int CallbackParam, const std::string& helpDescription = "");
	void UnmapCallback(kbInputCallback* const pCB);

	const KeyComboMapType& GetKeyComboMap() const { return m_KeyComboToCallbackMap; }

	enum eMouseBehavior_t {
		MB_LockToCenter,
		MB_LockToWindow
	};

	void										SetMouseBehavior(const eMouseBehavior_t newMouseBehavior) { m_MouseBehavior = newMouseBehavior; }
	eMouseBehavior_t							GetMouseBehavior() const { return m_MouseBehavior; }

	kbVec2i										GetMouseCursorPosition() const { return kbVec2i(m_Input.AbsCursorX, m_Input.AbsCursorY); }

	void										RegisterInputListener(IInputListener* const pListener);
	void										UnregisterInputListener(IInputListener* const pListener);

private:

	LPXINPUTENABLE								m_FuncXInputEnable;
	LPXINPUTGETSTATE							m_FuncXInputGetState;
	HWND										m_Hwnd;

	kbInput_t									m_Input;

	KeyComboBitField_t							m_KeyComboBitField;

private:
	KeyComboMapType								m_KeyComboToCallbackMap;

	eMouseBehavior_t							m_MouseBehavior;

	std::vector<IInputListener*>				m_InputListeners;
};

extern kbInputManager* g_pInputManager;

