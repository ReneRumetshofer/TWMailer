CC=g++

CFLAGS=-g -Wall -Wextra -std=c++17 -pthread
LIBS=-lldap -llber -lpthread

rebuild: clean all
all: ./target/server ./target/client

clean:
	clear
	rm -f target/* obj/*

./obj/message.o: shared/message.cpp
	${CC} ${CFLAGS} -o obj/message.o shared/message.cpp -c

./obj/handlers.o: server/handlers.cpp
	${CC} ${CFLAGS} -o obj/handlers.o server/handlers.cpp -c

./obj/blacklist.o: server/blacklist.cpp
	${CC} ${CFLAGS} -o obj/blacklist.o server/blacklist.cpp -c

./obj/utilities.o: shared/utilities.cpp
	${CC} ${CFLAGS} -o obj/utilities.o shared/utilities.cpp -c

./obj/ldap_auth.o: server/ldap_auth.cpp
	${CC} ${CFLAGS} -o obj/ldap_auth.o server/ldap_auth.cpp -c

./obj/server.o: server/server.cpp
	${CC} ${CFLAGS} -o obj/server.o server/server.cpp -c

./target/server: ./obj/message.o ./obj/handlers.o ./obj/blacklist.o ./obj/utilities.o ./obj/ldap_auth.o ./obj/server.o
	${CC} ${CFLAGS} -o target/server ./obj/message.o ./obj/handlers.o ./obj/utilities.o ./obj/blacklist.o ./obj/ldap_auth.o ./obj/server.o ${LIBS}

./obj/client.o: client/client.cpp
	${CC} ${CFLAGS} -o obj/client.o client/client.cpp -c

./target/client: ./obj/client.o
	${CC} ${CFLAGS} -o target/client ./obj/message.o ./obj/utilities.o ./obj/client.o ${LIBS}
