#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/time.h>
#include "shared.h"

int main(int argc, char* argv[])
{
    // confirm arguments present
    if(argc < 3)
    {
        throwError(0, "Producer received incorrect number of args");
    }

    // parse args
    size_t buffer_size = atoi(argv[1]);
    if(buffer_size < 0)
    {
        throwError(0, "Producer received invalid buffer size");
    }

    size_t items_to_produce = atoi(argv[2]);
    if(items_to_produce < 0)
    {
        throwError(0, "Producer received invalid number of items to produce");
    }

    #ifdef DEBUG_PRINT
    printf("Producer started! buffer_size = %lu, items_to_produce = %lu\n", buffer_size, items_to_produce);
    #endif

    // open shared semaphores
    sem_t* sem_buffer_avail = sem_open(SEMAPHORE_BUFFER_AVAIL, 0);
    if(sem_buffer_avail == SEM_FAILED)
    {
        throwError(errno, "Failed to open SEMAPHORE_BUFFER_AVAIL semaphore");
    }

    sem_t* sem_todo_modify = sem_open(SEMAPHORE_TODO_MODIFY, 0);
    if(sem_todo_modify == SEM_FAILED)
    {
        throwError(errno, "Failed to open SEMAPHORE_TODO_MODIFY semaphore");
    }

    // allocate and map shared memory buffer
    int shmid = shmget(SHARED_MEM_BUFFER, buffer_size * sizeof(SharedMemItem), IPC_CREAT);
    if(shmid == -1)
    {
        throwError(errno, "Producer failed to open shared memory");
    }
    
    SharedMemItem* item_buffer = shmat(shmid, NULL, 0);
    if(item_buffer == (void*)-1)
    {
        throwError(errno, "Producer failed to map shared memory to address space");
    }

    struct timeval timestamp;
    int next_index_producer = 0;
    
    // create log file
    FILE* log = fopen("producer.log", "w");
    if(log == NULL)
    {
        throwError(0, "Failed to create producer.log");
    }

    // send produced items to buffer 
    for(int i = 0; i < items_to_produce; i++)
    {
        //sleep(1);
        
        // wait until available room in buffer for produced item
        sem_wait(sem_buffer_avail);

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

        #ifdef DEBUG_PRINT
            printf("Producer sent value = %u\n", item_buffer[next_index_producer].id);
        #endif

        // increment buffer index for next produced items
        next_index_producer++;
        if(next_index_producer >= buffer_size)
        {
            next_index_producer = 0;
        }

        // signal produced item to modifier
        sem_post(sem_todo_modify);
    }

    // send EOS signal
    sem_wait(sem_buffer_avail);
    sprintf(item_buffer[next_index_producer].timestamp, "EOS");
    sem_post(sem_todo_modify);

    #ifdef DEBUG_PRINT
        printf("Producer sent EOS\n");
    #endif

    // detatch shared memory
    if(shmdt(item_buffer) == -1)
    {
        throwError(errno, "Producer failed to detatch shared memory");
    }

    // open shared semaphores
    sem_close(sem_buffer_avail);
    sem_close(sem_todo_modify);

    // close log file
    fclose(log);

    return 0;
}