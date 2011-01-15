CC = gcc
CFLAGS = -O2 -Wall -Werror -I src -fmessage-length=80
LDFLAGS = -lpthread -lev
DEBUG = -ggdb

all: dirt

dirt: dirt.c csapp.o

clean:
	rm -f *.o dirt *~
