.PHONY: all 
all: shell.out test.out


shell.out: parser.c shell.c
	gcc -o shell.out parser.c shell.c

test.out: parser.c test.c
	gcc -o test.out parser.c test.c


.PHONY: clean
clean:
	rm -f *.out