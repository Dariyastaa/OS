CC      := gcc
PROGS   := sem sender receiver
LDFLAGS := -pthread

.PHONY: all clean rebuild

all: $(PROGS)

%.o: %.c
	$(CC) -c $< -o $@

sem: sem.o
	$(CC) $^ -o $@ $(LDFLAGS)

sender: sender.o
	$(CC) $^ -o $@

receiver: receiver.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGS)

rebuild: clean all
