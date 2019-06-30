CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: all check clean dist

all: lab0

check:
	# correct case with no params.
	# return code 0.
	# correct case with specified input. return code 0.
	# correct case with specified output. return code 0.
	# correct case with specified intput and output. return code 0.

	# error case with unrecognized argument.
	# print out error message, usage line. return code 1.

	# error case with with inaccessible specified input. return code 2
	# error case with with non-existant specified output. return code 3

clean:
	rm -rf lab0 *.o

dist: lab0
	tar -zcvf lab0.tar.gz lab0

lab0: lab0.o
	$(CC) $(CFLAGS) lab0.o -o lab0

lab0.o: lab0.c
	$(CC) $(CFLAGS) -c lab0.c -o lab0.o
