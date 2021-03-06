// Student ID: hall1945
// Name: Teague Hall
// Course: CSCI5103 Operating Systems
// Assignment #3 Part 2
// Tested On: csel-kh1260-01.cselabs.umn.edu
// Description: Main process that initializes shared memory and spawns producer/modifier/consumer processes. Also performs clean-up.

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
#include "shared.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>

int main(int argc, char* argv[])
{
    size_t buffer_size; // items
    size_t items_to_produce = 1000;
    
    // print error if invalid number of arguments specified
    if(argc < 2)
    {
        printf("ERROR: Invalid number of arguments specified:\n");
        printf(" Arg 1: Buffer size in items (required)\n");
        printf(" Arg 2: Produced items (optional, defaults to 1000)\n");
        exit(EXIT_FAILURE);
    }

    // read-in buffer size
    buffer_size = atoi(argv[1]);
    if(buffer_size <= 0)
    {
        throwError(0, "Invalid buffer size specified");
    }

    // read-in producer items(if specified)
    if(argc > 2)
    {
        items_to_produce = atoi(argv[2]);
        if(items_to_produce <= 0)
        {
            throwError(0, "Invalid number of producer items specified");
        }
    }

    // convert args back to strings (will be fed to execls)
    char str_buffer_size[128];
    char str_items_to_produce[128];
    sprintf(str_buffer_size, "%lu", buffer_size);
    sprintf(str_items_to_produce, "%lu", items_to_produce);

    // delete shared memory segment if one already exists for item buffer
    int shmid_buffer = shmget(SHARED_MEM_BUFFER, 1, 0);
    if(shmid_buffer != -1)
    {
        #ifdef DEBUG_PRINT
            printf("Founding existing shared memory segment for item buffer, deleting it...\n");
        #endif
        
        // delete
        if(shmctl(shmid_buffer, IPC_RMID, 0) == -1)
        {
            throwError(errno, "Failed to remove existing shared memory segment for item buffer");
        }
    }

    // allocate shared memory for item buffer
    shmid_buffer = shmget(SHARED_MEM_BUFFER, buffer_size * sizeof(SharedMemItem), IPC_CREAT | 0666);
    if(shmid_buffer == -1)
    {
        throwError(errno, "Failed to create shared memory for item buffer");
    }

    // delete shared memory segment if one already exists for semaphores
    int shmid_semaphores = shmget(SHARED_MEM_SEMAPHORES, 1, 0);
    if(shmid_semaphores != -1)
    {
        #ifdef DEBUG_PRINT
            printf("Founding existing shared memory segment for semaphores, deleting it...\n");
        #endif
        
        // delete
        if(shmctl(shmid_semaphores, IPC_RMID, 0) == -1)
        {
            throwError(errno, "Failed to remove existing shared memory segment for semaphores");
        }
    }

    // allocate shared memory for semaphores
    shmid_semaphores = shmget(SHARED_MEM_SEMAPHORES, sizeof(SharedMemSemaphores), IPC_CREAT | 0666);
    if(shmid_semaphores == -1)
    {
        throwError(errno, "Failed to create shared memory for semaphores");
    }

    // attach shared memory semaphores 
    SharedMemSemaphores* semaphores = shmat(shmid_semaphores, NULL, 0);
    if(semaphores == (void*)-1)
    {
        throwError(errno, "Failed to map shared memory semaphores");
    }

    // initialize semaphores
    sem_init(&semaphores->sem_buffer_avail, PTHREAD_PROCESS_SHARED, buffer_size);
    sem_init(&semaphores->sem_todo_modify, PTHREAD_PROCESS_SHARED, 0);
    sem_init(&semaphores->sem_todo_consume, PTHREAD_PROCESS_SHARED, 0);

    // detatch semaphore reference from shared memory
    shmdt(semaphores);

    // create producer
    pid_t pid_producer = fork();
    if(pid_producer == 0)
    {
        execl("producer.out", "producer.out", str_buffer_size, str_items_to_produce, NULL);
    }

    // create modifier
    pid_t pid_modifier = fork();
    if(pid_modifier == 0)
    {
        execl("modifier.out", "modifier.out", str_buffer_size, NULL);
    }

    // create consumer
    pid_t pid_consumer = fork();
    if(pid_consumer == 0)
    {
        execl("consumer.out", "consumer.out", str_buffer_size, NULL);
    }

    // wait for child processes
    waitpid(pid_producer, NULL, 0); //printf("Producer finished\n");
    waitpid(pid_modifier, NULL, 0); //printf("Modifier finished\n");
    waitpid(pid_consumer, NULL, 0); //printf("Consumer finished\n");

    // remove shared memory
    if(shmctl(shmid_buffer, IPC_RMID, 0) == -1)
    {
        throwError(errno, "Failed to remove item buffer shared memory");
    }

    if(shmctl(shmid_semaphores, IPC_RMID, 0) == -1)
    {
        throwError(errno, "Failed to remove semaphores shared memory");
    }

    printf("--------------------------------\n");
    printf("|           SUCCESS             \n");
    printf("--------------------------------\n");
    printf("Buffer Size: %lu\n", buffer_size);
    printf("Produced Items: %lu\n", items_to_produce);
    printf("Log files outputted locally\n");
    printf("--------------------------------\n");

    return 0;
}

