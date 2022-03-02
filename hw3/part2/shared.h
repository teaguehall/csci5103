#ifndef SHARED_H
#define SHARED_H

#define DEBUG_PRINT

#include <stddef.h>
#include <semaphore.h>

#define SHARED_MEM_BUFFER       0xBEEF
#define SHARED_MEM_SEMAPHORES   0xFEED

typedef struct SharedMemItem
{
    int id;
    char timestamp[128];
} SharedMemItem;

typedef struct SharedMemSemaphores
{
    sem_t sem_buffer_avail;
    sem_t sem_todo_modify;
    sem_t sem_todo_consume;
} SharedMemSemaphores;

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