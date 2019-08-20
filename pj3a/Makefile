#NAME: Qingwei Zeng
#EMAIL: zenn@ucla.edu
#ID: 005181694

.PHONY: default dist clean sanity trivial

build: lab3a.c lab3a.h ext2_fs.h
	gcc -o lab3a -g -Wall -Wextra lab3a.c

dist: clean build
	tar -czvf lab3a-005181694.tar.gz lab3a.c lab3a.h README Makefile ext2_fs.h

clean:
	rm -f lab3a *.tar.gz

trivial: default
	./lab3a trivial.img | less

sanity: dist
	./P3A_check.sh 005181694
