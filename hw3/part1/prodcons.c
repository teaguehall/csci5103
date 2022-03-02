// Student ID: hall1945
// Name: Teague Hall
// Course: CSCI5103 Operating Systems
// Assignment #3 Part 1
// Tested On: csel-kh1260-01.cselabs.umn.edu
// Description: Multithreaded (single process) solution for producer/consumer/modifier problem

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

//#define DEBUG_PRINT

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

// buffer
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
    pthread_join(tid_producer, NULL); // printf("Producer thread finished\n");
    pthread_join(tid_modifier, NULL); // printf("Modifier thread finished\n");
    pthread_join(tid_consumer, NULL); // printf("Consumer thread finished\n");

    printf("--------------------------------\n");
    printf("|           SUCCESS             \n");
    printf("--------------------------------\n");
    printf("Buffer Size: %lu\n", max_items);
    printf("Produced Items: %u\n", produced_items);
    printf("Log files outputted locally\n");
    printf("--------------------------------\n");

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
        //sleep(1);
        
        // wait until available room in buffer for produced item
        sem_wait(&sem_buffer_avail);

        // grab time of day
        if(gettimeofday(&timestamp, NULL) == -1)
        {
            throwError(errno, "Failed to retrieve timestamp");
        }

        // write data to item
        item_buffer[next_index_producer].id = i + 1; // add 1 so ID starts 1
        sprintf(item_buffer[next_index_producer].timestamp, "%ld-%ld", timestamp.tv_sec, timestamp.tv_usec);

        // log to file
        fprintf(log, "%d %s\n", item_buffer[next_index_producer].id, item_buffer[next_index_producer].timestamp);
        fflush(log);

        // increment buffer index for next produced items
        next_index_producer++;
        if(next_index_producer >= max_items)
        {
            next_index_producer = 0;
        }

        // signal produced item to modifier
        sem_post(&sem_todo_modify);
    }

    // send EOS signal
    sem_wait(&sem_buffer_avail);
    sprintf(item_buffer[next_index_producer].timestamp, "EOS");
    sem_post(&sem_todo_modify);

    #ifdef DEBUG_PRINT
        printf("Producer sent EOS");
    #endif

    // close log file
    fclose(log);

    pthread_exit(NULL);
}

void* routineModifier(void* vargp)
{
    struct timeval timestamp;
    char timestamp_string[128];

    // run indefinitely until EOS received
    while(1)
    {
        // wait until items need modifying
        sem_wait(&sem_todo_modify);

        // break out if EOS received
        if(strcmp(item_buffer[next_index_modifier].timestamp, "EOS") == 0)
        {
            #ifdef DEBUG_PRINT 
                printf("Modifier received EOS\n");
            #endif

            sem_post(&sem_todo_consume);
            break;
        }

        #ifdef DEBUG_PRINT
            printf("Modifier received value = %u\n", item_buffer[next_index_modifier].id);
        #endif

        // grab time of day
        if(gettimeofday(&timestamp, NULL) == -1)
        {
            throwError(errno, "Failed to retrieve timestamp");
        }
        sprintf(timestamp_string, " %ld-%ld", timestamp.tv_sec, timestamp.tv_usec);

        // append new timestamp to existing
        strcat(item_buffer[next_index_modifier].timestamp, timestamp_string);

        // increment modifier item index (for next iteration)
        next_index_modifier++;
        if(next_index_modifier >= max_items)
        {
            next_index_modifier = 0;
        }

        // signal to consumer
        sem_post(&sem_todo_consume);
    }

    pthread_exit(NULL);
}

void* routineConsumer(void* vargp)
{
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

        // break out if EOS received
        if(strcmp(item_buffer[next_index_consumer].timestamp, "EOS") == 0)
        {
            #ifdef DEBUG_PRINT 
                printf("Consumer received EOS\n");
            #endif

            break;
        }

        #ifdef DEBUG_PRINT 
            printf("Consumer received value = %u\n", item_buffer[next_index_consumer].id);
        #endif

        // log to file
        fprintf(log, "%d %s\n", item_buffer[next_index_consumer].id, item_buffer[next_index_consumer].timestamp);
        fflush(log);

        // increment modifier item index (for next iteration)
        next_index_consumer++;
        if(next_index_consumer >= max_items)
        {
            next_index_consumer = 0;
        }

        // signal opening in buffer
        sem_post(&sem_buffer_avail);
    }

    // close log file
    fclose(log);

    pthread_exit(NULL);
}