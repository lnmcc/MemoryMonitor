#ifndef _MEMTRACER_H 
#define _MEMTRACER_H 

#include <map>
#include <list>
#include <stack>
#include <pthread.h>
#include <sys/wait.h>
#include "sem_mutex.h"
#include "mem_checker_header.h"
	
#ifdef MEM_TRACE

#define TRACE_NEW new(__FILE__, __LINE__)
#define TRACE_DELETE delete

void* operator new(size_t size, char* fileName, int lineNum);
void* operator new[](size_t size, char* fileName, int lineNum);

void operator delete(void* address);
void operator delete[](void* address);

class MemTracer {
public:
	MemTracer();
	virtual ~MemTracer();
	
	void insert(void* address, Operation *OP);
	bool erase(void* address, Operation *OP);

private:
    void parseError(char* file, int lineNum, int err) {

private:
	map<void*, Operation> m_mapOP;
	list<Operation> m_listErrOP;	
	SemMutex m_mutexMap;
	int m_msgQueue;		
	char m_msgPath[FILENAME_LEN];	
};

#else

class MemTracer {};

#endif	

#endif
