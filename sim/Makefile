CFLAGS=-O3

all: sim

sim: main.o ihexread.o cpu8048.o
	gcc $(CFLAGS) -o sim main.o ihexread.o cpu8048.o

main.o: main.c Makefile ihexread.h cpu8048.h
	gcc $(CFLAGS) -c main.c

ihexread.o: ihexread.c ihexread.h Makefile
	gcc $(CFLAGS) -c ihexread.c

cpu8048.o: cpu8048.c cpu8048.h Makefile
	gcc $(CFLAGS) -c cpu8048.c


clean:
	rm *.o sim daatest

