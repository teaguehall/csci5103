CC=gcc
CFLAGS=-I.

main: prodcons.c producer.c modifier.c consumer.c shared.h
	$(CC) -o prodcons.out prodcons.c
	$(CC) -o producer.out producer.c
	$(CC) -o modifier.out modifier.c
	$(CC) -o consumer.out consumer.c

clean:
	rm -f prodcons.out
	rm -f producer.out
	rm -f modifier.out
	rm -f consumer.out
