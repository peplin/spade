CC = gcc
CFLAGS = -Wall -std=c99 -Werror -I src -fmessage-length=80
LDFLAGS = -lpthread -lev -llog4c -lconfig -ldl

ifdef OPTIMIZED
   CFLAGS += -O2
else
   CFLAGS += -ggdb
endif

ifdef PROFILE
   LDFLAGS += -pg
   CFLAGS += -pg
endif

all: spade

spade: spade.o csapp.o http.o util.o server.o config.o cgi.o

clean:
	rm -f *.o spade *~
