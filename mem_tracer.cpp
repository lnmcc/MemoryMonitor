#include <iostream>
#include <stdio.h>
#include <string.h>
#include <map>
#include <list>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <pwd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/msg.h>

#include "mem_tracer.h"

using namespace std;

#ifdef  MEM_TRACE

MemTracer g_memTracer;

typedef struct {
	char		fileName[FILENAME_LEN];	 
	unsigned long	lineNum;	 
} DelInfo;

char DEL_FILE[FILENAME_LEN] = {0};
int DEL_LINE = 0;
SemMutex g_lock;
stack<DelInfo> g_delInfoStack;

static void buildStack() {
	DelInfo lastDel = {"", 0};
	strcpy(lastDel.fileName, DEL_FILE);
	lastDel.lineNum = DEL_LINE;
	g_delInfoStack.push(lastDel);
}

void* operator new(size_t size, char* fileName, int lineNum) {
	Operation OP;
	void* address = NULL;
	address = ::operator new(size);
	
	if (address) {
        memset(&OP, 0x0, sizeof(Operation));
		strncpy(OP.fileName, fileName, FILENAME_LEN - 1);
		OP.lineNum = lineNum;
		OP.size = size;
		OP.type = SINGLE_NEW;
		OP.errCode = 0;
		OP.address = address;
		g_memTracer.insert(address, &OP);
	}
	
	return address;
}

void* operator new[](size_t size, char* fileName, int lineNum) {
	Operation OP;
	void *address;
	
	address = ::operator new[](size);
	
	if (address) {
        memset(&OP, 0x0, sizeof(Operation));
		strncpy(OP.fileName, fileName, FILENAME_LEN - 1);
		OP.lineNum = lineNum;
		OP.size = size;
		OP.type = ARRAY_NEW;
		OP.errCode = 0;
		OP.address = address;
		g_memTracer.insert(address, &OP);
	}

	return address;
}

void operator delete(void* address) {
	if (NULL == address) {
		return;
	}

    g_lock.lock();

    if(DEL_LINE != 0 && !strcmp(DEL_FILE, ""))  
        buildStack();

    memset(DEL_FILE, 0x0, sizeof(DEL_FILE));
    strncpy(DEL_FILE, __FILE__, FILENAME_LEN - 1);
    DEL_LINE = __LINE__;

    Operation OP;
    memset(&OP, 0x0, sizeof(Operation));
	strncpy(OP.fileName, DEL_FILE, FILENAME_LEN - 1);
	OP.lineNum = DEL_LINE;
	OP.size = 0; 
	OP.type = SINGLE_DELETE;
	OP.errCode = 0;
	OP.address = address;

    memset(DEL_FILE, 0x0, sizeof(DEL_FILE));
	DEL_LINE = 0;

    g_lock.unlock();
		
	g_memTracer.erase(address, &OP);
	free(address);
}

void operator delete[](void* address) {
	if (NULL == address) {
		return;
	}

    g_lock.lock();

    if(DEL_LINE != 0 && !strcmp(DEL_FILE, ""))  
        buildStack();

    memset(DEL_FILE, 0x0, sizeof(DEL_FILE));
    strncpy(DEL_FILE, __FILE__, FILENAME_LEN - 1);
    DEL_LINE = __LINE__;

    Operation OP;
    memset(&OP, 0x0, sizeof(Operation));
	strncpy(OP.fileName, DEL_FILE, FILENAME_LEN - 1);
	OP.lineNum = DEL_LINE;
	OP.size = 0; 
	OP.type = ARRAY_DELETE;
	OP.errCode = 0;
	OP.address = address;

    memset(DEL_FILE, 0x0, sizeof(DEL_FILE));
	DEL_LINE = 0;

    g_lock.unlock();
		
	g_memTracer.erase(address, &OP);
	free(address);
}

MemTracer::MemTracer() {	
	key_t key;
	int flags, counter = 0;

	struct passwd *pwd;
	FILE *fp = NULL;

	sprintf(m_msgPath, "/tmp/mem_tracer%d", getpid());

	if((fp = fopen(m_msgPath, "a+")) == NULL) {
		cerr << "MemTracer: Cannot create message queue: " <<  m_msgPath << endl; 
		exit( 1 );
	}

	if((key = ftok(m_msgPath, 'a')) == -1 ) {
		cerr << "MemTracer: Cannot create KEY for message queue" << endl;
		exit(1);
	}

	if((m_msgQueue = msgget(key, IPC_CREAT | IPC_EXCL | 0777)) == -1) {

        parseError(__FILE__, __LINE__, errno);

        if(errno == EEXIST) {

            cerr << "MemTracer: Message queue has exist, try to delete it" << endl;

            if (msgctl(m_msgQueue, IPC_RMID, NULL) == -1) {
                cerr << "MemTracer: Cannot delete message queue" << endl;
                exit(1);
            }
            m_msgQueue = msgget(key, IPC_CREAT | IPC_EXCL | 0777);

        } else {
            cerr << "MemTracer: Cannot create message queue" << endl;
            exit(1);
        }    
	}
}

