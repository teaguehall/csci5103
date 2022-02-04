// Student ID: hall1945
// Name: Teague Hall
// Course: CSCI5103 Operating Systems
// Assignment #1
// Tested On: csel-kh1260-01.cselabs.umn.edu
// Description: Producer process that gets created by aggregator process.
// Producer opens up corresponding file and wait for communications to
// begin from the aggregators. All numbers within file are sent to aggregator
// after which the file is closed.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// SIGUSR1 is mapped to this handler, but nothing to do...
void handlerDoNothing(int signal)
{
}

void main(int argc, char* argv[0])
{
    int error;
    sigset_t wait_mask, blocked_mask, old_mask;
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

    // block user 1 signal
    error = sigemptyset(&blocked_mask);
    if(error)
    {
        printf("ERROR: Failed to create empty signal set. %s, Exiting...\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    error = sigaddset(&blocked_mask, SIGUSR1);
    if(error)
    {
        printf("ERROR: Failed to add signal to set. %s, Exitting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    error = sigprocmask (SIG_BLOCK, &blocked_mask, &old_mask);
    if(error)
    {
        printf("Failed to set signal mask. %s, Exitting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // build user 1 wait mask
    error = sigfillset(&wait_mask);
    if(error)
    {
        printf("ERROR: Failed to set signal mask, %s, Exiting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    error = sigdelset(&wait_mask, SIGUSR1);
    if(error)
    {
        printf("ERROR: Failed to remove signal from set, %s, Exiting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // map user 1 signal to handler
    signal(SIGUSR1, handlerDoNothing);

    // send PID to aggregator
    value.sival_int = getpid();
    error = sigqueue(aggregator_pid, SIGRTMIN + producer_id, value);
    if(error)
    {
        printf("ERROR: Failed to send message to producer, %s, Exiting...\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // wait for signal response
    sigsuspend(&wait_mask);

    // open file
    char filename[64];
    sprintf(filename, "data%u.txt",producer_id + 1);
    FILE* file = fopen(filename, "r");
    if(file == NULL)
    {
        printf("ERROR: Failed to open file. %s, Exiting...", strerror(errno));
    }

    // iterate over file
    char fileline[64]; 
    int number;
    while(fgets(fileline, 64, file) != NULL)
    {
        // read number from file
        sscanf(fileline, "%d", &number);

        // send number
        value.sival_int = number;
        error = sigqueue(aggregator_pid, SIGRTMIN + producer_id, value);
        if(error)
        {
            printf("ERROR: Failed to send message to producer, %s, Exiting...\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // wait for signal response
        sigsuspend(&wait_mask);
    }

    // send negative value to indication completion
    value.sival_int = -1;
    error = sigqueue(aggregator_pid, SIGRTMIN + producer_id, value);
    if(error)
    {
        printf("ERROR: Failed to send message to producer, %s, Exiting...\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return;
}