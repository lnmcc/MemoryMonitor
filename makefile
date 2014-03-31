OBJS = mem_tracer.o sem_mutex.o 
CFLAGS = -Wall -g -DMEM_TRACE
LFLAGS = -lpthread -lcurses -lrt

all : demo.o mem_monitor.o $(OBJS)
	g++ -g -o demo demo.o $(OBJS)  $(LFLAGS) $(LFLAGS) 
	g++ -g -o mem_monitor mem_monitor.o sem_mutex.o $(LFLAGS) 

mem_tracer.o : mem_tracer.cpp 
	g++ -c $(CFLAGS) mem_tracer.cpp 

mem_monitor.o : mem_monitor.cpp
	g++ -c $(CFLAGS) mem_monitor.cpp

demo.o : demo.cpp 
	g++ -c $(CFLAGS) demo.cpp		

sem_mutex.o : sem_mutex.cpp 
	g++ -c $(CFLAGS) sem_mutex.cpp 
			
clean:
	rm -f *.o demo mem_monitor
