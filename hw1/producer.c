#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void main(int argc, char* argv[0])
{
    int error;
    union sigval value;
    pid_t aggregator_pid;
    int producer_id;

    // confirm number of arguments
    if(argc != 3)
    {
        printf("ERROR: Please specify aggregator process ID and producer ID. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    // grab aggregator PID
    aggregator_pid = atoi(argv[1]);
    if(aggregator_pid == 0)
    {
        printf("ERROR: Invalid aggregator PID specified Exiting...\n");
        exit(EXIT_FAILURE);
    }

    // grab producer ID
    producer_id = atoi(argv[2]); // TODO - should we confirm valid producer ID?
    
    printf("Aggregator PID = %u, Producer ID = %d!\n", aggregator_pid, producer_id);

    
    value.sival_int = getpid();

    // send PID to aggreagtor
    error = sigqueue(aggregator_pid, SIGRTMIN + producer_id, value);
    if(error)
    {
        printf("ERROR: Failed to send message to producer, %s, Exiting...\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // wait for confirmation
    //TODO

    return;
}