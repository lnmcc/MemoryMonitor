#ifndef _SEMMUTEX_H
#define _SEMMUTEX_H

#include <semaphore.h>

using namespace std;

class SemMutex {
public:
	SemMutex();
	virtual ~SemMutex();

	void lock();
	void unlock();

private:
    pid_t gettid();

private:
	sem_t m_Semaphore;
	unsigned int m_lockCount;	 
    pid_t m_pid;
};

#endif
