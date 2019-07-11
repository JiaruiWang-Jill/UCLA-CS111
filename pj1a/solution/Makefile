CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: build check clean dist

build: lab1a

clean:
	rm -rf lab1a *.o *.tar.gz

dist: build
	@tar -zcf lab1a-005181694.tar.gz lab1a.c Makefile README

lab1a: lab1a.c
	$(CC) $(CFLAGS) lab1a.c -o lab1a

