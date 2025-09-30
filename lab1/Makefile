CC      := gcc
PROGS   := mycat mygrep

.PHONY: all clean rebuild

all: $(PROGS)

%.o: %.c
	$(CC) -c $< -o $@

mycat: mycat.o
	$(CC) $^ -o $@

mygrep: mygrep.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGS)

rebuild: clean all
