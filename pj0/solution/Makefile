CC=gcc
CFLAGS=-Wall -Wextra
TESTS:=test_in_eaccess test_out_eaccess test_in_eexist test_invalid_arg
TESTS+=test_default test_input_option test_output_option test_input_output_option
GANG=0

.PHONY: build check clean dist
.PHONY: $(TESTS)

build: lab0

check: build $(TESTS)
	@echo "Completed all tests."

clean:
	rm -rf lab0 *.o *.tar.gz

dist: build
	@tar -zcf lab0-005181694.tar.gz lab0.c Makefile README backtrace.png breakpoint.png

lab0: lab0.c
	$(CC) $(CFLAGS) lab0.c -o lab0

test_out_eaccess: build
	@ echo "Testing output file that does not have read access..."; \
	touch no_perm; \
	chmod 000 ./no_perm; \
	./lab0 --input lab0.c --output no_perm > /dev/null 2>&1; \
	if [[ $$? -ne 3 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 3 when unable to open output file"; \
	fi; \
	rm -f no_perm;

test_in_eaccess: build
	@ echo "Testing input file that does not have read access..."; \
	touch no_perm; \
	chmod 000 ./no_perm; \
	./lab0 --input no_perm > /dev/null 2>&1; \
	if [[ $$? -ne 2 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 2 when open with insufficient permission"; \
	fi; \
	rm -f no_perm;

test_in_eexist: build
	@ echo "Testing non-existant input file..."; \
	./lab0 --input not_exist > /dev/null 2>&1; \
	if [[ $$? -ne 2 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 2 when the input file doesn't exist"; \
	fi;

test_invalid_arg: build
	@ echo "Testing invalid argument..."; \
	./lab0 --wtf > /dev/null 2>&1; \
	if [[ $$? -ne 1 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 1 when there is an invalid argument"; \
	fi;

test_default: build
	@ echo "Testing default case..."; \
	result=`cat Makefile | ./lab0 | diff - Makefile`; \
	if [[ $$? -ne 0 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 0 when operation successful"; \
	fi; \
	if [[ ! -z "$$result" ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Mismatch between stdin and stdout without any arguments"; \
	fi;

test_input_option: build
	@ echo "Testing execution with input option..."; \
	./lab0 --input Makefile > /dev/null 2>&1; \
	if [[ $$? -ne 0 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 0 when the operation is successful"; \
	fi; \
	result=`./lab0 --input Makefile | diff - Makefile`; \
	if [[ ! -z "$$result" ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Mismatch between input file and stdout"; \
	fi;

test_output_option: build
	@ echo "Testing execution with output option..."; \
	cat Makefile | ./lab0 --output out > /dev/null 2>&1; \
	if [[ $$? -ne 0 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 0 when the operation is successful"; \
	fi; \
	result=`diff out Makefile`; \
	if [[ ! -z "$$result" ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Mismatch between stdin and output file"; \
	fi; \
	rm -f out;

test_input_output_option: build
	@ echo "Testing execution with both options..."; \
	./lab0 --input Makefile --output out > /dev/null 2>&1; \
	if [[ $$? -ne 0 ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Should exit with status 0 when the operation is successful"; \
	fi; \
	result=`diff out Makefile`; \
	if [[ ! -z "$$result" ]]; then \
		echo "  CHECK FAILED!!!"; \
		echo "  Mismatch between input file and output file"; \
	fi; \
	rm -f out;

