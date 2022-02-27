#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <pthread.h>

// globals
int buffer_size;
int produced_items = 1000;

// prototypes
void throwError(int error, char* message);

// thread stuff
void* routineProducer(void* vargp);
void* routineModifier(void* vargp);
void* routineConsumer(void* vargp);

pthread_t tid_producer;
pthread_t tid_modifier;
pthread_t tid_consumer;

int main(int argc, char* argv[])
{
    // print error if invalid number of arguments specified
    if(argc < 2)
    {
        printf("ERROR: Invalid number of arguments specified:\n");
        printf(" Arg 1: Buffer size in items (required)\n");
        printf(" Arg 2: Produced items (optional, defaults to 1000)\n");
        exit(EXIT_FAILURE);
    }

    // set buffer size
    buffer_size = atoi(argv[1]);
    if(buffer_size <= 0)
    {
        throwError(0, "Invalid buffer size specified");
    }

    // set producer size if specifed
    if(argc > 2)
    {
        produced_items = atoi(argv[2]);
        if(produced_items <= 0)
        {
            throwError(0, "Invalid number of producer items specified");
        }
    }

    // spawn threads
    pthread_create(&tid_producer, NULL, routineProducer, NULL);
    pthread_create(&tid_modifier, NULL, routineModifier, NULL);
    pthread_create(&tid_consumer, NULL, routineConsumer, NULL);

    // wait for threads to finish
    pthread_join(tid_producer, NULL); printf("Producer finished!\n");
    pthread_join(tid_modifier, NULL); printf("Modifier finished!\n");
    pthread_join(tid_consumer, NULL); printf("Consumer finished!\n");

    return 0;
}

void throwError(int errnum, char* message)
{
    if(errnum)
    {
        printf("ERROR: %s: %s, exiting...\n", message, strerror(errnum));
    }
    else
    {
        printf("ERROR: %s, exiting...\n", message);
    }

    exit(EXIT_FAILURE);
}

void* routineProducer(void* vargp)
{
    int item;
    for(item = 0; item < produced_items; item++)
    {
        printf("Produced item %d\n", item);
        sleep(1);
    }
    
}

void* routineModifier(void* vargp)
{
    printf("Hello from modifier!\n");
}

void* routineConsumer(void* vargp)
{
    printf("Hello from consumer!\n");
}