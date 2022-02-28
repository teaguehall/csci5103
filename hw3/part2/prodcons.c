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

int main(int argc, char* argv[])
{
    pid_t pid_producer;
    pid_t pid_modifier;
    pid_t pid_consumer;

    size_t max_items;
    size_t produced_items = 1000;
    
    // print error if invalid number of arguments specified
    if(argc < 2)
    {
        printf("ERROR: Invalid number of arguments specified:\n");
        printf(" Arg 1: Buffer size in items (required)\n");
        printf(" Arg 2: Produced items (optional, defaults to 1000)\n");
        exit(EXIT_FAILURE);
    }

    // read-in buffer size
    max_items = atoi(argv[1]);
    if(max_items <= 0)
    {
        throwError(0, "Invalid buffer size specified");
    }

    // read-in producer items(if specified)
    if(argc > 2)
    {
        produced_items = atoi(argv[2]);
        if(produced_items <= 0)
        {
            throwError(0, "Invalid number of producer items specified");
        }
    }

    // allocate shared memory
    int shmid = shmget(SHARED_MEM_KEY, sizeof(SharedMemObject), IPC_CREAT | 0666);
    if(shmid == -1)
    {
        throwError(errno, "Failed to create shared memory object");
    }
    
    // map shared memory to address space
    SharedMemObject* pshared = shmat(shmid, NULL, 0);
    if(pshared == -1)
    {
        throwError(errno, "Failed to map shared memory to address space");
    }

    // create producer
    pid_producer = fork();
    if(pid_producer == 0)
    {
        execl("producer.out", "producer.out", NULL);
    }

    // create modifier
    pid_modifier = fork();
    if(pid_modifier == 0)
    {
        execl("modifier.out", "modifier.out", NULL);
    }

    // create consumer
    pid_consumer = fork();
    if(pid_consumer == 0)
    {
        execl("consumer.out", "consumer.out", NULL);
    }

    // wait for child processes
    waitpid(pid_producer, NULL, 0); printf("Producer finished\n");
    waitpid(pid_modifier, NULL, 0); printf("Modifier finished\n");
    waitpid(pid_consumer, NULL, 0); printf("Consumer finished\n");

    return 0;
}

