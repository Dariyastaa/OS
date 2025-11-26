CC      := gcc
PROGS   := mypipe myfiforead myfifowrite 

.PHONY: all clean rebuild

all: $(PROGS)

%.o: %.c
	$(CC) -c $< -o $@

mypipe: mypipe.o
	$(CC) $^ -o $@

myfiforead: myfiforead.o
	$(CC) $^ -o $@

myfifowrite: myfifowrite.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGS)

rebuild: clean all
