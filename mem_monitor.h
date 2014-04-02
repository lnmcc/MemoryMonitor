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
    MemMonitor();
    virtual ~MemMonitor();

    void init(char* msgPath, pid_t pid, int interval);
    void start();
    void stop();
    void analyseMsg();
    void display();

private:
    char* parseError(int err);
    void initScreen();
    void warningWin(char* msg);

private:
    static void* analyseRoutine(void* arg);
    static void* displayRoutine(void* arg);

private:
    int m_msgQueue;
    char* m_msgPath;
    FILE* m_fp;
    pid_t m_pid;
    pthread_t m_disp_pid;
    pthread_t m_analyse_pid;
    map<void*, MemStatus> m_mapMemStatus;
    list<MemStatus> m_listLeakMem;
    unsigned long m_totalLeak;
    unsigned int m_interval;
    WINDOW *win;
    pthread_mutex_t m_mapMutex;
    bool running;
};

#endif
