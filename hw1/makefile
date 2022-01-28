CC=gcc
CFLAGS=-I.

main: aggregator.c producer.c
	$(CC) -o aggregator.out aggregator.c
	$(CC) -o producer.out producer.c

clean:
	rm -f aggregator.out
	rm -f producer.out
