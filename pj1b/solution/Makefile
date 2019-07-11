CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: build check clean dist
.PHONY: $(TESTS)

build: lab1b

clean:
	rm -rf lab1b-client lab1b-server *.o *.tar.gz

dist: build
	@tar -zcf lab1b-005181694.tar.gz lab1b-client.c lab1b-server.c Makefile README

lab1b: lab1b-client lab1b-server

lab1b-client: lab1b-client.c
	$(CC) $(CFLAGS) lab1b-client.c -o lab1b-client

lab1b-server: lab1b-server.c
	$(CC) $(CFLAGS) lab1b-server.c -o lab1b-server

