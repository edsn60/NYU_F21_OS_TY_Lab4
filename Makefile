CC=gcc
CFLAGS=-g -std=gnu11 -Wall -Werror -Wextra -lcrypto

.PHONY: all
all: nyufile

nyufile: nyufile.o fsinfo.o file_recovery.o fat_manipulate.o
	$(CC) -o $@ $^ $(CFLAGS)

nyufile.o: nyufile.c fsinfo.h file_recovery.h

fsinfo.o: fsinfo.c fsinfo.h fat_manipulate.h

file_recovery.o: file_recovery.c fsinfo.h fat_manipulate.h

fat_manipulate.o: fat_manipulate.c fsinfo.h

.PHONY: clean
clean:
	rm -f *.o nyufile
