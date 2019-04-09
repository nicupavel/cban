#Defs
CC=gcc
BIN=../bin
#CFLAGS= -Wstrict-prototypes -O2 -fomit-frame-pointer -pipe -march=i686 -fno-strict-aliasing -fno-strength-reduce
#CFLAGS= -O4 -D NDEBUG -fomit-frame-pointer -pipe -march=i686 -fno-strict-aliasing -fno-strength-reduce
CFLAGS= -O4 -D NDEBUG
DEBUG= -g1

all: cban
install:
	mv cban $(BIN) || echo "no cban binary found"
cban: cban.o
	$(CC) $(CFLAGS)  cban.o -o cban
cban.o: cban.c
	$(CC) $(CFLAGS) -c cban.c
clean:
	/bin/rm -rfv *.o cban


