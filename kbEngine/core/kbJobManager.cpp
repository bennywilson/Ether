//===================================================================================================
// kbJobManager.cpp
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#include <Windows.h>
#include "kbCore.h"
#include "kbJobManager.h"

kbJobManager * g_pJobManager = nullptr;

DWORD WINAPI ThreadMain( LPVOID lpParam ) {
	DWORD threadId = GetThreadId(GetCurrentThread());
	kbLog( "Thread created with id %d", threadId );

	kbJobManager * jobManager = (kbJobManager*)lpParam;

	while( jobManager->IsShuttingDown() == false ) {
		kbJob * newJob = jobManager->GrabJob();

		if ( newJob != nullptr ) {
			newJob->Run();
			newJob->MarkJobAsComplete();
		}
	}

	return 0;
};

/**
 *	kbJobManager
 */
kbJobManager::kbJobManager() :
	m_JobQueueHead( nullptr ),
	m_JobQueueTail( nullptr ),
	m_ShutdownRequested( false ) {
	g_pJobManager = this;

	m_Mutex = CreateMutex( nullptr, FALSE, nullptr );

	MemoryBarrier();

	for ( int i = 0; i < MAX_NUM_THREADS; i++ ) {
		m_Threads[i] = CreateThread( nullptr, 0, ThreadMain, this, 0, nullptr );
	}
}

/**
 *	~kbJobManager
 */
kbJobManager::~kbJobManager() {
	m_ShutdownRequested = true;

	WaitForMultipleObjects( MAX_NUM_THREADS, m_Threads, TRUE, INFINITE );
	
	for ( int i = 0; i < MAX_NUM_THREADS; i++ ) {
		CloseHandle( m_Threads[i] );
	}

	CloseHandle( m_Mutex );
}

/**
 *	kbJobManager::RegisterJob
 */
void kbJobManager::RegisterJob( kbJob * job ) {
	job->m_IsFinished = false;

	WaitForSingleObject( m_Mutex, INFINITE );

	if ( m_JobQueueHead == nullptr ) {
		m_JobQueueHead = job;
		m_JobQueueTail = job;
		job->m_Next = nullptr;
	} else {
		m_JobQueueTail->m_Next = job;
		m_JobQueueTail = job;
		m_JobQueueTail->m_Next = nullptr;
	}

	ReleaseMutex( m_Mutex );
}

/**
 *	kbJobManager::GrabJob
 */
kbJob *	kbJobManager::GrabJob() {
	WaitForSingleObject( m_Mutex, INFINITE );

	kbJob * returnedJob = m_JobQueueHead;

	if ( returnedJob != nullptr ) {

		m_JobQueueHead = returnedJob->m_Next;

		if ( m_JobQueueHead == nullptr ) {
			m_JobQueueTail = nullptr;
		}
	}

	ReleaseMutex( m_Mutex );

	return returnedJob;
}
