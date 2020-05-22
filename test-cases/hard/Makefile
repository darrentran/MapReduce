CC:=g++
WARN:=-Wall 
LIB:=-lm -pthread -O3
CCOPTS:=-std=c++11 -ggdb -D_GNU_SOURCE
TARGET:=distwc

$(TARGET): ../threadpool.o ../mapreduce.o distwc.o
	$(CC) $(WARN) $(CCOPTS) $^ -o $@ $(LIB)
	g++ -o validation validation.cc -std=c++11 -Wall -O3

%.o: %.c
	$(CC) $(WARN) $(CCOPTS) $< -c $(LIB)

%.o: %.cpp
	$(CC) $(WARN) $(CCOPTS) $< -c $(LIB)

clean-result: 
	rm -rf result-*.txt
clean-all:
	rm -rf *.o *.gch $(TARGET)
	rm -rf result-*.txt
	rm -rf validation

threadpool.o: ../threadpool.cpp ../threadpool.h
distwc.o: distwc.cpp ../mapreduce.h
mapreduce.o: ../mapreduce.cpp ../mapreduce.h ../threadpool.h
