CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra -lcrypto

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o execution.o encoder.o thread_pool.o task_manager.o
	$(CC) -o $@ $^ $(CFLAGS)

nyuenc.o: nyuenc.c thread_pool.h execution.h

execution.o: execution.c thread_pool.h execution.h encoder.h task_manager.h

task_manager.o: task_manager.c task_manager.h

threadpool.o: thread_pool.c thread_pool.h task_manager.h execution.h

encoder.o: encoder.c

.PHONY: clean
clean:
	rm -f *.o nyuenc
