#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <pwd.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "mem_monitor.h"

MemMonitor::MemMonitor(char* msgPath, pid_t pid) {
    m_msgPath = msgPath;
    m_pid = pid;
    win = NULL; 
    m_msgQueue = -1;
    m_totalLeak = 0;
    m_interval = 1;
    pthread_mutex_init(&m_mapMutex, NULL);
}

MemMonitor::~MemMonitor() {
    pthread_mutex_destroy(&m_mapMutex);
}

void MemMonitor::parseError(int err) {
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

void MemMonitor::start() {
    FILE *fp = NULL;
    key_t key = -1;

    if((fp = fopen(m_msgPath, "r")) == NULL) {
        cout << "MemMonitor: Cannot not open message queue file: " << m_msgPath << endl;
        exit(1);
    }

    key = ftok(m_msgPath, 'a');
    if((m_msgQueue = msgget(key, 0)) == -1) {
        parseError(errno);
        fclose(fp);
        exit(1);
    }

    initScreen();
}

void MemMonitor::warningWin(char* msg) {
    const char* prompt = "Press AnyKey to Exit";
    WINDOW* warningWin = newwin(4, 60, LINES / 2, COLS / 2);
    box(warningWin, '!', '!');

    if(msg) 
        mvwaddstr(warningWin, 1, (60 - strlen(msg)) / 2, msg);

    mvwaddstr(warningWin, 2, (60 - strlen(prompt)) / 2, prompt);
    touchwin(warningWin);
    wrefresh(warningWin);
    getch();
    touchwin(stdscr);
    refresh();
}

void MemMonitor::initScreen() {
    initscr();
    cbreak();
    noecho();
    nonl();
    mvprintw(0, 0, "ProcessID: %d\tInterval: %d s", m_pid, m_interval);
    attrset(A_REVERSE);
    mvprintw(3, 0, "%-24s%-12s%-24s%-10s\n",
            "File Name", "Line Num", "Allocated(Byte)", "Percent(%)");
    attrset(A_REVERSE);
    refresh();
}

map<void*, MemStatus>& MemMonitor::getStatusMap() {
    return m_mapMemStatus;
}

int MemMonitor::getMsgQueue() {
    return m_msgQueue;
}

int list<MemStatus>& MemMonitor::getLeakMemList() {
    return m_listLeakMem;
}

void MemMonitor::lock() {
    pthread_mutex_lock(&m_mapMutex);
}

void MemMonitor::unlock() {
    pthread_mutex_unlock(&m_mapMutex);
}

void MemMonitor::addLeak(unsigned long leakByte) {
    m_totalLeak += leakByte;
}

void* MemMonitor::analyseMsg(void* arg) {
    MemMonitor* monitor = (MemMonitor*)arg;
    int msgQueue = monitor->getMsgQueue();
    map<void, MemStatus> memStatusMap = monitor->getStatusMap();
    list<MemStatus> listLeakMem = monitor->getLeakMemList();
    MsgEntity recvMsg;
    map<void*, MemStatus>::iterator map_iter;
    list<memStatus>::iterator list_iter;

    while(true) {
        if(msgrev(msgQueue, &recvMsg, sizeof(recvMsg.OP), MSG_TYPE, 0) != -1) {
            map_iter = memStatusMap.find(recvMsg.OP.address);

            if(map_iter != memStatusMap.end()) {
                monitor->lock();  
                if(recvMsg.OP.type == ARRAY_NEW || recvMsg.OP.type == SINGLE_NEW) {
                    monitor->addLeak(recvMsg.OP.size); 
                    map_iter->second.size += recvMsg.OP.size;
                    map_iter->second.type = recvMsg.OP.type;
                }
                if(recvMsg.OP.type == SINGLE_DELETE || recvMsg.OP.type == ARRAY_DELETE) {
                    if(recvMsg.OP.type == SINGLE_DELETE && map_iter->second.type == ARRAY_NEW) {
                        for(list_iter = listLeakMem.begin(); list_iter != listLeakMem.end(); list_iter++) {
                            if(!strcasecmp(list_iter->fileName, map_iter->second.fileName) && 
                               (list_iter->LineNum == map_iter->seond->LineNum)) {
                                    
                               }
                        }
                    }
                }
            }
        }
    } //end while
}

void* MemMonitor::display(void* arg) {
    
} 

int main(int argc, char *argv[]) {
    int ch;
    char msgPath[FILENAME_LEN];
    pid_t pid;

    if(argc < 3) {
        cout << "Usage: -p [ProcessID]" << endl;
        exit(1);
    }

    while(true) {	
        if (-1 == (ch = getopt(argc, argv, "p" ))) {
            break;	
        }
        switch(ch) {
        case 'p':
            pid = atol(argv[optind]);
            break;
        default:
                break;
        }	
    }

    sprintf(msgPath, "/tmp/mem_tracer%d", pid);

    MemMonitor monitor(msgPath, pid);
    
    return 0;
}
