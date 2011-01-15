CC = gcc
DEBUG = -ggdb
CFLAGS = -O2 -Wall -Werror -I src -fmessage-length=80 $(DEBUG)
LDFLAGS = -lpthread -lev -llog4c

all: dirt

dirt: dirt.c csapp.o

clean:
	rm -f *.o dirt *~
