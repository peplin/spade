CC = gcc
CFLAGS = -g -Wall -Werror -fPIC
LDFLAGS = -lpthread

all: adder.so

adder.so: adder.o csapp.o
	$(CC) -shared -fPIC adder.o csapp.o -o adder.so 

clean:
	rm -f *~ *.o adder  *.so
