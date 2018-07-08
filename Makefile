CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread -ldl

all: bestServer

bestServer: bestServer.c csapp.o sbuf.o
	$(CC) $(CFLAGS) -o bestServer bestServer.c csapp.o sbuf.o $(LIB)

csapp.o:
	$(CC) $(CFLAGS) -c csapp.c

sbuf.o: csapp.o
	$(CC) $(CFLAGS) -c sbuf.c csapp.o


clean:
	rm -f *.o baselineServer *~

