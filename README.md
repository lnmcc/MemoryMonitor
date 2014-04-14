MemoryMonitor
=============

一个用于C++语言的内存泄露检测工具

### 使用方法
  1. 在你需要检测的代码中加入头文件
  <pre><code>
  #include "mem_tracer.h"
  </pre></code>
  2. 增加宏定义
   <pre><code>
    #ifdef MEM_TRACE
    #define new     TRACE_NEW
    #define delete  TRACE_DELETE
    #endif
   </pre></code>
  3. 编译时增加定义 
  <pre><code>
  -DMEM_TRACE
	</pre></code>
  4. 运行mem\_monitor
  5. 允许被检测程序
  6. 停止mem\_monitor: 按2次CTRL+C
  
### DEMO

### BUGS
  1. 输出记录不能滚动
  2. 被检测程序退出后的处理有问题

