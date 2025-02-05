/// kbCore.h
///
/// 2016-2025 blk 1.0

#pragma once
#pragma warning(disable : 4482)

#include <windows.h>
//#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <string>
#include "kbString.h"
/**
 *	kbTypeInfoType_t
 */
enum kbTypeInfoType_t {
	KBTYPEINFO_NONE,
	KBTYPEINFO_BOOL,
	KBTYPEINFO_INT,
	KBTYPEINFO_FLOAT,
	KBTYPEINFO_STRING,
	KBTYPEINFO_VECTOR,
	KBTYPEINFO_VECTOR4,
	KBTYPEINFO_PTR,
	KBTYPEINFO_TEXTURE,
	KBTYPEINFO_STATICMODEL,
	KBTYPEINFO_SOUNDWAVE,
	KBTYPEINFO_SHADER,
	KBTYPEINFO_ENUM,
	KBTYPEINFO_ANIMATION,
	KBTYPEINFO_KBSTRING,
	KBTYPEINFO_STRUCT,
	KBTYPEINFO_GAMEENTITY,
};

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

///	kbGUID - Each kbGameEntity is given a GUID at construction that is saved out and referenced across multiple files
struct kbGUID {
	kbGUID() {
		m_iGuid[0] = m_iGuid[1] = m_iGuid[2] = m_iGuid[3] = 0;
	}

	union {
		GUID m_Guid;
		uint m_iGuid[4];
	};

	bool operator ==(const kbGUID& rhs) const { return m_iGuid[0] == rhs.m_iGuid[0] && m_iGuid[1] == rhs.m_iGuid[1] && m_iGuid[2] == rhs.m_iGuid[2] && m_iGuid[3] == rhs.m_iGuid[3]; }
	bool IsValid() const { return m_iGuid[0] != 0 && m_iGuid[1] != 0 && m_iGuid[2] != 0 && m_iGuid[3] != 0; }
};

extern FILE* g_LogFile;
extern bool	g_UseEditor;

enum kbOutputMessageType_t {
	Message_Normal,
	Message_Warning,
	Message_Assert,
	Message_Error,
};

typedef void (kbOutputCB)(kbOutputMessageType_t, const char*);
extern kbOutputCB* outputCB;

namespace blk {
	/// Call `initialize_engine()` before any other blk functions
	void initialize_engine(char* const logName = nullptr);
	void shutdown_engine();

	void log(const char* const msg, ...);
	void error(const char* const msg, ...);
	void warning(const char* const msg, ...);
	void warning_check(const bool expression, const char* const msg, ...);
	void error_check(const bool expression, const char* const msg, ...);
};

#define kbAssert( expression, msg, ... ) \
	if ( expression == true ) return; \
	kbAssert_Impl( msg, __VA_ARGS__ ); \
	DebugBreak();

// helper functions
const float kbPI = 3.14159265359f;
const float kbEpsilon = 0.00001f;
inline float kbToRadians(const float degrees) { return degrees * kbPI / 180.0f; }
inline float kbToDegrees(const float radians) { return radians * 180.0f / kbPI; }

inline bool kbCompareByte4(const byte lhs[4], const byte rhs[4]) { return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2] && lhs[3] == rhs[3]; }

template<typename T> T kbClamp(const T& value, const T& min, const T& max) { return value < min ? min : (value > max ? max : value); }
template<typename T> T kbSaturate(const T& value) { return value < 0 ? 0 : (value > 1 ? 1 : value); }

template<typename T> inline T kbLerp(const T a, const T b, const float t) { return ((b - a) * t) + a; }

void StringFromWString(std::string& outString, const std::wstring& srcString);
void WStringFromString(std::wstring& outString, const std::string& srcString);
void StringToLower(std::string& outString);

std::string GetFileExtension(const std::string& FileName);
std::wstring GetFileExtension(const std::wstring& FileName);

#include "kbVector.h"

float kbfrand();

inline int kbirand(const int min, const int max) {
	return min + rand() % (max - min);
}

inline float kbfrand(const float min, const float max) {
	return min + (kbfrand() * (max - min));
}

inline kbVec2 kbVec2Rand(const kbVec2& min, const kbVec2& max) {
	kbVec2 randVec;
	randVec.x = min.x + (kbfrand() * (max.x - min.x));
	randVec.y = min.y + (kbfrand() * (max.y - min.y));

	return randVec;
}

inline kbVec3 kbVec3Rand(const kbVec3& min, const kbVec3& max) {
	kbVec3 randVec;
	randVec.x = min.x + (kbfrand() * (max.x - min.x));
	randVec.y = min.y + (kbfrand() * (max.y - min.y));
	randVec.z = min.z + (kbfrand() * (max.z - min.z));

	return randVec;
}

