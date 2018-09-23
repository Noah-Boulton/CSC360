.phony all:
all: PMan

PMan: PMan.c
	gcc -c linkedList.c -o linkedList.o
	gcc PMan.c -lreadline linkedList.o -o PMan

.PHONY clean:
clean:
	-rm -rf *.o *.exe
