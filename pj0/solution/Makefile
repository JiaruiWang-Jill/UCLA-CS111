CC=gcc
CFLAGS=-Wall -Wextra

.PHONY: build check clean dist

build: lab0

check: build test_in_eaccess test_out_eaccess

clean:
	rm -rf lab0 *.o

dist: build
	tar -zcvf lab0.tar.gz lab0

lab0: lab0.c
	$(CC) $(CFLAGS) lab0.c -o lab0

test_out_eaccess: build
	@touch no_perm; \
	chmod 000 ./no_perm; \
	./lab0 --input lab0.c --output no_perm > /dev/null 2>&1; \
	if [[ $$? -ne 3 ]]; then \
		echo "\nCHECK FAILED!!!"; \
		echo "Should exit with status 3 when unable to open output file"; \
	fi; \
	rm -f no_perm

test_in_eaccess: build
	@touch no_perm; \
	chmod 000 ./no_perm; \
	./lab0 --input no_perm > /dev/null 2>&1; \
	if [[ $$? -ne 3 ]]; then \
		echo "\nCHECK FAILED!!!"; \
		echo "Should exit with status 2 when unable to open output file"; \
	fi; \
	rm -f no_perm

test_in_eaccess: build
	@touch no_perm; \
	chmod 000 ./no_perm; \
	./lab0 --input no_perm > /dev/null 2>&1; \
	if [[ $$? -ne 3 ]]; then \
		echo "\nCHECK FAILED!!!"; \
		echo "Should exit with status 2 when unable to open output file"; \
	fi; \
	rm -f no_perm
