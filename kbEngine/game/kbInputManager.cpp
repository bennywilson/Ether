/// kbInputManager.cpp
///
/// 2017-2025 blk 1.0

#include <sstream>
#include "blk_core.h"
#include "blk_containers.h"
#include "kbInputManager.h"

kbInputManager* g_pInputManager = nullptr;

///	kbInputManager::kbInputManager
kbInputManager::kbInputManager() :
	m_FuncXInputEnable(nullptr),
	m_FuncXInputGetState(nullptr),
	m_Hwnd(nullptr),
	m_MouseBehavior(MB_LockToCenter) {

	if (g_pInputManager != nullptr) {
		blk::error("kbInputManager::kbInputManager() - g_pInputManager has already been set");
	}

	m_Input.m_LeftStick.set(0.0f, 0.0f);
	m_Input.m_PrevLeftStick.set(0.0f, 0.0f);
	m_Input.m_RightStick.set(0.0f, 0.0f);
	m_Input.m_PrevRightStick.set(0.0f, 0.0f);

	g_pInputManager = this;
}

///	kbInputManager::~kbInputManager
kbInputManager::~kbInputManager() {
	if (g_pInputManager == nullptr) {
		blk::error("kbInputManager::~kbInputManager() - g_pInputManager was already nullptr");
	}

	g_pInputManager = nullptr;
}

///	kbInputManager::Init
void kbInputManager::Init(HWND Hwnd) {
	m_Hwnd = Hwnd;

	if (m_FuncXInputEnable == nullptr) {
		HINSTANCE hInst = LoadLibrary(XINPUT_DLL);
		if (hInst) {
			m_FuncXInputEnable = (LPXINPUTENABLE)GetProcAddress(hInst, "XInputEnable");
			m_FuncXInputGetState = (LPXINPUTGETSTATE)GetProcAddress(hInst, "XInputGetState");
		}

		if (m_FuncXInputEnable) {
			m_FuncXInputEnable(true);
		}
	}
}

///	kbInputManager::UpdateKey
void kbInputManager::UpdateKey(const uint keyPress) {}

