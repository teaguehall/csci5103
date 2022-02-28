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
    #ifdef PRINT_DEBUG
    printf("Producer started\n");
    #endif
    
    // get shared memory ID
    int shmid = shmget(SHARED_MEM_KEY, sizeof(SharedMemObject), IPC_CREAT);
    if(shmid == -1)
    {
        throwError(errno, "Failed to retrieve shared memory ID (try issuing 'ipcrm -M 0x1A2B3C4D' to remove shared memory)");
    }

    // map shared memory to address space
    SharedMemObject* pshared = shmat(shmid, NULL, 0);
    if(pshared == -1)
    {
        throwError(errno, "Failed to map shared memory to address space");
    }

    int debug = 0;
    while(1)
    {
        printf("Producing = %d\n", debug++);
        pshared->debug = debug;
        sleep(1);
    }

    return 0;
}