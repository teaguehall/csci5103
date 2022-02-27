#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

// globals

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

// buffer items
typedef struct Item
{
    int id;
    char timestamp[128];
} Item;

Item* item_buffer;
size_t max_items;

// buffer indexes 
int next_index_producer = 0;
int next_index_modifier = 0;
int next_index_consumer = 0;

// semaphores
sem_t sem_buffer_avail;
sem_t sem_todo_modify;
sem_t sem_todo_consume;

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

    // set item buffer size
    max_items = atoi(argv[1]);
    if(max_items <= 0)
    {
        throwError(0, "Invalid buffer size specified");
    }

    // allocate memory for item buffer
    item_buffer = malloc(max_items * sizeof(Item));
    if(item_buffer == NULL)
    {
        throwError(0, "Failed to allocate memory for item buffer");
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

    // initialize semaphores
    sem_init(&sem_buffer_avail, 0, max_items);
    sem_init(&sem_todo_modify, 0, 0);
    sem_init(&sem_todo_consume, 0, 0);

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
    Item* pitem;
    struct timeval timestamp;
    
    // create log file
    FILE* log = fopen("producer.log", "w");
    if(log == NULL)
    {
        throwError(0, "Failed to create producer.log");
    }

    // send produced items to buffer 
    for(int i = 0; i < produced_items; i++)
    {
        sleep(1);
        
        // wait until available room in buffer for produced item
        sem_wait(&sem_buffer_avail);

        // grab time of day
        if(gettimeofday(&timestamp, NULL) == -1)
        {
            throwError(errno, "Failed to retrieve timestamp");
        }

        // grab reference to next available buffer item
        pitem = item_buffer + next_index_producer;

        // write data to item
        pitem->id = i;
        sprintf(pitem->timestamp, "%ld-%ld", timestamp.tv_sec, timestamp.tv_usec);

        // signal produced item to modifier
        sem_post(&sem_todo_modify);

        // increment buffer index for next produced items
        next_index_producer++;
        if(next_index_producer >= max_items)
        {
            next_index_producer = 0;
        }

        // log to file
        fprintf(log, "%d %s\n", pitem->id, pitem->timestamp);
        fflush(log);
    }

    // send EOS signal
    pitem = item_buffer + next_index_producer;
    sprintf(pitem->timestamp, "EOS");
    sem_post(&sem_todo_modify);
}

void* routineModifier(void* vargp)
{
    Item* pitem;
    struct timeval timestamp;
    char timestamp_string[128];

    // run indefinitely until EOS received
    while(1)
    {
        // wait until items need modifying
        sem_wait(&sem_todo_modify);

        printf("Modifier received value!\n");

        // set item pointer to next modifier item
        pitem = item_buffer + next_index_modifier;

        // increment modifier item index (for next iteration)
        next_index_modifier++;
        if(next_index_modifier >= max_items)
        {
            next_index_modifier = 0;
        }

        // break out if EOS received
        if(strcmp(pitem->timestamp, "EOS") == 0)
        {
            printf("Modifier received EOS\n");
            sem_post(&sem_todo_consume);
            break;
        }

        // grab time of day
        if(gettimeofday(&timestamp, NULL) == -1)
        {
            throwError(errno, "Failed to retrieve timestamp");
        }
        sprintf(timestamp_string, " %ld-%ld", timestamp.tv_sec, timestamp.tv_usec);

        // append new timestamp to existing
        strcat(pitem->timestamp, timestamp_string);
        
        // signal to consumer
        sem_post(&sem_todo_consume);
    }
}

void* routineConsumer(void* vargp)
{
    Item* pitem;

    // create log file
    FILE* log = fopen("consumer.log", "w");
    if(log == NULL)
    {
        throwError(0, "Failed to create consumer.log");
    }

    // run indefinitely until EOS received
    while(1)
    {
        // wait until items need consuming
        sem_wait(&sem_todo_consume);

        // TODO remove this eventually
        printf("Consumer received item!\n");

        // set item pointer to next consumer item
        pitem = item_buffer + next_index_consumer;

        // increment modifier item index (for next iteration)
        next_index_consumer++;
        if(next_index_consumer >= max_items)
        {
            next_index_consumer = 0;
        }

        // break out if EOS received
        if(strcmp(pitem->timestamp, "EOS") == 0)
        {
            printf("Consumer received EOS\n");
            break;
        }

        // log to file
        fprintf(log, "%d %s\n", pitem->id, pitem->timestamp);
        fflush(log);

        // signal opening in buffer
        sem_post(&sem_buffer_avail);
    }
}