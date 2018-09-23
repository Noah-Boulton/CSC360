.phony all:
all: ACS

ACS: ACS.c
	gcc -c queue.c -o queue.o
	gcc ACS.c -lpthread queue.o -o ACS

.PHONY clean:
clean:
	-rm -rf *.o *.exe

