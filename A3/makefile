.phony all:
all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c
	gcc -o diskinfo diskinfo.c

disklist: disklist.c
	gcc -o disklist disklist.c

diskget: diskget.c
	gcc -o diskget diskget.c

diskput: diskput.c
	gcc -o diskput diskput.c

.PHONY clean:
clean:
	-rm -rf *.o *.exe
