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
    if(argc < 2)
    {
        throwError(0, "Consumer received incorrect number of args");
    }

    // parse args
    size_t buffer_size = atoi(argv[1]);
    if(buffer_size < 0)
    {
        throwError(0, "Consumer received invalid buffer size");
    }

    #ifdef DEBUG_PRINT
    printf("Consumer started! buffer_size = %lu\n", buffer_size);
    #endif
    
    //// open shared semaphores
    //sem_t* sem_buffer_avail = sem_open(SEMAPHORE_BUFFER_AVAIL, 0);
    //if(sem_buffer_avail == SEM_FAILED)
    //{
    //    throwError(errno, "Failed to open SEMAPHORE_BUFFER_AVAIL semaphore");
    //}
    //
    //sem_t* sem_todo_consume = sem_open(SEMAPHORE_TODO_CONSUME, 0);
    //if(sem_todo_consume == SEM_FAILED)
    //{
    //    throwError(errno, "Failed to open SEMAPHORE_TODO_CONSUME semaphore");
    //}

    // get shared-memory ID of item buffer and map to address space
    int shmid_buffer = shmget(SHARED_MEM_BUFFER, buffer_size * sizeof(SharedMemItem), IPC_CREAT);
    if(shmid_buffer == -1)
    {
        throwError(errno, "Consumer failed to get item buffer shared memory");
    }
    
    SharedMemItem* item_buffer = shmat(shmid_buffer, NULL, 0);
    if(item_buffer == (void*)-1)
    {
        throwError(errno, "Consumer failed to map shared memory item buffer to address space");
    }

    // get shared-memory ID of semaphores and map to address space
    int shmid_semaphores = shmget(SHARED_MEM_SEMAPHORES, sizeof(SharedMemSemaphores), IPC_CREAT | 0666);
    if(shmid_semaphores == -1)
    {
        throwError(errno, "Consumer failed to get semaphores shared memory");
    }

    SharedMemSemaphores* semaphores = shmat(shmid_semaphores, NULL, 0);
    if(semaphores == (void*)-1)
    {
        throwError(errno, "Consumer failed to map shared memory semaphores to address space");
    }

    FILE* log = fopen("consumer.log", "w");
    if(log == NULL)
    {
        throwError(0, "Failed to create consumer.log");
    }

    int next_index_consumer = 0;

    // run indefinitely until EOS received
    while(1)
    {
        // wait until items need consuming
        sem_wait(&semaphores->sem_todo_consume);

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
        if(next_index_consumer >= buffer_size)
        {
            next_index_consumer = 0;
        }

        // signal opening in buffer
        sem_post(&semaphores->sem_buffer_avail);
    }

    // detatch shared memory
    if(shmdt(item_buffer) == -1)
    {
        throwError(errno, "Consumer failed to detatch item buffer shared memory");
    }

    if(shmdt(semaphores) == -1)
    {
        throwError(errno, "Consumer failed to detatch semaphores shared memory");
    }

    // close log file
    fclose(log);

    return 0;
}