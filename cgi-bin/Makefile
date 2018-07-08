CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder subtract multiply

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

subtract: subtract.c
	$(CC) $(CFLAGS) -o subtract subtract.c

multiply: multiply.c
	$(CC) $(CFLAGS) -o multiply multiply.c

clean:
	rm -f adder subtract multiply *~
