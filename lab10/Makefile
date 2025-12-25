CC = gcc
CFLAGS = -pthread

all: rwl

rwl: rwl.c
	$(CC) $(CFLAGS) -o rwl rwl.c

clean:
	rm -f rwl

rebuild: clean all