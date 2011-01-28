CC = gcc
CFLAGS = -g -Wall -Werror -fPIC
LDFLAGS = -lpthread

all: adder.so

adder.so: adder.o
	$(CC) -shared -fPIC adder.o -o adder.so

clean:
	rm -f *~ *.o adder
