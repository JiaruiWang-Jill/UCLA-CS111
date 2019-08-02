CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: build tests graphs dist clean
.PHONY: $(TESTS)

build: lab2_add

tests: build
	./tests_add.sh > lab2_add.csv

graphs: tests

dist:
	tar -zcf lab2a-005181694.tar.gz \
		lab2_add.c SortedList.h SortedList.c lab2_list.c \
		Makefile lab2_add.csv lab2_list.csv *.png \
		README

clean:
	rm -rf lab2_add lab2_list *.o *.tar.gz

lab2_add: lab2_add.c
	$(CC) $(CFLAGS) -pthread lab2_add.c -o lab2_add

lab2_list: lab2_list.c SortedList.o
	$(CC) $(CFLAGS) -pthread lab2_list.c SortedList.o -o lab2_list

SortedList.o:
	$(CC) $(CFLAGS) -c SortedList.c -o SortedList.o

