CC = gcc
CFLAGS = -pthread

all: mymutex

mymutex: mymutex.c
	$(CC) $(CFLAGS) -o mymutex mymutex.c

clean:
	rm -f mymutex

rebuild: clean all