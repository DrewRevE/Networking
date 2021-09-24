CC=g++
CXX=	g++	-O2	-Wall
CFLAGS=-Wall -Iincludes -Wextra -std=c99 -ggdb
LDLIBS=-lcrypto
VPATH=src

all:    server  client

client: client.o

server: server.o

client.o:   client.cpp 

server.o:   server.cpp

clean:  rm -rf *~ *.o test  rm server   rm client
.PHONY : clean all