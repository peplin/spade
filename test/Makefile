CC = gcc
CFLAGS = -g -Wall -Werror
LDFLAGS = -lpthread

all: cgi

cgi: 
	$(MAKE) -C cgi-bin

clean:
	rm -f *~ *.o proxy core
	$(MAKE) -C cgi-bin
