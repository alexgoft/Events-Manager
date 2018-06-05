CC = g++
STD = -std=gnu++11

EM_CLIENT = emClient.cpp
EM_SERVER = emServer.cpp
EM_FILES = Log.cpp Log.h Utils.cpp Utils.h

TAROBJECTS = ${EM_CLIENT} ${EM_SERVER} $(EM_FILES) README Makefile

CFLAGS = -pthread -Wextra -Wvla -Wall

all: emClient emServer

emClient: $(EM_SERVER) $(EM_FILES)
	${CC} $(STD) ${CFLAGS} ${EM_CLIENT} $(EM_FILES) -o emClient

emServer: $(EM_SERVER) $(EM_FILES)
	${CC} $(STD) ${CFLAGS} ${EM_SERVER} $(EM_FILES) -o emServer

tar:
	tar cvf ex5.tar ${TAROBJECTS}

clean:
	rm -f ex5.tar emClient emServer

.PHONY: all emClient emServer tar clean
