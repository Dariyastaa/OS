CC      := gcc
PROGS   := sem sender receiver

.PHONY: all clean rebuild

all: $(PROGS)

%.o: %.c
	$(CC) -c $< -o $@

sem: sem.o
	$(CC) $^ -o $@

sender: sender.o
	$(CC) $^ -o $@

receiver: receiver.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGS)

rebuild: clean all
