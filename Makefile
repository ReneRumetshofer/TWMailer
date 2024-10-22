CC=g++

CFLAGS=-g -Wall -Wextra -O -std=c++17 -pthread
LIBS=-lldap -llber

rebuild: clean all
all: ./target/server ./target/client

clean:
	clear
	rm -f target/* obj/*

./obj/message.o: server/message.cpp
	${CC} ${CFLAGS} -o obj/message.o server/message.cpp -c

./obj/handlers.o: server/handlers.cpp
	${CC} ${CFLAGS} -o obj/handlers.o server/handlers.cpp -c

./obj/utilities.o: server/utilities.cpp
	${CC} ${CFLAGS} -o obj/utilities.o server/utilities.cpp -c

./obj/server.o: server/server.cpp
	${CC} ${CFLAGS} -o obj/server.o server/server.cpp -c

./target/server: ./obj/message.o ./obj/handlers.o ./obj/utilities.o ./obj/server.o
	${CC} ${CFLAGS} -o target/server ./obj/message.o ./obj/handlers.o ./obj/utilities.o ./obj/server.o ${LIBS}

./obj/client.o: client/client.cpp
	${CC} ${CFLAGS} -o obj/client.o client/client.cpp -c

./target/client: ./obj/client.o
	${CC} ${CFLAGS} -o target/client ./obj/client.o ${LIBS}