///	kbInputManager::Update
void kbInputManager::Update(const float DeltaTime) {
	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if (bFirstRun) {
		ShowCursor(false);
		bFirstRun = false;
	}

	if (g_UseEditor == false) {
		if (GetForegroundWindow() != m_Hwnd) {
			if (bWindowIsSelected) {
				bWindowIsSelected = false;
				ShowCursor(true);
			}
			return;
		}
	}

	XINPUT_STATE InputState = { 0 };

	m_Input.m_PrevLeftStick = m_Input.m_LeftStick;
	m_Input.m_PrevRightStick = m_Input.m_RightStick;

	m_Input.m_LeftStick.set(0.0f, 0.0f);
	m_Input.m_RightStick.set(0.0f, 0.0f);

	m_Input.MouseDeltaX = 0;
	m_Input.MouseDeltaY = 0;

	for (int i = 0; i < 4; i++) {
		if (m_FuncXInputGetState && m_FuncXInputGetState(i, &InputState) == ERROR_SUCCESS) {
			XINPUT_GAMEPAD& pGamePad = InputState.Gamepad;

			if (abs(pGamePad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
				m_Input.m_LeftStick.x = kbSaturate(pGamePad.sThumbLX / 32767.0f) * 2.0f - 1.0f;
			}

			if (abs(pGamePad.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
				m_Input.m_LeftStick.y = kbSaturate(pGamePad.sThumbLY / 32767.0f) * 2.0f - 1.0f;
			}

			if (abs(pGamePad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
				m_Input.m_RightStick.x = kbSaturate(pGamePad.sThumbRX / 32767.0f) * 2.0f - 1.0f;
			}

			if (abs(pGamePad.sThumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
				m_Input.m_RightStick.y = kbSaturate(pGamePad.sThumbRY / 32767.0f) * 2.0f - 1.0f;
			}

			if (pGamePad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
				m_Input.LeftTrigger = (float)pGamePad.bLeftTrigger / (float)255;
			}
			else {
				m_Input.LeftTrigger = 0.0f;
			}

			if (pGamePad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
				if (m_Input.RightTrigger == 0.0f) {
					m_Input.RightTriggerPressed = true;
				}
				else {
					m_Input.RightTriggerPressed = false;
				}
				m_Input.RightTrigger = (float)pGamePad.bRightTrigger / (float)255;

			}
			else {
				m_Input.RightTriggerPressed = false;
				m_Input.RightTrigger = 0.0f;
			}

			for (uint iButton = 0; iButton < 16; iButton++) {
				if (pGamePad.wButtons & (1 << iButton)) {

					if (m_Input.GamepadButtonStates[iButton].m_Action == kbInput_t::KA_JustPressed) {
						m_Input.GamepadButtonStates[iButton].m_Action = kbInput_t::KA_Down;
					}
					else if (m_Input.GamepadButtonStates[iButton].m_Action != kbInput_t::KA_Down) {
						m_Input.GamepadButtonStates[iButton].m_Action = kbInput_t::KA_JustPressed;
						m_Input.GamepadButtonStates[iButton].m_LastActionTimeSec = g_GlobalTimer.TimeElapsedSeconds();
					}
				}
				else {
					if (m_Input.GamepadButtonStates[iButton].m_Action == kbInput_t::KA_JustPressed || m_Input.GamepadButtonStates[iButton].m_Action == kbInput_t::KA_Down) {
						m_Input.GamepadButtonStates[iButton].m_Action = kbInput_t::KA_JustReleased;
					}
					else {
						m_Input.GamepadButtonStates[iButton].m_Action = kbInput_t::KA_None;
					}
				}
			}
		}
	}

	for (int i = 0; i < 256; i++) {
		if (m_Input.KeyState[i].m_Action == kbInput_t::KA_JustReleased) {
			m_Input.KeyState[i].m_Action = kbInput_t::KA_None;
		}
	}

	for (int i = 0; i < 4; i++) {
		if (m_Input.ArrowState[i].m_Action == kbInput_t::KA_JustReleased) {
			m_Input.ArrowState[i].m_Action = kbInput_t::KA_None;
		}
	}

	for (int i = 0; i < kbInput_t::Num_NonCharKeys; i++) {
		if (m_Input.NonCharKeyState[i].m_Action == kbInput_t::KA_JustReleased) {
			m_Input.NonCharKeyState[i].m_Action = kbInput_t::KA_None;
		}
	}

	KeyComboBitField_t combo;

	if (GetAsyncKeyState(VK_LCONTROL)) {
		combo.m_Ctrl = true;
	}

	if (GetAsyncKeyState(VK_LSHIFT)) {
		combo.m_Shift = true;
	}

	// Check that a key was pressed this frame, so that we don't spam commands
	bool bAtLeastOneNewKeyPressed = false;

	for (int i = 0; i < 256; i++) {
		if (GetAsyncKeyState(i)) {
			if (m_Input.KeyState[i].m_Action == kbInput_t::KA_None) {
				m_Input.KeyState[i].m_Action = kbInput_t::KA_JustPressed;
				m_Input.KeyState[i].m_LastActionTimeSec = g_GlobalTimer.TimeElapsedSeconds();
			}
			else {
				m_Input.KeyState[i].m_Action = kbInput_t::KA_Down;
			}

			if (i >= '!' && i <= '}') {
				if (m_Input.KeyState[i].m_Action == kbInput_t::KA_JustPressed) {
					bAtLeastOneNewKeyPressed = true;
				}

				if (i < 64) {
					combo.m_Bits0 |= (__int64)1 << i;
				}
				else {
					combo.m_Bits1 |= (__int64)1 << (i - 64);
				}
			}
		}
		else {
			if (m_Input.KeyState[i].m_Action == kbInput_t::KA_Down) {
				m_Input.KeyState[i].m_Action = kbInput_t::KA_JustReleased;
				m_Input.KeyState[i].m_LastActionTimeSec = g_GlobalTimer.TimeElapsedSeconds();
			}
			else {
				m_Input.KeyState[i].m_Action = kbInput_t::KA_None;
			}
		}
	}

	for (int i = 0; i < 4; i++) {
		static int arrowList[] = { VK_UP, VK_LEFT, VK_RIGHT, VK_DOWN };

		if (GetAsyncKeyState(arrowList[i])) {
			if (m_Input.ArrowState[i].m_Action == kbInput_t::KA_None) {
				m_Input.ArrowState[i].m_Action = kbInput_t::KA_JustPressed;
				m_Input.ArrowState[i].m_LastActionTimeSec = g_GlobalTimer.TimeElapsedSeconds();
			}
			else {
				m_Input.ArrowState[i].m_Action = kbInput_t::KA_Down;
			}
		}
		else {
			if (m_Input.ArrowState[i].m_Action == kbInput_t::KA_Down) {
				m_Input.ArrowState[i].m_Action = kbInput_t::KA_JustReleased;
				m_Input.ArrowState[i].m_LastActionTimeSec = g_GlobalTimer.TimeElapsedSeconds();
			}
			else {
				m_Input.ArrowState[i].m_Action = kbInput_t::KA_None;
			}
		}
	}

	for (int i = 0; i < kbInput_t::Num_NonCharKeys; i++) {

		// Note: This list's order must match kbNonCharKey_t
		static int NonCharKeyList[] = { VK_ESCAPE, VK_LCONTROL, VK_RCONTROL, VK_RETURN };

		if (GetAsyncKeyState(NonCharKeyList[i])) {
			if (m_Input.NonCharKeyState[i].m_Action == kbInput_t::KA_None) {
				m_Input.NonCharKeyState[i].m_Action = kbInput_t::KA_JustPressed;
				m_Input.NonCharKeyState[i].m_LastActionTimeSec = g_GlobalTimer.TimeElapsedSeconds();
			}
			else {
				m_Input.NonCharKeyState[i].m_Action = kbInput_t::KA_Down;
			}
		}
		else {
			if (m_Input.NonCharKeyState[i].m_Action == kbInput_t::KA_Down) {
				m_Input.NonCharKeyState[i].m_Action = kbInput_t::KA_JustReleased;
				m_Input.NonCharKeyState[i].m_LastActionTimeSec = g_GlobalTimer.TimeElapsedSeconds();
			}
			else {
				m_Input.NonCharKeyState[i].m_Action = kbInput_t::KA_None;
			}
		}
	}

	// Notify objects that are watching this key combination
	if (bAtLeastOneNewKeyPressed) {
		auto comboMapResults = m_KeyComboToCallbackMap.find(combo);
		if (comboMapResults != m_KeyComboToCallbackMap.end()) {
			InputCallbackInfo_t& cbInfo = comboMapResults->second;
			cbInfo.m_CallbackObject->InputKeyPressedCB(cbInfo.m_CallbackParam);
		}
	}

	POINT CursorPos;
	GetCursorPos(&CursorPos);
	RECT rc;
	GetClientRect(m_Hwnd, &rc);

	m_Input.AbsCursorX = CursorPos.x;
	m_Input.AbsCursorY = CursorPos.y;

	if (bWindowIsSelected == false) {
		if (GetAsyncKeyState(VK_LBUTTON)) {
			POINT RelativePos = CursorPos;
			ScreenToClient(m_Hwnd, &RelativePos);
			if (RelativePos.x < 0 || RelativePos.x >(rc.right - rc.left) || RelativePos.y < 0 || RelativePos.y >(rc.bottom - rc.top)) {
				return;
			}
			bWindowIsSelected = true;
			ShowCursor(false);

			RECT clientRect;
			GetClientRect(m_Hwnd, &clientRect);
			clientRect.bottom -= 64;
			clientRect.top += 64;
			clientRect.left += 64;
			clientRect.right -= 64;

			ClipCursor(&clientRect);
		}
		else {
			return;
		}
	}

	HMONITOR hCurrMonitor = MonitorFromPoint(POINT(CursorPos), MONITOR_DEFAULTTONULL);
	if (hCurrMonitor != nullptr) {

		if (m_MouseBehavior != MB_LockToWindow) {
			MONITORINFO monInfo = {};
			monInfo.cbSize = sizeof(MONITORINFO);
			if (GetMonitorInfo(hCurrMonitor, &monInfo)) {
				const LONG CenterX = monInfo.rcMonitor.left + (monInfo.rcMonitor.right - monInfo.rcMonitor.left) / 2;
				const LONG CenterY = monInfo.rcMonitor.top + (monInfo.rcMonitor.bottom - monInfo.rcMonitor.top) / 2;

				m_Input.MouseDeltaX = CursorPos.x - CenterX;
				m_Input.MouseDeltaY = CursorPos.y - CenterY;
				SetCursorPos(CenterX, CenterY);
			}
		}
		else {
			CursorPos.x = kbClamp(CursorPos.x, (LONG)32, (LONG)rc.right - 32);
			CursorPos.y = kbClamp(CursorPos.y, (LONG)32, (LONG)rc.bottom - 32);
			SetCursorPos(CursorPos.x, CursorPos.y);
		}
	}

	if (GetAsyncKeyState(VK_LBUTTON)) {
		if (m_Input.LeftMouseButtonDown == false) {
			m_Input.LeftMouseButtonPressed = true;
		}
		else {
			m_Input.LeftMouseButtonPressed = false;
		}

		m_Input.LeftMouseButtonDown = true;
	}
	else {
		m_Input.LeftMouseButtonDown = false;
		m_Input.LeftMouseButtonPressed = false;
	}

	if (GetAsyncKeyState(VK_RBUTTON)) {
		if (m_Input.RightMouseButtonDown == false) {
			m_Input.RightMouseButtonPressed = true;
		}
		else {
			m_Input.RightMouseButtonPressed = false;
		}

		m_Input.RightMouseButtonDown = true;
	}
	else {
		m_Input.RightMouseButtonDown = false;
		m_Input.RightMouseButtonPressed = false;
	}

	if (GetAsyncKeyState(VK_LEFT)) {
		m_Input.m_LeftStick.x = -1.0f;
	}

	if (GetAsyncKeyState(VK_RIGHT)) {
		m_Input.m_LeftStick.x = 1.0f;
	}

	if (GetAsyncKeyState(VK_UP)) {
		m_Input.m_LeftStick.y = 1.0f;
	}

	if (GetAsyncKeyState(VK_DOWN)) {
		m_Input.m_LeftStick.y = -1.0f;
	}

	for (size_t i = 0; i < m_InputListeners.size(); i++) {
		m_InputListeners[i]->InputCB(m_Input);
	}
}

///	kbInputManager::MapKeysToCallback
void kbInputManager::MapKeysToCallback(const std::string& stringCombo, kbInputCallback* pCB, const int callbackParam, const std::string& helpDescription) {
	std::stringstream ss;
	ss.str(stringCombo);
	std::string curKey;

	KeyComboBitField_t newComboKey;

	while (std::getline(ss, curKey, ' ')) {
		if (curKey == "ctrl") {
			newComboKey.m_Ctrl = true;
			continue;
		}

		if (curKey == "shift") {
			newComboKey.m_Shift = true;
			continue;
		}

		if (curKey.size() != 1 || curKey[0] < '!' || curKey[0] > '}') {
			if (curKey.size() == 0) {
				blk::error("kbInputManager::MapKeysToCallback() - %s mapped invalid key.", pCB->GetInputCBName());
			}
			else {
				blk::error("kbInputManager::MapKeysToCallback() - %s mapped invalid key %d.", pCB->GetInputCBName(), curKey[0]);
			}
			continue;
		}

		const __int64 iCurKey = std::toupper(curKey[0]);
		if (iCurKey < 64) {
			newComboKey.m_Bits0 |= (__int64)1 << iCurKey;
		}
		else {
			newComboKey.m_Bits1 |= (__int64)1 << (iCurKey - 64);
		}
	}

	// Check if the key combination already exiwsts
	if (m_KeyComboToCallbackMap.find(newComboKey) != m_KeyComboToCallbackMap.end()) {
		blk::error("kbInputManager::MapKeysToCallback() - %s mapped tried to map over an existing key combo %s with description %s.", pCB->GetInputCBName(), stringCombo, m_KeyComboToCallbackMap.find(newComboKey)->second.m_HelpDescription);
		return;
	}

	InputCallbackInfo_t CallbackInfo;
	CallbackInfo.m_CallbackObject = pCB;
	CallbackInfo.m_CallbackParam = callbackParam;
	CallbackInfo.m_HelpDescription = helpDescription;
	CallbackInfo.m_KeyComboDisplayString = stringCombo;

	for (int i = 0; i < CallbackInfo.m_KeyComboDisplayString.size(); i++) {
		if (CallbackInfo.m_KeyComboDisplayString[i] == ' ') {
			CallbackInfo.m_KeyComboDisplayString[i] = '+';
		}
	}

	m_KeyComboToCallbackMap[newComboKey] = CallbackInfo;
}

///	kbInputManager::UnmapCallback
void kbInputManager::UnmapCallback(kbInputCallback* pCB) {
	/*	if ( stringCombo.length() == 0 || pCB == nullptr ) {
			blk::error( "kbInputManager::RegisterKeyToCallback() - Called with bad data" );
			return;
		}

	*/
}

///	kbInputManager::RegisterInputListener
void kbInputManager::RegisterInputListener(IInputListener* const pListener) {
	blk::error_check(pListener != nullptr, "kbInputManager::RegisterInputListener() - null pListener");

	if (blk::std_find(m_InputListeners, pListener) != m_InputListeners.end()) {
		return;
	}

	m_InputListeners.push_back(pListener);
}

///	kbInputManager::UnregisterInputListener
void kbInputManager::UnregisterInputListener(IInputListener* const pListener) {

	blk::error_check(pListener != nullptr, "kbInputManager::UnregisterInputListener() - null pListener");
	//blk::error_check( VectorFind( m_InputListeners, pListener ) != m_InputListeners.end(), "kbInputManager::RegisterInputListener() - pListener already registered" ); 

	blk::std_remove_swap(m_InputListeners, pListener);
}
