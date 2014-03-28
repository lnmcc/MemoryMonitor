#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "sem_mutex.h"

SemMutex::SemMutex() {
	sem_init(&m_Semaphore, 0, 1);
	m_lockCount = 0 ;	 
	m_pid = 0 ;
}

SemMutex::~SemMutex() { 
	sem_destroy(&m_Semaphore);
}

pid_t SemMutex::gettid() {
    return syscall(SYS_gettid);
}

void SemMutex::lock() {
    if(m_pid != gettid()) {
		sem_wait(&m_Semaphore);
        m_pid = gettid();
	}	
	m_lockCount++;
}

void SemMutex::unlock() {
    if(m_pid == gettid()) {

        m_lockCount--;
					
		if(m_lockCount == 0) {
			sem_post( &m_Semaphore );
            m_pid = 0;
	    }
    }	
}
