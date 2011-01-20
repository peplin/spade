CC = gcc
CFLAGS = -Wall -std=c99 -Werror -I src -fmessage-length=80
LDFLAGS = -lpthread -lev -llog4c -lconfig

#ifdef DEBUG
#   CFLAGS += -ggdb
#else
#   CFLAGS += -O2
#endif

CFLAGS += -ggdb

all: dirt

dirt: dirt.o csapp.o http.o util.o server.o config.o

clean:
	rm -f *.o dirt *~
