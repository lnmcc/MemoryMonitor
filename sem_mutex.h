/* ************************************************************************
	> File Name: sem_mutex.h
	> Author: lnmcc
	> Mail: lnmcc@hotmail.com 
	> Blog: lnmcc.net 
	> Created Time: 2014年04月11日 星期五 15时08分46秒
 *********************************************************************** */

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
