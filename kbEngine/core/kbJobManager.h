//===================================================================================================
// kbJobManager.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBJOBMANAGER_H_
#define _KBJOBMANAGER_H_

#define MAX_NUM_THREADS 8

/**
 *	kbJob
 */
class kbJob {
	friend class kbJobManager;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

															kbJob() : m_Next( nullptr ), m_bIsFinished( 1 ) { }

	virtual void											Run() = 0;

	bool													IsJobFinished() const { return m_bIsFinished != 0; }
	void													WaitForJob() const { while( IsJobFinished() == false ) { } }
	void													MarkJobAsComplete() { m_bIsFinished = true; }

private:

	kbJob *													m_Next;
	volatile int											m_bIsFinished;
};

/**
 *	kbJobManager
 */
class kbJobManager {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
															kbJobManager();
															~kbJobManager();

	void													RegisterJob( kbJob * job );

	kbJob *													GrabJob();

	bool													IsShuttingDown() { return m_bShutdownRequested; }

private:

	kbJob *													m_JobQueueHead;
	kbJob *													m_JobQueueTail;

	HANDLE													m_Threads[MAX_NUM_THREADS];
	HANDLE													m_Mutex;

	bool													m_bShutdownRequested;
};

extern kbJobManager *										g_pJobManager;

void SetThreadName( const char threadName[] );

#endif