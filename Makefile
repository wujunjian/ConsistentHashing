OBJECT= ConsistentHashing.o test_main.o
CPP=g++ -g -O2
INCLUDES:=-I. 
LIB:=-lpthread

all: test_main 
test_main:$(OBJECT)
	$(CPP) $(FLAG) $^ -o $@ $(LIB)

%.o : %.cpp
	$(CPP) -c $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $< 

clean:
	@rm -rf *.o
