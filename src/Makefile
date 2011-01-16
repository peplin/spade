CC = gcc
CFLAGS = -Wall -Werror -I src -fmessage-length=80
LDFLAGS = -lpthread -lev -llog4c -lconfig

ifdef DEBUG
   CFLAGS += -ggdb
else
   CFLAGS += -O2
endif

all: dirt

dirt: dirt.c csapp.o http.o util.o server.o

clean:
	rm -f *.o dirt *~
