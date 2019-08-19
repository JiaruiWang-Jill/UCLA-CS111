CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: default check clean dist local

default: lab4b.c
	$(CC) -g -lm -lmraa $(CFLAGS) lab4b.c -o lab4b

check:
	{ echo "START"; sleep 2; echo "STOP"; sleep 2; echo "OFF"; } | ./lab4b --log=log.txt
	if [ $? -ne 0 ]; then
		echo "ERROR: program did not successfully exit"
	else
		echo "OK"
	fi
	for c in START STOP OFF SHUTDOWN; do
		grep "$c" log.txt > /dev/null
		if [ $? -ne 0 ]; then
			echo "Failed to log '$c' command"
		else
			echo "'$c' OK"
		fi
	done
	rm -f log.txt

clean:
	rm -f lab4b lab4b-005181694.tar.gz

dist:
	tar -czf lab4b-005181694.tar.gz lab4b.c Makefile README

