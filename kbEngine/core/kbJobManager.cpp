/// kbJobManager.cpp
///
/// 2016-2025 blk 1.0

#include <Windows.h>
#include "kbCore.h"
#include "kbJobManager.h"

kbJobManager* g_pJobManager = nullptr;

/// SetThreadName
void SetThreadName(const char threadName[]) {
	struct THREADNAME_INFO {
		DWORD dwType = 0x1000;
		LPCSTR szName = nullptr;
		DWORD dwThreadID = GetCurrentThreadId();
		DWORD dwFlags = 0;
	} thread_info;
	thread_info.szName = threadName;
	thread_info.dwThreadID = GetCurrentThreadId();
	thread_info.dwFlags = 0;

	__try {
		RaiseException(0x406D1388, 0, sizeof(thread_info) / sizeof(ULONG_PTR), (ULONG_PTR*)&thread_info);
	} __except (EXCEPTION_CONTINUE_EXECUTION) {
	}
}

/// ThreadMain
DWORD WINAPI ThreadMain(LPVOID lpParam) {
	const DWORD threadId = GetThreadId(GetCurrentThread());
	blk::log("Thread created with id %d", threadId);

	const std::string threadName = "kbEngine Thread" + std::to_string(threadId);
	SetThreadName(threadName.c_str());

	kbJobManager* const jobManager = (kbJobManager*)lpParam;
	while (jobManager->IsShuttingDown() == false) {
		kbJob* newJob = jobManager->GrabJob();

		if (newJob != nullptr) {
			newJob->Run();
			newJob->MarkJobAsComplete();
		}
	}

	return 0;
};

/// kbJobManager::kbJobManager
kbJobManager::kbJobManager() :
	m_JobQueueHead(nullptr),
	m_JobQueueTail(nullptr),
	m_bShutdownRequested(false) {
	g_pJobManager = this;

	m_Mutex = CreateMutex(nullptr, FALSE, nullptr);

	MemoryBarrier();

	for (int i = 0; i < MAX_NUM_THREADS; i++) {
		m_Threads[i] = CreateThread(nullptr, 0, ThreadMain, this, 0, nullptr);
	}
}

/// kbJobManager::~kbJobManager
kbJobManager::~kbJobManager() {
	m_bShutdownRequested = true;

	WaitForMultipleObjects(MAX_NUM_THREADS, m_Threads, TRUE, INFINITE);

	for (int i = 0; i < MAX_NUM_THREADS; i++) {
		CloseHandle(m_Threads[i]);
	}

	CloseHandle(m_Mutex);
}

/// kbJobManager::RegisterJob
void kbJobManager::RegisterJob(kbJob* job) {
	job->m_bIsFinished = false;

	WaitForSingleObject(m_Mutex, INFINITE);

	if (m_JobQueueHead == nullptr) {
		m_JobQueueHead = job;
		m_JobQueueTail = job;
		job->m_Next = nullptr;
	} else {
		m_JobQueueTail->m_Next = job;
		m_JobQueueTail = job;
		m_JobQueueTail->m_Next = nullptr;
	}

	ReleaseMutex(m_Mutex);
}

/// kbJobManager::GrabJob
kbJob* kbJobManager::GrabJob() {
	WaitForSingleObject(m_Mutex, INFINITE);

	kbJob* returnedJob = m_JobQueueHead;

	if (returnedJob != nullptr) {

		m_JobQueueHead = returnedJob->m_Next;

		if (m_JobQueueHead == nullptr) {
			m_JobQueueTail = nullptr;
		}
	}

	ReleaseMutex(m_Mutex);

	return returnedJob;
}
