CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: build tests graphs dist clean
.PHONY: $(TESTS)

build: lab2_add lab2_list

tests: tests_add tests_list

tests_add: lab2_add
	./tests_add.sh > lab2_add.csv

tests_list: lab2_list
	-./tests_list.sh > lab2_list.csv

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

lab2_list: lab2_list.c SortedList.c
	$(CC) $(CFLAGS) -pthread lab2_list.c SortedList.c -o lab2_list

