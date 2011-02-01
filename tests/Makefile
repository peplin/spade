CC = gcc
CFLAGS = -g -Wall -Werror
LDFLAGS = -lpthread

all:
	$(MAKE) -C dirt
	$(MAKE) -C cgi-bin

clean:
	rm -f *~ *.o
	$(MAKE) -C dirt clean
	$(MAKE) -C cgi-bin clean