inline kbVec4 kbVec4Rand(const kbVec4& min, const kbVec4& max) {
	kbVec4 randVec;
	randVec.x = min.x + (kbfrand() * (max.x - min.x));
	randVec.y = min.y + (kbfrand() * (max.y - min.y));
	randVec.z = min.z + (kbfrand() * (max.z - min.z));
	randVec.w = min.w + (kbfrand() * (max.w - min.w));
	return randVec;

}

#define SAFE_RELEASE( object ) { if ( object != nullptr ) { object->Release(); object = nullptr; } }

/// kbTimer
class kbTimer {
public:
	kbTimer() {
		LARGE_INTEGER largeInt;
		QueryPerformanceFrequency(&largeInt);
		m_ClockFrequency = largeInt.QuadPart / 1000.0;

		Reset();
	}

	void Reset() {
		LARGE_INTEGER largeInt;
		QueryPerformanceCounter(&largeInt);
		m_Counter = largeInt.QuadPart;
	}

	float TimeElapsedMS() const {
		LARGE_INTEGER largeInt;
		QueryPerformanceCounter(&largeInt);

		return (float)((largeInt.QuadPart - m_Counter) / m_ClockFrequency);
	}

	float TimeElapsedSeconds() const {
		return TimeElapsedMS() / 1000.0f;
	}

private:
	double										m_ClockFrequency;
	__int64										m_Counter;
};

extern kbTimer g_GlobalTimer;

enum ScopedTimerList_t {
	GAME_THREAD,
	GAME_ENTITY_UPDATE,
	COMPONENT_UPDATE,
	CLOTH_COMPONENT,
	GAME_THREAD_IDLE,
	RENDER_THREAD,
	RENDER_THREAD_CLEAR_BUFFERS,
	RENDER_G_BUFFER,
	RENDER_LIGHTING,
	RENDER_SHADOW_DEPTH,
	RENDER_LIGHT,
	RENDER_UNLIT,
	RENDER_TRANSLUCENCY,
	RENDER_LIGHTSHAFTS,
	RENDER_POST_PROCESS,
	RENDER_TEXT,
	RENDER_DEBUG,
	RENDER_PRESENT,
	RENDER_ENTITYID,
	RENDER_SYNC,
	RENDER_SYNC_PARTICLES,
	RENDER_GPUTIMER_STALL,
	TEMP_1,
	TEMP_2,
	TEMP_3,
	TEMP_4,
	TEMP_5,
	TEMP_6,
	TEMP_7,
	TEMP_8,
	TEMP_9,
	TEMP_10,
	MAX_NUM_SCOPED_TIMERS,
};

struct kbScopedTimerData_t {
	kbScopedTimerData_t(const ScopedTimerList_t timerIdx, const char* const stringName);

	kbString									m_ReadableName;

	float										GetFrameTime() const;

	const static int							NUM_FRAME_TIMES = 10;
	float										m_FrameTimes[NUM_FRAME_TIMES];
	int											m_FrameTimeIdx;
};

/// kbScopedTimer
class kbScopedTimer {
public:
	kbScopedTimer(ScopedTimerList_t index);
	~kbScopedTimer();

private:
	kbTimer										m_Timer;
	ScopedTimerList_t							m_TimerIndex;
};

#define START_SCOPED_TIMER(index) kbScopedTimer a##index(index);

void UpdateScopedTimers();
const kbScopedTimerData_t& GetScopedTimerData(const ScopedTimerList_t index);

/// kbInput_t
struct kbInput_t {
	kbInput_t() {
		memset(this, 0, sizeof(kbInput_t));
	}

	enum kbKeyAction_t {
		KA_None,
		KA_JustPressed,
		KA_Down,
		KA_JustReleased,
	};

	enum kbArrow_t {
		Up,
		Left,
		Right,
		Down
	};

	enum kbNonCharKey_t {
		Escape = 0,
		LCtrl,
		RCtrl,
		Return,
		Num_NonCharKeys,
	};

	const static char KB_SPACE = 32;

	bool IsKeyPressedOrDown(const char key) const { return KeyState[key].m_Action == KA_JustPressed || KeyState[key].m_Action == KA_Down; }
	bool WasKeyJustPressed(const char key, const bool bConsumeInput = true) const {
		return KeyState[key].m_Action == KA_JustPressed;
	}

	bool IsArrowPressedOrDown(const kbArrow_t arrow) const { return ArrowState[arrow].m_Action == KA_JustPressed || ArrowState[arrow].m_Action == KA_Down; }
	bool WasArrowJustPressed(const kbArrow_t arrow) const { return ArrowState[arrow].m_Action == KA_JustPressed; }

	bool IsNonCharKeyPressedOrDown(const kbNonCharKey_t key) const { return NonCharKeyState[key].m_Action == KA_JustPressed || NonCharKeyState[key].m_Action == KA_Down; }
	bool WasNonCharKeyJustPressed(const kbNonCharKey_t key) const { return NonCharKeyState[key].m_Action == KA_JustPressed; }

