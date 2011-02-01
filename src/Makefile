CC = gcc
CFLAGS = -Wall -std=c99 -Werror -I src -fmessage-length=80
LDFLAGS = -lpthread -llog4c -lconfig -ldl -lzmq

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

spade: spade.o csapp.o http.o util.o server.o config.o cgi.o dirt.o clay.o

clean:
	rm -f *.o spade *~
