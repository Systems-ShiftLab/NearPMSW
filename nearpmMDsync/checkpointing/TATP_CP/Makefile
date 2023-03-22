CC=g++
CFLAGS= -std=c++11 
LDFLAGS=
SOURCES=tatp_db.cc tatp_nvm.cc ../include/txopt.cc -lpmem -lpthread 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=tatp_nvm

all:
	    $(CC) $(SOURCES) -o ${EXECUTABLE} $(CFLAGS)
