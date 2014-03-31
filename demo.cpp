#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <list>
#include <iostream>
#include <string.h>
#include "mem_tracer.h"

#ifdef MEM_TRACE
#define new     TRACE_NEW
#define delete  TRACE_DELETE
#endif

using namespace std;

class A {
public:
    A() {}
    ~A() {}

private:
    char ch[1024];
};

int main() {
    int num = 0;

    while(1) {
        A* a = new A();
        sleep(1);
        cout << ++num << endl;
    }

	return 0;
}
