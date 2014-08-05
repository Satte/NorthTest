#pragma once
#include <pthread.h>

class RWLock
{
public:
	RWLock() : m_nReaderCount(0), m_hDataLock(PTHREAD_MUTEX_INITIALIZER), m_hMutex(PTHREAD_MUTEX_INITIALIZER) { }
	~RWLock() { pthread_mutex_destroy(&m_hMutex); pthread_mutex_destroy(&m_hDataLock); }

	void acquireReadLock() { pthread_mutex_lock(&m_hMutex); if (++m_nReaderCount == 1) pthread_mutex_lock(&m_hDataLock); pthread_mutex_unlock(&m_hMutex); }
	void releaseReadLock() { pthread_mutex_lock(&m_hMutex); if (--m_nReaderCount == 0) pthread_mutex_unlock(&m_hDataLock); pthread_mutex_unlock(&m_hMutex); }
	void acquireWriteLock() { pthread_mutex_lock(&m_hDataLock); }
	void releaseWriteLock() { pthread_mutex_unlock(&m_hDataLock); }

private:
	pthread_mutex_t m_hMutex;		// Handle to a mutex that allows a single reader at a time access to the reader counter.
	pthread_mutex_t	m_hDataLock;	// Handle to a semaphore that keeps the data locked for either the readers or the writers.
	int		m_nReaderCount;	// The count of the number of readers. Can legally be zero or one while a writer has the data locked.
};

#define CRITICAL_SECTION pthread_mutex_t
#define InitializeCriticalSection(x) pthread_mutex_init(x, nullptr)
#define DeleteCriticalSection(x) pthread_mutex_destroy(x)
#define EnterCriticalSection(x) pthread_mutex_lock(x)
#define LeaveCriticalSection(x) pthread_mutex_unlock(x)

class Critical_Section
{
public:
	Critical_Section(pthread_mutex_t * rCritical) : m_Critical(rCritical) { pthread_mutex_lock(m_Critical); }
	~Critical_Section() { pthread_mutex_unlock(m_Critical); }
private:
	pthread_mutex_t* m_Critical ;
};
