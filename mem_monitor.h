#ifndef __MEMMONITOR_H
#define __MEMMONITOR_H

#include<iostream>
#include <string.h>
#include <map>
#include <curses.h>
#include <pthread.h>
#include <list>
#include "common.h"

using namespace std;

class  MemMonitor {
public:
    MemMonitor();
    virtual ~MemMonitor();

    void init(const char* msgPath, const int interval);
    void start();
    void stop();
    void analyseMsg();
    void display();

private:
    const char* parseError(const int err);
    void initScreen();
    void warningWin(const char* msg);

private:
    static void* analyseRoutine(void* arg);
    static void* displayRoutine(void* arg);

private:
    int m_msgQueue;
    const char* m_msgPath;
    FILE* m_fp;
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
