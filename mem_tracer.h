/* ************************************************************************
	> File Name: mem_tracer.h
	> Author: lnmcc
	> Mail: lnmcc@hotmail.com 
	> Blog: lnmcc.net 
	> Created Time: 2014年04月11日 星期五 15时08分46秒
 *********************************************************************** */

#ifndef _MEMTRACER_H 
#define _MEMTRACER_H 

#include <map>
#include <list>
#include <stack>
#include <pthread.h>
#include <sys/wait.h>
#include "sem_mutex.h"
#include "common.h"


#ifdef MEM_TRACE

#define TRACE_NEW new(__FILE__, __LINE__)
#define TRACE_DELETE g_lock.lock(); \
                     if(DEL_LINE != 0 && strcmp(DEL_FILE, "")) \
                        buildStack(); \
                     memset(DEL_FILE, 0x0, sizeof(DEL_FILE)); \
                     strncpy(DEL_FILE, __FILE__, FILENAME_LEN - 1); \
                     DEL_LINE = __LINE__; \
                     delete

typedef struct {
	char		fileName[FILENAME_LEN];	 
	unsigned long	lineNum;	 
} DelInfo;

extern SemMutex g_lock;
extern char DEL_FILE[FILENAME_LEN];
extern int DEL_LINE;

void* operator new(size_t size) throw(bad_alloc);
void* operator new[](size_t size) throw(bad_alloc);
void* operator new(size_t size, const char* fileName, const int lineNum) throw(bad_alloc);
void* operator new[](size_t size, const char* fileName, const int lineNum) throw(bad_alloc);

void operator delete(void* address);
void operator delete[](void* address);


void buildStack(); 

class MemTracer {
public:
	MemTracer();
	virtual ~MemTracer();
	
	void insert(void* address, Operation *OP);
	bool erase(void* address, Operation *OP);

private:
    void parseError(const char* file, const int lineNum, const int err); 

private:
	SemMutex m_mutex;
	int m_msgQueue;		
    const char *m_msgPath;
    FILE* m_fp;
};

#else

class MemTracer {};

#endif	

#endif
