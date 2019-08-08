CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: build tests profile graphs dist clean

build: lab2_list.c SortedList.c
	$(CC) $(CFLAGS) -g -pthread lab2_list.c SortedList.c -o lab2_list

tests: lab2_list
	-./tests_list.sh > lab2_list.csv

profile: tests

graphs: tests

dist: build tests graphs
	tar -zcf lab2b-005181694.tar.gz \
		Makefile SortedList.h SortedList.c lab2_list.c lab2b_list.csv profile.out \
		lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png README \
		tests_list.sh

clean:
	rm -rf lab2_list *.o *.tar.gz *.png *.csv profile.out

