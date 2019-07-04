CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: all check clean dist

all: lab0

check: lab0
	./lab0 --input lab0.c --output no_perm 2> stderr; \
	if [[ $$? -ne 3 ]]; then \
		echo "FAILED: Should exit with status 3 when unable to open output file"; \
	fi

clean:
	rm -rf lab0 *.o

dist: lab0
	tar -zcvf lab0.tar.gz lab0

lab0: lab0.c
	$(CC) $(CFLAGS) lab0.c -o lab0
