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

    // create shared semaphores
    sem_t* sem_buffer_avail = sem_open(SEMAPHORE_BUFFER_AVAIL, O_CREAT, 0666, buffer_size);
    if(sem_buffer_avail == SEM_FAILED)
    {
        throwError(errno, "Failed to create SEMAPHORE_BUFFER_AVAIL semaphore");
    }

    sem_t* sem_todo_modify = sem_open(SEMAPHORE_TODO_MODIFY, O_CREAT, 0666, 0);
    if(sem_todo_modify == SEM_FAILED)
    {
        throwError(errno, "Failed to create SEMAPHORE_TODO_MODIFY semaphore");
    }

    sem_t* sem_todo_consume = sem_open(SEMAPHORE_TODO_CONSUME, O_CREAT, 0666, 0);
    if(sem_todo_consume == SEM_FAILED)
    {
        throwError(errno, "Failed to create SEMAPHORE_TODO_CONSUME semaphore");
    }

    // allocate and map shared memory buffer
    int shmid = shmget(SHARED_MEM_BUFFER, buffer_size * sizeof(SharedMemItem), IPC_CREAT | 0666);
    if(shmid == -1)
    {
        throwError(errno, "Failed to create shared memory object");
    }
    
    SharedMemItem* pbuffer = shmat(shmid, NULL, 0);
    if(pbuffer == -1)
    {
        throwError(errno, "Failed to map shared memory to address space");
    }

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
    waitpid(pid_producer, NULL, 0); printf("Producer finished\n");
    waitpid(pid_modifier, NULL, 0); printf("Modifier finished\n");
    waitpid(pid_consumer, NULL, 0); printf("Consumer finished\n");

    return 0;
}

