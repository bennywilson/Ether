//===================================================================================================
// kbCore.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _CORE_H_
#define _CORE_H_

#pragma warning( disable : 4482 )

#include <fstream>
#include <vector>
#include <map>
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

#include <windows.h>


typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;


/**
 *	kbGUID - Each kbGameEntity is given a GUID at construction that is saved out and referenced across multiple files
 */
struct kbGUID {

	kbGUID() {
		m_iGuid[0] = m_iGuid[1] = m_iGuid[2] = m_iGuid[3] = 0;
	}

	union {
		GUID	m_Guid;
		uint	m_iGuid[4];
	};

	bool operator ==( const kbGUID & rhs ) const { return m_iGuid[0] == rhs.m_iGuid[0] && m_iGuid[1] == rhs.m_iGuid[1] && m_iGuid[2] == rhs.m_iGuid[2] && m_iGuid[3] == rhs.m_iGuid[3]; }
	bool IsValid() const { return m_iGuid[0] != 0 && m_iGuid[1] != 0 && m_iGuid[2] != 0 && m_iGuid[3] != 0; }
};

#include "kbResourceManager.h"

extern FILE * g_LogFile;
extern bool	g_UseEditor;

enum kbOutputMessageType_t {
	Message_Normal,
	Message_Warning,
	Message_Assert,
	Message_Error,
};

// Note: call before doing anything else
void InitializeKBEngine(char *const logName = nullptr );
void ShutdownKBEngine();

typedef void ( kbOutputCB )( kbOutputMessageType_t, const char * );
extern kbOutputCB * outputCB;
void kbWarning( const char *const msg, ... );
void kbWarningCheck( const bool bExpression, const char *const msg, ... );
void kbAssert_Impl( const char *const msg, ... );
void kbLog( const char *const msg, ... );
void kbError( const char *const msg, ... );
void kbErrorCheck( const bool bExpression, const char *const msg, ... );

#define kbAssert( expression, msg, ... ) \
	if ( expression == true ) return; \
	kbAssert_Impl( msg, __VA_ARGS__ ); \
	DebugBreak();

// helper functions
const float kbPI = 3.14159265359f;
const float kbEpsilon = 0.00001f;
inline float kbToRadians( const float degrees ) { return degrees * kbPI / 180.0f; }
inline float kbToDegrees( const float radians ) { return radians * 180.0f / kbPI; }
float kbfrand();

template<typename T> T kbClamp( const T & value, const T & min, const T & max ) { return value < min ? min : ( value > max ? max : value ); }

template<typename T, typename B> void VectorRemoveFast( T & list, B entry ) { list.erase( std::remove( list.begin(), list.end(), entry ), list.end() );  }
template<typename T> void VectorRemoveFastIndex( T & list, const int i ) { list.erase( list.begin() + i ); }
template<typename T, typename B> bool VectorFind( T & list, B entry ) { return std::find( list.begin(), list.end(), entry ) != list.end(); }
template<typename T> inline T kbLerp( T a, T b, float t ) { return ( ( b - a ) * t ) + a; }

void StringFromWString( std::string & outString, const std::wstring & srcString );
void WStringFromString( std::wstring & outString, const std::string & srcString );

std::string GetFileExtension( const std::string & FileName );
std::wstring GetFileExtension( const std::wstring & FileName );

#define SAFE_RELEASE( object ) { if ( object != nullptr ) { object->Release(); object = nullptr; } }

/**
 *  kbTimer
 */
class kbTimer {

//---------------------------------------------------------------------------------------------------
public:

	kbTimer() {
      LARGE_INTEGER largeInt;
      QueryPerformanceFrequency( &largeInt );
      m_ClockFrequency = largeInt.QuadPart / 1000.0;

      Reset();
   }

   void Reset() {
      LARGE_INTEGER largeInt;
      QueryPerformanceCounter( &largeInt );
      m_Counter = largeInt.QuadPart;
   }

   float TimeElapsedMS() {
      LARGE_INTEGER largeInt;
      QueryPerformanceCounter( &largeInt );

      return ( float ) ( ( largeInt.QuadPart - m_Counter ) / m_ClockFrequency );
   }

	float TimeElapsedSeconds() {
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
	RENDER_GPUTIMER_STALL,
	MAX_NUM_SCOPED_TIMERS,
};

struct kbScopedTimerData_t {

	kbScopedTimerData_t( const ScopedTimerList_t timerIdx, char *const stringName );

	kbString									m_ReadableName;

	float										GetFrameTime() const;

	const static int							NUM_FRAME_TIMES = 10;
	float										m_FrameTimes[NUM_FRAME_TIMES];
	int											m_FrameTimeIdx;
};

/**
 *	kbScopedTimer
 */
class kbScopedTimer {

//---------------------------------------------------------------------------------------------------
public:

												kbScopedTimer( ScopedTimerList_t index );
												~kbScopedTimer();

private:
	kbTimer										m_Timer;
	ScopedTimerList_t							m_TimerIndex;
};

#define START_SCOPED_TIMER(index) kbScopedTimer a##index(index);

void UpdateScopedTimers();
const kbScopedTimerData_t & GetScopedTimerData( const ScopedTimerList_t index );

/**
 *  kbInput_t
 */
struct kbInput_t {
	kbInput_t() {
		memset( this, 0, sizeof( kbInput_t ) );
	}

	enum kbKeyAction_t {
		KA_None,
		KA_JustPressed,
		KA_Down,
		KA_JustReleased,
	};

	bool IsKeyPressedOrDown( const char key ) const { return KeyState[key].m_Action == KA_JustPressed || KeyState[key].m_Action == KA_Down; }

	struct kbKeyState_t {
		kbKeyAction_t	m_Action;
		float			m_LastActionTimeSec;
	};

	kbKeyState_t	KeyState[256];
	float			LeftStickX;
	float			LeftStickY;
	float			RightStickX;
	float			RightStickY;
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

#endif
