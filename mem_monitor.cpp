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
#include <signal.h>
#include <mqueue.h>
#include "mem_monitor.h"

MemMonitor::MemMonitor() {
    m_msgPath = NULL;
    m_pid = 0;
    m_fp = NULL;
    win = NULL; 
    m_msgQueue = -1;
    m_totalLeak = 0;
    m_interval = 0;
    running = false;

    pthread_mutex_init(&m_mapMutex, NULL);
}

MemMonitor::~MemMonitor() {
    fclose(m_fp);
    pthread_mutex_destroy(&m_mapMutex);
}


void MemMonitor::init(char* msgPath, pid_t pid, int interval) {
    m_msgPath = msgPath;
    m_pid = pid;
    m_interval = interval;   

    key_t key = -1;

    sprintf(m_msgPath, "/home/sijiewang/mem_tracer");

     if((m_fp = fopen(m_msgPath, "a+")) == NULL) {
        cout << "MemMonitor: Cannot not open message queue file: " << m_msgPath << endl;
        exit(1);
    }

	if((key = ftok(m_msgPath, 'a')) == -1 ) {
		cerr << "MemMonitor: Cannot create KEY for message queue" << endl;
        fclose(m_fp);
		exit(1);
	}

    while((m_msgQueue = msgget(key, IPC_CREAT | IPC_EXCL | 0777)) == -1) {

            parseError(errno);

            if(errno == EEXIST) {

                cerr << "MemTracer: Message queue has exist, try to delete it" << endl;

                if (msgctl(m_msgQueue, IPC_RMID, NULL) == -1) {
                    cerr << "MemTracer: Cannot delete message queue" << endl;
                    parseError(errno);
                }
           
                sleep(1);

            } else {
                cerr << "MemTracer: Cannot create message queue" << endl;

                fclose(m_fp);
                exit(1);
            }    
    }	

    initScreen();
}

void MemMonitor::start() {
    running = true;
    pthread_create(&m_analyse_pid, NULL, analyseRoutine, this);
    pthread_create(&m_disp_pid, NULL, displayRoutine, this);

    pthread_join(m_analyse_pid, NULL);
    pthread_join(m_disp_pid, NULL);
}

void MemMonitor::stop() {
    cout << "Stop" << endl;
    if(msgctl(m_msgQueue, IPC_RMID, NULL) == -1) {
         cerr << "MemTracer: Cannot delete message queue" << endl;
         parseError(errno);
    }   
    fclose(m_fp);
    unlink(m_msgPath);
}

void* MemMonitor::analyseRoutine(void* arg) {
    MemMonitor* mm = (MemMonitor*)arg;
    mm->analyseMsg();
    return NULL;
}

void* MemMonitor::displayRoutine(void* arg) {
    MemMonitor* mm = (MemMonitor*)arg;
    mm->display();
    return NULL;
}

void MemMonitor::warningWin(char* msg) {
    const char* prompt = "Press AnyKey to Exit";
    WINDOW* wWin = newwin(4, 80, LINES / 2 + 2 , COLS / 2 - 40);
    box(wWin, '*', '*');

    if(msg) 
        mvwaddstr(wWin, 1, (60 - strlen(msg)) / 2, msg);

    mvwaddstr(wWin, 2, (60 - strlen(prompt)) / 2, prompt);
    touchwin(wWin);
    wrefresh(wWin);
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
		    cerr << "---EACCES---" << endl;
            prompt = "EACCES";
            break;
        case  EAGAIN:
		    cerr << "---EAGAIN---" << endl;
            prompt = "EAGAIN";
            break;
        case EFAULT:
		    cerr << "---EFAULT---" << endl;
            prompt = "EFAULT";
            break;
        case EIDRM:
		    cerr << "---EIDRM---" << endl;
            prompt = "EIDRM";
            break; 
        case EINTR:
		    cerr << "---EINTR---" << endl;
            prompt = "EINTR";
            break;
        case EINVAL:
		    cerr << "---EINVAL---" << endl;
            prompt = "EINVAL";
            break;
        case ENOMEM:
		    cerr << "---ENOMEM---" << endl;
            prompt = "ENOMEM";
            break;
        case E2BIG:
		    cerr << "---E2BIG---" << endl;
            prompt = "E2BIG";
            break;
        case ENOMSG:
		    cerr << "---ENOMSG---" << endl;
            prompt = "ENOMSG";
            break;
        default:
	        cerr << "---Undefined Error---" << endl;
            prompt = "UnKnown Error Code";
            break;
    }
    return prompt;
}

