CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: build tests graphs dist clean
.PHONY: $(TESTS)

build: lab2_add lab2_list

tests: lab2_add.csv lab2_list.csv

graphs: tests
	./lab2_add.gp
	./lab2_list.gp

dist: graphs
	tar -zcf lab2a-005181694.tar.gz Makefile *.h *.c *.sh *.csv *.png README

clean:
	rm -rf lab2_add lab2_list *.o *.tar.gz *.png *.csv

lab2_add.csv: lab2_add tests_add.sh
	./tests_add.sh > lab2_add.csv

lab2_list.csv: lab2_list tests_list.sh
	-./tests_list.sh > lab2_list.csv 2> /dev/null

tests_sort: SortedList.c sort_test.c
	$(CC) $(CFLAGS) sort_test.c SortedList.c -o sort_test && ./sort_test

lab2_add: lab2_add.c
	$(CC) $(CFLAGS) -pthread lab2_add.c -o lab2_add

lab2_list: lab2_list.c SortedList.c
	$(CC) $(CFLAGS) -pthread lab2_list.c SortedList.c -o lab2_list

