//===================================================================================================
// kbJobManager.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBJOBMANAGER_H_
#define _KBJOBMANAGER_H_

#define MAX_NUM_THREADS 8

/*
 *	kbJob
 */
class kbJob {
	friend class kbJobManager;

public:
					kbJob() : m_Next( NULL ), m_IsFinished( 1 ) { }

	virtual void	Run() = 0;

	bool			IsJobFinished() const { return m_IsFinished != 0; }
	void			WaitForJob() const { while( IsJobFinished() == false ) { } }
	void			MarkJobAsComplete() { m_IsFinished = true; }

private:
	kbJob *			m_Next;
	volatile int	m_IsFinished;
};

/*
 *	kbJobManager
 */
class kbJobManager {
public:
					kbJobManager();
					~kbJobManager();

	void			RegisterJob( kbJob * job );

	kbJob *			GrabJob();

	bool			IsShuttingDown() { return m_ShutdownRequested; }

private:

	kbJob *			m_JobQueueHead;
	kbJob *			m_JobQueueTail;

	HANDLE			m_Threads[MAX_NUM_THREADS];
	HANDLE			m_Mutex;

	bool			m_ShutdownRequested;
};

extern kbJobManager * g_pJobManager;

#endif