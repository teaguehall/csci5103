#ifndef SHARED_H
#define SHARED_H

#define PRINT_DEBUG

#include <stddef.h>
#include <semaphore.h>

#define SEMAPHORE_BUFFER_AVAIL  "/sem_buffer_avail"
#define SEMAPHORE_TODO_MODIFY   "/sem_todo_mdofiy"
#define SEMAPHORE_TODO_CONSUME  "/sem_todo_consume"
#define SHARED_MEM_BUFFER       0x4D3C2B1A

typedef struct SharedMemItem
{
    int id;
    char timestamp[128];
} SharedMemItem;

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