// Student ID: hall1945
// Name: Teague Hall
// Course: CSCI5103 Operating Systems
// Assignment #3 Part 2
// Tested On: csel-kh1260-01.cselabs.umn.edu
// Description: Modifier process

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
        throwError(0, "Modifier received incorrect number of args");
    }

    // parse args
    size_t buffer_size = atoi(argv[1]);
    if(buffer_size < 0)
    {
        throwError(0, "Modifier received invalid buffer size");
    }

    #ifdef DEBUG_PRINT
    printf("Modifier started! buffer_size = %lu\n", buffer_size);
    #endif

    // get shared-memory ID of item buffer and map to address space
    int shmid_buffer = shmget(SHARED_MEM_BUFFER, buffer_size * sizeof(SharedMemItem), IPC_CREAT);
    if(shmid_buffer == -1)
    {
        throwError(errno, "Modifier failed to get item buffer shared memory");
    }
    
    SharedMemItem* item_buffer = shmat(shmid_buffer, NULL, 0);
    if(item_buffer == (void*)-1)
    {
        throwError(errno, "Modifier failed to map shared memory item buffer to address space");
    }

    // get shared-memory ID of semaphores and map to address space
    int shmid_semaphores = shmget(SHARED_MEM_SEMAPHORES, sizeof(SharedMemSemaphores), IPC_CREAT | 0666);
    if(shmid_semaphores == -1)
    {
        throwError(errno, "Modifier failed to get semaphores shared memory");
    }

    SharedMemSemaphores* semaphores = shmat(shmid_semaphores, NULL, 0);
    if(semaphores == (void*)-1)
    {
        throwError(errno, "Modifier failed to map shared memory semaphores to address space");
    }

    struct timeval timestamp;
    char timestamp_string[128];
    int next_index_modifier = 0;

    // run indefinitely until EOS received
    while(1)
    {
        // wait until items need modifying
        sem_wait(&semaphores->sem_todo_modify);

        // break out if EOS received
        if(strcmp(item_buffer[next_index_modifier].timestamp, "EOS") == 0)
        {
            #ifdef DEBUG_PRINT 
                printf("Modifier received EOS\n");
            #endif

            sem_post(&semaphores->sem_todo_consume);
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
        if(next_index_modifier >= buffer_size)
        {
            next_index_modifier = 0;
        }

        // signal to consumer
        sem_post(&semaphores->sem_todo_consume);
    }

    // detatch shared memory
    if(shmdt(item_buffer) == -1)
    {
        throwError(errno, "Modifier failed to detatch item buffer shared memory");
    }

    if(shmdt(semaphores) == -1)
    {
        throwError(errno, "Modifier failed to detatch semaphores shared memory");
    }

    return 0;
}