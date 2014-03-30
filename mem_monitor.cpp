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

char* MemMonitor::parseError(int err) {
      char* prompt = NULL;
      switch(err) {
        case EACCES:
            prompt = "EACCES";
            break;
        case  EAGAIN:
            prompt = "EAGAIN";
            break;
        case EFAULT:
            prompt = "EFAULT";
            break;
        case EIDRM:
            prompt = "EIDRM";
            break; 
        case EINTR:
            prompt = "EINTR";
            break;
        case EINVAL:
            prompt = "EINVAL";
            break;
        case ENOMEM:
            prompt = "ENOMEM";
            break;
        case E2BIG:
            prompt = "E2BIG";
            break;
        case ENOMSG:
            prompt = "ENOMSG";
            break;
        default:
            prompt = "UnKnown Error Code";
            break;
    }
    return prompt;
}

void MemMonitor::analyseMsg() {
    MsgEntity recvMsg;
    map<void*, MemStatus>::iterator map_iter;
    list<MemStatus>::iterator list_iter;

    while(true) {
        if(msgrev(m_msgQueue, &recvMsg, sizeof(recvMsg.OP), MSG_TYPE, 0) == -1) {
            char *prompt = NULL;
            prompt = parseError(errno);
            warningWin(prompt);
            endwin();
            break;
        } 

        pthread_mutex_lock(&m_mapMutex); 

        if(recvMsg.OP.type == SINGLE_NEW || recvMsg.OP.type == ARRAY_NEW) {
            map_iter = m_mapMemStatus.find(recvMsg.OP.address);
            if(map_iter == m_mapMemStatus.end()) {
                MemStatus memStatus;
                memcpy(&memStatus, 0x0, sizeof(MemStatus));
                m_totalLeak += recvMsg.OP.size;
                strncpy(memStatus.fileName, recvMsg.OP.fileName, FILENAME_LEN - 1);
                memStatus.lineNum = recvMsg.OP.lineNum; 
                memStatus.address = recvMsg.OP.address;
                memStatus.type = recvMsg.OP.type;
                memStatus.totalSize += recvMsg.OP.size;
                m_mapMemStatus.insert(pair<void*, MemStatus>(recvMsg.OP.address, memStatus));
            } else {
                m_totalLeak += recvMsg.OP.size;
                MemStatus memStatus;
                memcpy(&memStatus, 0x0, sizeof(MemStatus));
                strncpy(memStatus.fileName, map_iter->second.fileName, FILENAME_LEN - 1);
                memStatus.lineNum = map_iter->second.lineNum;
                memStatus.address = map_iter->second.address;
                memStatus.totalSize = map_iter->second.totalSize;
                memStatus.type = map_iter->second.type;
                m_listLeakMem.push_back(memStatus);

                map_iter->second.totalSize += recvMsg.OP.size;
                map_iter->second.type = recvMsg.OP.type;
                strncpy(map_iter->second.fileName, recvMsg.OP.fileName, FILENAME_LEN - 1);
                map_iter->second.lineNum = recvMsg.OP.lineNum;
            }

            pthread_mutex_unlock(&m_mapMutex);
            continue;
        } // end if: recv new or array new

       if(recvMsg.OP.type == SINGLE_DELETE || recvMsg.OP.type == ARRAY_DELETE) {
           map_iter = m_mapMemStatus.find(recvMsg.OP.address);
           if(map_iter == m_mapMemStatus.end()) {

                pthread_mutex_unlock(&m_mapMutex);

                char *prompt = "You delete a pointer not traced!";
                warningWin(prompt);
                endwin();
                break;    
           }

           if(recvMsg.OP.type == SINGLE_DELETE && map_iter->second.type == SINGLE_NEW) {
                m_totalLeak -= recvMsg.OP.size;
                m_mapMemStatus.erase(map_iter);
                
                pthread_mutex_unlock(&m_mapMutex);
                continue;
           }

          if(recvMsg.OP.type == ARRAY_DELETE && map_iter->second.type == ARRAY_NEW) {
                m_totalLeak -= recvMsg.OP.size;
                m_mapMemStatus.erase(map_iter);

                pthread_mutex_unlock(&m_mapMutex);
                continue;
           }

           bool found = false;
           if(recvMsg.OP.type == SINGLE_DELETE && map_iter->second.type == ARRAY_NEW) {
               for(list_iter = m_listLeakMem.begin(); list_iter != m_listLeakMem.end(); list_iter++) {
                    if(!strcasecmp(list_iter->fileName, map_iter->second.fileName) && list_iter->lineNum == map_iter->second.lineNum) {
                        found = true;
                        list_iter->totalSize  += map_iter->second.totalSize;
                        m_mapMemStatus.erase(map_iter);

                        pthread_mutex_unlock(&m_mapMutex);
                        break;
                    } // end if
               } // end for
               if(!found) {
                   m_listLeakMem.push_back(map_iter->second);
                   found = true;
               }

               pthread_mutex_unlock(&m_mapMutex);
               continue;
           } // end if
           
           /* This cannot happen ever. */
           if(recvMsg.OP.type == ARRAY_DELETE && map_iter->second.type == SINGLE_NEW) {
               cout << "Fatal Error" << endl; 
               exit(1); 
           }

       }// end if : recv delete or array delete

    } //end while
}

void MemMonitor::display() {
    
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