MemTracer::~MemTracer() {
    if(msgctl(m_msgQueue, IPC_RMID, NULL) == -1) {
        cerr << "Cannot delete message queue, mybe you need delete file: " 
             << m_msgPath << " yourself" << endl;
    }
    unlink(m_msgPath);
}

void MemTracer::parseError(char* file, int lineNum, int err) {
    cerr << "Error: " << file << ": " << lineNum << ": ";
	switch(err) {
	case EAGAIN:
		cerr << "---EAGAIN---" << endl;
		break;
	case EACCES:
		cerr << "---EACCES---" << endl;
		break;
	case EFAULT:
		cerr << "---EFAULT---" << endl;
		break;
	case EIDRM:
		cerr << "---EIDRM---" << endl;
		break;
	case EINTR:
		cerr << "---EINTR---" << endl;
		break;
	case EINVAL:
		cerr << "---EINVAL---" << endl;
		break;
	case ENOMEM:
		cerr << "---ENOMEM---" << endl;
		break;
	default:
	    cerr << "---Undefined Error---" << endl;
		break;	
	}
}

void MemTracer::insert(void* address, Operation* OP) {
	MsgEntity sendMsg;
    struct msqid_ds	msgQueInfo;

	if(!address || !OP) {
        cerr << "MemTracer: Error address or operation" << endl;
		return; 
	}

	m_mutexMap.lock();

	memcpy((void*) &sendMsg.OP, OP, sizeof(Operation));
	
	if(m_msgQueue != -1) {
		
		if(0 == msgctl(m_msgQueue, IPC_STAT, &msgQueInfo)) {
			if(msgQueInfo.msg_qbytes > (msgQueInfo.msg_cbytes + sizeof(Operation))) {
				sendMsg.OP.errCode = 0; 
				sendMsg.type = MSG_TYPE;
				if(msgsnd(m_msgQueue, &sendMsg, sizeof(sendMsg.OP), IPC_NOWAIT) == -1) 
					parseError(__FILE__, __LINE__, errno);
			}
		} else 
			parseError(__FILE__, __LINE__, errno);
	}

	m_mapOP.insert(pair<void*, Operation>(address, sendMsg.OP));

	m_mutexMap.unlock();
}

bool MemTracer::erase(void* address, Operation* OP) {
	map<void*, Operation>::iterator iter_mapOP;
	MsgEntity sendMsg;
	struct msqid_ds	msgQueInfo;

	if (!address || !OP) {
        cerr << "MemTracer: you want to erase a error address or operation";
		return false;
    }

	if(!strcmp(OP->fileName,"") && OP->lineNum == 0) {
		iter_mapOP = m_mapOP.find(address);
        if(iter_mapOP == m_mapOP.end()) {
            cout << "MemTracer: Cannot not find any information for the address" << endl;
			return false;
        }

		if(!g_delInfoStack.empty()) {
			DelInfo del = g_delInfoStack.top();
			strcpy(OP->fileName, del.fileName);
			OP->lineNum = del.lineNum;
			g_delInfoStack.pop();
		}
	}
	
	if(m_msgQueue != -1) {
		if(msgctl(m_msgQueue, IPC_STAT, &msgQueInfo) == 0) {
			if(msgQueInfo.msg_qbytes > (msgQueInfo.msg_cbytes + sizeof(Operation))) {
				memcpy((void *)&sendMsg.OP, OP, sizeof(Operation));
				sendMsg.OP.errCode = 1;
				sendMsg.type = MSG_TYPE;
				if(msgsnd(m_msgQueue, &sendMsg, sizeof(sendMsg.OP), IPC_NOWAIT) == -1)
					parseError(__FILE__, __LINE__, errno);
            }	
		} else 
			parseError(__FILE__, __LINE__, errno);
	}

	m_mutexMap.lock();

	iter_mapOP = m_mapOP.find(address);

	if(iter_mapOP != m_mapOP.end()) {
		Operation tmp;

		if(SINGLE_DELETE == OP->type && ARRAY_NEW == iter_mapOP->second.type) {
			memcpy(&tmp, &(iter_mapOP->second), sizeof(Operation));
			tmp.errCode = 1;
			m_listErrOP.push_back(tmp);
		}		

		m_mapOP.erase(iter_mapOP);

		m_mutexMap.unlock();
		return true;

	} else {
        printf("MemTracer : *****WARNING: %s: %ld : %p, Delete a pointer cannot find in MAP.******\n", OP->fileName, OP->lineNum , address);
		m_mutexMap.unlock();
		return false;
	}
}

#endif
