CC=g++
CFLAGS= -std=c++11  -lpmem -lpthread 
SOURCES=../include/txopt.cc tpcc_db.cc tpcc_nvm.cc
EXECUTABLE=tpcc_nvm

all: 
	$(CC) $(SOURCES) -o ${EXECUTABLE} $(CFLAGS)
