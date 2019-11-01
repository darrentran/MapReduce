CC		= g++
CFLAGS 	= -Wall -Werror -pthread -std=c++14
SOURCES	= $(wildcard *.cpp)
OBJECTS	= $(SOURCES:%.cpp=%.o)


.PHONY: all clean compile compress

all: wc

clean:
	rm -f *.o wordcount

cr:
	rm -f result-*.txt

compress:
	zip wordcount.zip README.md Makefile *.cpp *.h

run-test:
	./wordcount ./a2-testcase/sample1.txt ./a2-testcase/sample2.txt ./a2-testcase/sample3.txt ./a2-testcase/sample4.txt ./a2-testcase/sample5.txt ./a2-testcase/sample6.txt ./a2-testcase/sample7.txt ./a2-testcase/sample8.txt ./a2-testcase/sample9.txt ./a2-testcase/sample10.txt ./a2-testcase/sample11.txt ./a2-testcase/sample12.txt ./a2-testcase/sample13.txt ./a2-testcase/sample14.txt ./a2-testcase/sample15.txt ./a2-testcase/sample16.txt ./a2-testcase/sample17.txt ./a2-testcase/sample18.txt ./a2-testcase/sample19.txt ./a2-testcase/sample20.txt 

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@ -g

compile: $(OBJECTS)

wc: $(OBJECTS)
	$(CC) $(CFLAGS) -o wordcount $(OBJECTS)

