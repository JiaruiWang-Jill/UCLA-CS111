# NAME: Qingwei Zeng
# EMAIL: zenn@ucla.edu
# ID: 005181694

.PHONY: build clean dist

build:
	cp lab3b.py lab3b
	chmod +x lab3b

clean:
	rm -f lab3b-005181694.tar.gz lab3b

dist: build
	tar -czvf lab3b-005181694.tar.gz README Makefile lab3b.py lab3b_lib.py

sanity: clean dist
	./P3B_check.sh 005181694
