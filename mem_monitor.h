#ifndef __MEMMONITOR_H
#define __MEMMONITOR_H

#include<iostream>
#include <string.h>
#include <map>
#include <curses.h>
#include <pthread.h>
#include <list>
#include "mem_checker_header.h"

using namespace std;

class  MemMonitor {
public:
    MemMonitor(char* msgPath, pid_t pid);
    virtual ~MemMonitor();

    void start();
    int getMsgQueue();
    map<void*, MemStatus>& getStatusMap();
    list<MemStatus>& getLeakMemList();
    void lock(); 
    void unlock();
    void addLeak(unsigned long leakByte);

private:
    void analyseMsg();
    void display();
    char* parseError(int err);

private:
    void initScreen();
    void warningWin(char* msg);
    void parseError(int err);

private:
    int m_msgQueue;
    char* m_msgPath;
    pid_t m_pid;
    map<void*, MemStatus> m_mapMemStatus;
    list<Memstatus> m_listLeakMem;
    unsigned long m_totalLeak;
    unsigned int m_interval;
    WINDOW *win;
    pthread_mutex_t m_mapMutex;
};

#endif
