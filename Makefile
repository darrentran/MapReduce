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

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@ -g

compile: $(OBJECTS)

wc: $(OBJECTS)
	$(CC) $(CFLAGS) -o wordcount $(OBJECTS)

