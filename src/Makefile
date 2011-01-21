CC = gcc
CFLAGS = -Wall -std=c99 -Werror -I src -fmessage-length=80
LDFLAGS = -lpthread -lev -llog4c -lconfig

#ifdef DEBUG
#   CFLAGS += -ggdb
#else
#   CFLAGS += -O2
#endif

CFLAGS += -ggdb

all: spade

spade: spade.o csapp.o http.o util.o server.o config.o cgi.o

clean:
	rm -f *.o spade *~
