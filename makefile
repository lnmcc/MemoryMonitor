OBJS = mem_tracer.o sem_mutex.o demo.o
DEBUG_FLAGS = -O0 -DMEM_TRACE -g

all : $(OBJS) 
	g++ -g -o demo  $(OBJS) -lpthread

mem_tracer.o : mem_tracer.cpp 
	g++ -c $(DEBUG_FLAGS) mem_tracer.cpp 

demo.o : demo.cpp 
	g++ -c $(DEBUG_FLAGS) demo.cpp		

sem_mutex.o : sem_mutex.cpp 
	g++ -c sem_mutex.cpp 
			
clean:
	rm -f *.o demo 