void MemMonitor::analyseMsg() {
    MsgEntity recvMsg;
    map<void*, MemStatus>::iterator map_iter;
    list<MemStatus>::iterator list_iter;

    while(running) {
        pthread_mutex_lock(&m_mapMutex); 

        if(msgrcv(m_msgQueue, &recvMsg, sizeof(recvMsg.OP), MSG_TYPE, 0) == -1) {
            char *prompt = NULL;
            prompt = parseError(errno);
            warningWin(prompt);
            endwin();
            pthread_mutex_unlock(&m_mapMutex);
            break; //break while
        } 

        if(recvMsg.OP.type == SINGLE_NEW || recvMsg.OP.type == ARRAY_NEW) {
            map_iter = m_mapMemStatus.find(recvMsg.OP.address);
            if(map_iter == m_mapMemStatus.end()) {
                MemStatus memStatus;
                memset(&memStatus, 0x0, sizeof(MemStatus));
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
                memset(&memStatus, 0x0, sizeof(MemStatus));
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

                //pthread_mutex_unlock(&m_mapMutex);

                char *prompt = "You delete a pointer not traced!";
                warningWin(prompt);
                endwin();
                break;  //break while TODO 
           }

           if(recvMsg.OP.type == SINGLE_DELETE && map_iter->second.type == SINGLE_NEW) {
                m_totalLeak -= recvMsg.OP.size;
                m_mapMemStatus.erase(map_iter);
                
                pthread_mutex_unlock(&m_mapMutex);
                continue; // continue while
           }

          if(recvMsg.OP.type == ARRAY_DELETE && map_iter->second.type == ARRAY_NEW) {
                m_totalLeak -= recvMsg.OP.size;
                m_mapMemStatus.erase(map_iter);

                pthread_mutex_unlock(&m_mapMutex);
                continue; // continue while
           }

           bool found = false;
           if(recvMsg.OP.type == SINGLE_DELETE && map_iter->second.type == ARRAY_NEW) {
               for(list_iter = m_listLeakMem.begin(); list_iter != m_listLeakMem.end(); list_iter++) {
                    if(!strcasecmp(list_iter->fileName, map_iter->second.fileName) && list_iter->lineNum == map_iter->second.lineNum) {
                        found = true;
                        list_iter->totalSize  += map_iter->second.totalSize;
                        m_mapMemStatus.erase(map_iter);

                        //pthread_mutex_unlock(&m_mapMutex);
                        break; // break for
                    } // end if
               } // end for
               if(!found) {
                   m_listLeakMem.push_back(map_iter->second);
                   found = true;
               }

               pthread_mutex_unlock(&m_mapMutex);
               continue; // continue while
           } // end if
           
           /* This cannot happen ever. */
           if(recvMsg.OP.type == ARRAY_DELETE && map_iter->second.type == SINGLE_NEW) {
               cout << "Fatal Error" << endl; 

               pthread_mutex_unlock(&m_mapMutex);
               exit(1); 
           }

       }// end if : recv delete or array delete

       pthread_mutex_unlock(&m_mapMutex);
    } //end while
}

void MemMonitor::display() {
    int line = 0;
    time_t sysTime;
    list<MemStatus> disp_list;
    map<void*, MemStatus>::iterator map_iter;
    list<MemStatus>::iterator list_iter, disp_iter;

    while(true) {
        line = 0;
        disp_list.clear();
        time(&sysTime);

        pthread_mutex_lock(&m_mapMutex);

        for(map_iter = m_mapMemStatus.begin(); map_iter != m_mapMemStatus.end(); map_iter++) {
            for(disp_iter = disp_list.begin(); disp_iter != disp_list.end(); disp_iter++) {
                if(!strcasecmp(map_iter->second.fileName, disp_iter->fileName)
                   && map_iter->second.lineNum == disp_iter->lineNum) {
                       disp_iter->totalSize += map_iter->second.totalSize;
                       break;
                }
            } // end for

            if(disp_iter == disp_list.end()) {
                disp_list.push_back(map_iter->second); 
            }
        } // end for
    
        for(list_iter = m_listLeakMem.begin(); list_iter != m_listLeakMem.end(); list_iter++) {
            for(disp_iter = disp_list.begin(); disp_iter != disp_list.end(); disp_iter++) {
                if(!strcasecmp(list_iter->fileName, disp_iter->fileName) 
                  && list_iter->lineNum == disp_iter->lineNum) {
                       disp_iter->totalSize += list_iter->totalSize;
                       break;
                  }
            } // end for

            if(disp_iter == disp_list.end()) {
                disp_list.push_back(*list_iter);
            }
        } //end for
#if 1 
        if(m_totalLeak != 0) {
            erase(); 

            mvprintw(line++, 0, "ProcessID:%d, Interval:%ds, Time:%s", m_pid, m_interval, ctime(&sysTime));
            refresh();

            mvprintw(line++, 0, "");		
            mvprintw(line++, 0, "");

            attrset(A_REVERSE);
            mvprintw(line++, 0, "%-24s%-12s%-24s%-10s\n",
                     "FILE", "LINE", "Allocated(Byte)", "Percent(%)");
            attrset(A_NORMAL);
            refresh();

            for (disp_iter = disp_list.begin(); disp_iter != disp_list.end(); disp_iter++) {
                mvprintw(line++, 0, "%-24s%-12d%-24d%2.2f%%",
                         disp_iter->fileName, disp_iter->lineNum,
                         disp_iter->totalSize,
                         (double) disp_iter->totalSize * 100 / m_totalLeak);

            }
            mvprintw(line++, 0, "");
            mvprintw(line++, 0, "Total: %d Bytes(%0.1fKB = %0.1fMB)",
                     m_totalLeak, (double)m_totalLeak / 1024, (double)m_totalLeak / 1024 / 1024 );

            refresh(); 
        }
#endif
        pthread_mutex_unlock(&m_mapMutex);
        sleep(m_interval);
    }// end while
} 

MemMonitor monitor;

void sighandler(int sig) {
    monitor.stop();
}

int main(int argc, char *argv[]) {
    int ch;
    char msgPath[FILENAME_LEN];
    pid_t pid;
    struct sigaction act;

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

    act.sa_handler = sighandler;
    sigaddset(&act.sa_mask, SIGQUIT);
    act.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &act, NULL);

    sprintf(msgPath, "/home/sijiewang/mem_tracer", pid);

    monitor.init(msgPath, pid, 1);
    monitor.start();    

    return 0;
}
