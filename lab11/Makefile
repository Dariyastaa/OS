CC = gcc
CFLAGS = -pthread

all: condvar

condvar: condvar.c
	$(CC) $(CFLAGS) -o condvar condvar.c

clean:
	rm -f condvar

rebuild: clean all