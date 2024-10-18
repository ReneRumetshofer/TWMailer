CC=g++

CFLAGS=-g -Wall -Wextra -O -std=c++17 -pthread
LIBS=-lldap -llber

rebuild: clean all
all: ./target/server

clean:
	clear
	rm -f target/* obj/*

./obj/server.o: server.cpp
	${CC} ${CFLAGS} -o obj/server.o server.cpp -c

./target/server: ./obj/server.o
	${CC} ${CFLAGS} -o target/server obj/server.o ${LIBS}
