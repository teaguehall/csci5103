#ifndef SHARED_H
#define SHARED_H

#define PRINT_DEBUG

#include <stddef.h>
#include <semaphore.h>

#define SHARED_MEM_KEY 0xABCD

typedef struct SharedMemObject
{
    int debug;
    size_t buffer_size;
    size_t produced_items;
    sem_t sem_buffer_avail;
    sem_t sem_todo_modify;
    sem_t sem_todo_consume;

} SharedMemObject;

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

#endif // SHARED_H