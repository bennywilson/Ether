//===================================================================================================
// kbCore.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <iostream>
#include <cstdarg>
#include <windows.h>
#include "kbCore.h"
#include "kbJobManager.h"

FILE * g_LogFile = nullptr;
bool g_UseEditor = false;
kbOutputCB * outputCB = nullptr;

std::string adjustedBuffer;
HANDLE g_WriteFileMutex = nullptr;

char * finalBuffer = nullptr;
int finalBufferLength = 0;

kbOutputMessageType_t messageType;
kbTimer g_GlobalTimer;

/**
 *	InitializeKBEngine
 */
void InitializeKBEngine( char *const logName ) {

	CoInitializeEx( nullptr, COINIT_MULTITHREADED );

	g_GlobalTimer.Reset();
	g_WriteFileMutex = CreateMutex( nullptr, FALSE, nullptr );

	if ( logName != nullptr ) {
		std::string fullName = "logs/";
		fullName += logName;

		fopen_s( &g_LogFile, fullName.c_str(), "w" );
	} else {
		fopen_s( &g_LogFile, "logs/logfile.txt", "w" );
	}

	if ( g_LogFile == nullptr ) {
		fopen_s( &g_LogFile, "logs/logfile2.txt", "w" );
		kbErrorCheck( g_LogFile != nullptr, "InitializeKBEngine() - Cannot create log file" );
	}

	kbLog( "Initializing kbCore" );

	g_pJobManager = new kbJobManager;
	kbLog( "kbCore Initialized" );
}

/**
 *	ShutdownKBEngine
 */
void ShutdownKBEngine() {

	kbLog( "Shutting down kbCore..." );

	delete g_pJobManager;
	g_pJobManager = nullptr;

	kbLog( "kbCore Shutdown" );

	fclose( g_LogFile );
	g_LogFile = nullptr;

	CloseHandle( g_WriteFileMutex );

	kbString::ShutDown();
}

/**
 *	WriteToFile
 */
void WriteToFile( const char *const msg, va_list arguments ) {
	
	DWORD dwWaitResult = WaitForSingleObject( g_WriteFileMutex, INFINITE );

	adjustedBuffer = msg;

	// floats are promoted to double, so adjust any format specifier
	std::replace( adjustedBuffer.begin(), adjustedBuffer.end(), '%f', '%g' );
	adjustedBuffer += "\n\0";

	const int finalStringLength = _vscprintf( msg, arguments ) + 2;

	if ( finalBuffer == nullptr || finalBufferLength < finalStringLength ) {
		finalBufferLength = finalStringLength;
		finalBuffer = new char[finalBufferLength];
	}

	vsprintf_s( finalBuffer, finalStringLength, adjustedBuffer.c_str(), arguments );

	fwrite( finalBuffer, sizeof( char ), finalStringLength, g_LogFile );

	if ( outputCB != nullptr ) {
		outputCB( messageType, finalBuffer );
	}

	OutputDebugString( finalBuffer );
	std::cout << finalBuffer;
	ReleaseMutex( g_WriteFileMutex );
}

/**
 *	kbError
 */
void kbError( const char *const msg, ... ) {
	messageType = Message_Error;

	va_list args;
	va_start( args, msg );
	WriteToFile( msg, args );
	va_end( args );

	DebugBreak();
	throw finalBuffer;
}

/**
 *	kbErrorCheck
 */
void kbErrorCheck( const bool bExpression, const char *const msg, ... ) {
	if ( bExpression == true ) {
		return;
	}

	va_list args;
	va_start( args, msg );
	WriteToFile( msg, args );
	va_end( args );

	DebugBreak();

	throw finalBuffer;
}

/**
 *	kbWarning
 */
void kbWarning( const char *const msg, ... ) {
	messageType = Message_Warning;

	va_list args;
	va_start( args, msg );
	WriteToFile( msg, args );
	va_end( args );
}


/**
 *	kbWarningCheck
 */
void kbWarningCheck( const bool bExpression, const char *const msg, ... ) {
	if ( bExpression == true ) {
		return;
	}

	messageType = Message_Warning;
	va_list args;
	va_start( args, msg );
	WriteToFile( msg, args );
	va_end( args );
}

/**
 *	kbAssert_Impl
 */
void kbAssert_Impl( const char *const msg, ... ) {
	messageType = Message_Assert;

	va_list args;
	va_start( args, msg );
	WriteToFile( msg, args );
	va_end( args );
}

/**
 *	kbLog
 */
void kbLog( const char *const msg, ... ) {
	messageType = Message_Normal;

	va_list args;
	va_start( args, msg );
	WriteToFile( msg, args );
	va_end( args );
}

/**
 *  kbfrand
 */
float kbfrand() {
	return ( rand() % 10000 ) / 10000.0f;
}

/**
 *	StringFromWString
 */
void StringFromWString( std::string & outString, const std::wstring & srcString ) {
	outString = std::string( srcString.begin(), srcString.end() );
}

/**
 *	WStringFromString
 */
void WStringFromString( std::wstring & outString, const std::string & srcString ) {
	outString = std::wstring( srcString.begin(), srcString.end() );
}

/**
 *	StringToLower
 */
void StringToLower( std::string & outString  ) {
	std::transform( outString.begin(), outString.end(), outString.begin(), ::tolower );
}

/**
 *	GetFileExtension
 */
std::string GetFileExtension( const std::string & FileName ) {
   std::size_t found = FileName.find_last_of( "." );
   if ( found != std::string::npos ) {
      return FileName.substr( found + 1 );
   }

   return "";
}

/**
 *	GetFileExtension
 */
std::wstring GetFileExtension( const std::wstring & FileName ) {
   std::size_t found = FileName.find_last_of( L"." );
   if ( found != std::wstring::npos ) {
      return FileName.substr( found + 1 );
   }

   return L"";
}


