MemoryMonitor
=============

一个用于C++语言的内存泄露检测工具

###使用方法：
  1. 在你需要检测的代码中加入头文件： #include "mem_tracer.h"
  2. 增加宏定义：
          #ifdef MEM_TRACE
          #define new     TRACE_NEW
          #define delete  TRACE_DELETE
          #endif
  3. 编译时增加定义: -DMEM_TRACE
  4. 运行mem_monitor
  5. 允许被检测程序
  
###DEMO：

###BUGS：
  
