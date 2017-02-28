a.out: main.o prog1.o slp.o util.o
	gcc -std=c99 -g main.o prog1.o slp.o util.o

main.o: main.c slp.h util.h prog1.h
	gcc  -std=c99 -g -c main.c

prog1.o: prog1.c slp.h util.h
	gcc -std=c99 -g -c prog1.c

slp.o: slp.c slp.h util.h
	gcc -std=c99 -g -c slp.c

util.o: util.c util.h
	gcc -std=c99 -g -c util.c 

clean: 
	rm -f a.out util.o prog1.o slp.o main.o