	struct kbKeyState_t {
		kbKeyAction_t	m_Action;
		float			m_LastActionTimeSec;
	};

	kbKeyState_t	KeyState[256];
	kbKeyState_t	ArrowState[4];
	kbKeyState_t	GamepadButtonStates[16];
	kbKeyState_t	NonCharKeyState[Num_NonCharKeys];

	kbVec2			m_LeftStick;
	kbVec2			m_PrevLeftStick;

	kbVec2			m_RightStick;
	kbVec2			m_PrevRightStick;

	float			LeftTrigger;
	float			RightTrigger;
	bool			RightTriggerPressed;
	LONG			MouseDeltaX;
	LONG			MouseDeltaY;
	LONG			AbsCursorX;
	LONG			AbsCursorY;
	bool			LeftMouseButtonPressed;
	bool			LeftMouseButtonDown;
	bool			RightMouseButtonPressed;
	bool			RightMouseButtonDown;
};

/// kbTextParser
struct kbTextParser {
	kbTextParser(std::string& inString) :
		m_StringBuffer(inString),
		m_StartBlock(0),
		m_EndBlock(m_StringBuffer.size() - 1) {
	}

	std::string& m_StringBuffer;
	std::string::size_type m_StartBlock;
	std::string::size_type m_EndBlock;

	bool SetBlock(const char* blockName) {
		m_StartBlock = 0;
		m_EndBlock = m_StringBuffer.size() - 1;

		static char delimiters[] = " \n\t";
		auto startBlock = m_StringBuffer.find(blockName);
		if (startBlock == std::string::npos) {
			return false;
		}

		auto endBlock = m_StringBuffer.find("}", startBlock);
		if (endBlock == std::string::npos) {
			return false;
		}

		m_StartBlock = startBlock;
		m_EndBlock = endBlock;

		return true;
	}

	void MakeLowerCase() {

		if (m_StartBlock == std::string::npos) {
			std::transform(m_StringBuffer.begin(), m_StringBuffer.end(), m_StringBuffer.begin(), ::tolower);
		}
		else {
			std::transform(m_StringBuffer.begin() + m_StartBlock, m_StringBuffer.begin() + m_EndBlock, m_StringBuffer.begin() + m_StartBlock, ::tolower);
		}
	}

	bool ContainsKey(const char* const key) const {
		auto keyStartPos = m_StringBuffer.find(key, m_StartBlock /*TODO: Should confine scope of search shaderStateEndBlock - shaderStateStartBlock*/);
		return (keyStartPos != std::string::npos);
	}

	bool GetValueForKey(std::string& outValue, const char* const key) const {
		outValue.clear();
		auto keyStartPos = m_StringBuffer.find(key, m_StartBlock /*TODO: Should confine scope of search shaderStateEndBlock - shaderStateStartBlock*/);
		if (keyStartPos != std::string::npos) {

			static char delimiters[] = " \n\t";
			auto valueStartPos = m_StringBuffer.find_first_of(delimiters, keyStartPos);
			if (valueStartPos == std::string::npos) {
				return false;
			}

			valueStartPos = m_StringBuffer.find_first_not_of(delimiters, valueStartPos + 1);
			if (valueStartPos == std::string::npos) {
				return false;
			}

			auto endValuePos = m_StringBuffer.find_first_of(delimiters, valueStartPos);
			if (endValuePos == std::string::npos) {
				return false;
			}

			outValue = m_StringBuffer.substr(valueStartPos, endValuePos - valueStartPos);
			return true;
		}

		return false;
	}

	void EraseBlock() {
		m_StringBuffer.erase(m_StartBlock, 1 + m_EndBlock - m_StartBlock);
	}

	void ReplaceBlockWithSpaces() {
		for (size_t i = m_StartBlock; i <= m_EndBlock; i++) {
			if (m_StringBuffer[i] != '\n') {
				m_StringBuffer[i] = ' ';
			}
		}
	}

	void RemoveComments() {

		// Remove comments
		for (int i = 0; i < m_StringBuffer.size() - 1; i++) {
			if (m_StringBuffer[i] == '/' && m_StringBuffer[i + 1] == '*') {
				int j = i;
				while (j < m_StringBuffer.size() - 1 && !(m_StringBuffer[j] == '*' && m_StringBuffer[j + 1] == '/')) {

					// Preserve new lines so that any error messages still line up with the source flie
					if (m_StringBuffer[j] != '\n') {
						m_StringBuffer[j] = ' ';
					}
					j++;
				}

				if (j < m_StringBuffer.size() - 1) {
					m_StringBuffer[j] = ' ';
					m_StringBuffer[j + 1] = ' ';
				}
			}

			if (m_StringBuffer[i] == '/' && m_StringBuffer[i + 1] == '/') {
				int j = i;
				while (j < m_StringBuffer.size() - 1 && m_StringBuffer[j] != '\n') {
					m_StringBuffer[j] = ' ';
					j++;
				}
			}
		}
	}
};