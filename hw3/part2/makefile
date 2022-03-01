CC=gcc
CFLAGS=-I.

main: prodcons.c producer.c modifier.c consumer.c shared.h
	$(CC) -o prodcons.out prodcons.c -lpthread -D_REENTRANT
	$(CC) -o producer.out producer.c -lpthread -D_REENTRANT
	$(CC) -o modifier.out modifier.c -lpthread -D_REENTRANT
	$(CC) -o consumer.out consumer.c -lpthread -D_REENTRANT

clean:
	rm -f prodcons.out
	rm -f producer.out
	rm -f modifier.out
	rm -f consumer.out