std::map<ScopedTimerList_t, struct kbScopedTimerData_t*> g_ScopedTimerMap;

/**
 *	kbScopedTimerData_t::kbScopedTimerData_t
 */
kbScopedTimerData_t::kbScopedTimerData_t( const ScopedTimerList_t timerIdx, char *const stringName ) {
	m_ReadableName = kbString( stringName );
	memset( &m_FrameTimes, 0, sizeof( m_FrameTimes ) );
	m_FrameTimeIdx = 0;

	g_ScopedTimerMap[timerIdx] = this;
}

/**
 *	kbScopedTimerData_t::GetFrameTime
 */
float kbScopedTimerData_t::GetFrameTime() const {
	float totalMS = 0.0f;
	for ( int i = 0; i < NUM_FRAME_TIMES; i++ ) {
		totalMS += m_FrameTimes[i];
	}

	return totalMS / NUM_FRAME_TIMES;
}

#define DECLARE_SCOPED_TIMER(Index, String) \
	kbScopedTimerData_t Index##Var(Index, String); \

DECLARE_SCOPED_TIMER(GAME_THREAD, "Game Thread")
DECLARE_SCOPED_TIMER(GAME_ENTITY_UPDATE, "   Entity Update")
DECLARE_SCOPED_TIMER(COMPONENT_UPDATE, "      Component Update")
DECLARE_SCOPED_TIMER(CLOTH_COMPONENT, "         Cloth Component")
DECLARE_SCOPED_TIMER(GAME_THREAD_IDLE, "   Game Thread Idle")
DECLARE_SCOPED_TIMER(RENDER_THREAD, "Render Thread")
DECLARE_SCOPED_TIMER(RENDER_THREAD_CLEAR_BUFFERS, "   Clear Buffers")
DECLARE_SCOPED_TIMER(RENDER_G_BUFFER, "   Render G-Buffer")
DECLARE_SCOPED_TIMER(RENDER_LIGHTING, "   Render Lighting")
DECLARE_SCOPED_TIMER(RENDER_SHADOW_DEPTH, "      Render Shadow Depth")
DECLARE_SCOPED_TIMER(RENDER_LIGHT, "      Render Lighting")
DECLARE_SCOPED_TIMER(RENDER_UNLIT, "      Render Unlit")
DECLARE_SCOPED_TIMER(RENDER_TRANSLUCENCY, "   Render Translucency")
DECLARE_SCOPED_TIMER(RENDER_LIGHTSHAFTS, "   Render Light Shafts")
DECLARE_SCOPED_TIMER(RENDER_POST_PROCESS, "   Render Post-Process")
DECLARE_SCOPED_TIMER(RENDER_TEXT, "   Render Text")
DECLARE_SCOPED_TIMER(RENDER_PRESENT, "   Present")
DECLARE_SCOPED_TIMER(RENDER_SYNC, "   Render Sync")
DECLARE_SCOPED_TIMER(RENDER_SYNC_PARTICLES, "   Render Sync Particles")
DECLARE_SCOPED_TIMER(RENDER_GPUTIMER_STALL, "GPU Timer Stall")
DECLARE_SCOPED_TIMER(RENDER_DEBUG,  "Debug Rendering")
DECLARE_SCOPED_TIMER(RENDER_ENTITYID,  "EntityId Rendering")

DECLARE_SCOPED_TIMER(TEMP_1, "Temp 1")
DECLARE_SCOPED_TIMER(TEMP_2, "Temp 2")
DECLARE_SCOPED_TIMER(TEMP_3, "Temp 3")
DECLARE_SCOPED_TIMER(TEMP_4, "Temp 4")
DECLARE_SCOPED_TIMER(TEMP_5, "Temp 5")
DECLARE_SCOPED_TIMER(TEMP_6, "Temp 6")
DECLARE_SCOPED_TIMER(TEMP_7, "Temp 7")
DECLARE_SCOPED_TIMER(TEMP_8, "Temp 8")
DECLARE_SCOPED_TIMER(TEMP_9, "Temp 9")
DECLARE_SCOPED_TIMER(TEMP_10, "Temp 10")


	
	
	
	
	
	
	
	
	
	
/**
 *	kbScopedTimer::kbScopedTimer
 */
kbScopedTimer::kbScopedTimer( ScopedTimerList_t index ) :
	m_TimerIndex( index ) {
}

/**
 *	~kbScopedTimer::kbScopedTimer
 */
kbScopedTimer::~kbScopedTimer() {

	kbScopedTimerData_t *const timerData = g_ScopedTimerMap[m_TimerIndex];
	timerData->m_FrameTimes[timerData->m_FrameTimeIdx] += m_Timer.TimeElapsedMS();
}

/**
 *	UpdateScopedTimers
 */
void UpdateScopedTimers() {

	for ( int i = 0; i < MAX_NUM_SCOPED_TIMERS; i++ ) {
		kbScopedTimerData_t *const timerData = g_ScopedTimerMap[(ScopedTimerList_t)i];

		if ( timerData == nullptr ) {
			kbError( "Scoped timer at index %d is uninitialized", i );
		}

		timerData->m_FrameTimeIdx++;
		if ( timerData->m_FrameTimeIdx >= kbScopedTimerData_t::NUM_FRAME_TIMES ) {
			timerData->m_FrameTimeIdx = 0;
		}
		timerData->m_FrameTimes[timerData->m_FrameTimeIdx] = 0.0f;
	}
}

/**
 *	GetScopedTimerData
 */
const kbScopedTimerData_t & GetScopedTimerData( const ScopedTimerList_t index ) {
	return *g_ScopedTimerMap[index];
}
