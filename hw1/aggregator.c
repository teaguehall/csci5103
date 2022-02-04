// Student ID: hall1945
// Name: Teague Hall
// Course: CSCI5103 Operating Systems
// Assignment #1
// Description: Aggregator process that spawns N number of producer processes per
// inputted arguments. Coordinates communication rounds between all producer 
// processes and calculates statistics on received numbers.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

// prototypes
void signalHandler(int signal, siginfo_t* pinfo, void* pcontext);
void signalReceive(int signal);
void signalRespond(int pid);
void logger(int signal, int value);

// globals shared between main & signal functions
double stat_avg = 0;
int stat_max = INT_MIN;
int stat_min = INT_MAX;
int stat_total_received = 0;
int stat_producer_received[5] = {0, 0, 0, 0, 0};
pid_t producer_pids[5];
int producer_active[5] = {0, 0, 0, 0, 0};
int producers_remaining = 0;

void main(int argc, char* argv[])
{   
    int error;
    int producer, max_producers;
    pid_t aggregator_pid;
    sigset_t blocked_mask, old_mask;
    struct sigaction new_action, old_action;
    char str_aggregator_pid[1024];
    char str_producer_id[1024];
    char* producer_args[]= { "./producer.out", str_aggregator_pid, str_producer_id, NULL};

    // confirm correct numbers of args
    if(argc != 2)
    {
        printf("ERROR: Please specify number of producers. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    // grab number of producers
    max_producers = atoi(argv[1]);
    if(max_producers <= 0 || max_producers > 5)
    {
        printf("ERROR: Invalid number of producers specified. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    // create empty signal set
    error = sigemptyset(&blocked_mask);
    if(error)
    {
        printf("ERROR: Failed to create empty signal set. %s, Exiting...\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // configure signal handlers & block
    for(producer = 0; producer < max_producers; producer++)
    {
        // add signal to blocked signal mask
        error = sigaddset(&blocked_mask, SIGRTMIN + producer);
        if(error)
        {
            printf("ERROR: Failed to add signal to set. %s, Exitting...", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // map signal to handler function
        new_action.sa_flags = SA_SIGINFO;
        new_action.sa_sigaction = &signalHandler;
        error =  sigaction(SIGRTMIN + producer, &new_action, &old_action);
        if(error)
        {
            printf("ERROR: Failed to map signal handler to producer %u. %s. Exiting\n", producer, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // block signal mask
    error = sigprocmask (SIG_BLOCK, &blocked_mask, &old_mask);
    if(error)
    {
        printf("Failed to set signal mask. %s, Exitting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // write aggregator pid to arg vector
    sprintf(str_aggregator_pid, "%u", getpid());

    // create producers
    for(producer = 0; producer < max_producers; producer++)
    {
        // fork and exec producer
        if(fork() == 0)
        {
            // write current producer ID to arg vector
            sprintf(str_producer_id, "%u", producer);
            execvp(producer_args[0], producer_args);
        }
    }

    // wait for all signals to intialize
    for(producer = 0; producer < max_producers; producer++)
    {
        signalReceive(SIGRTMIN + producer);
        signalRespond(producer_pids[producer]);
    }

    // perform communication rounds
    int round = 0;
    while(producers_remaining > 0)
    {
        // receive value from each producer
        for(producer = 0; producer < max_producers; producer++)
        {
            // only wait for producer if it's still active
            if(producer_active[producer])
            {
                signalReceive(SIGRTMIN + producer);
                signalRespond(producer_pids[producer]);
            }
        }
    }

    // print stats:
    printf("--------------------------------\n");
    printf("AGGREGATOR FINISHED - STATS\n");
    printf("--------------------------------\n");
    printf("Max: %d\n", stat_max);
    printf("Min: %d\n", stat_min);
    printf("Average: %f\n", stat_avg);
    for(producer = 0; producer < max_producers; producer++)
    {
        printf("Signals Received - Producer %d: %d\n", producer + 1, stat_producer_received[producer]);
    }
    printf("Signals Received - Total: %d\n", stat_total_received);
    printf("--------------------------------\n");
}

// handler for RT signals
void signalHandler(int signal, siginfo_t* pinfo, void* pcontext)
{
    int producer = signal - SIGRTMIN;

    // log signal
    logger(signal, pinfo->si_value.sival_int);

    // initialize if first signal received
    if(!producer_active[producer])
    {
        producer_pids[producer] = pinfo->si_value.sival_int;
        producer_active[producer] = 1;
        producers_remaining++;
    }
    else
    {
        // check for termination signal
        if(pinfo->si_value.sival_int == -1)
        {
           producer_active[producer] = 0;
           producers_remaining--;
        }
        else
        {
            // stats: increment total signal count
            stat_total_received++;

            // stats: increment producer specific count
            stat_producer_received[producer]++;
            
            // stats: check max
            if(pinfo->si_value.sival_int > stat_max)
            {
                stat_max = pinfo->si_value.sival_int;
            }

            // stats: check min
            if(pinfo->si_value.sival_int < stat_min)
            {
                stat_min = pinfo->si_value.sival_int;
            }

            // stats: calculate average (running average so we don't need to keep track of all numbers received)
            stat_avg = stat_avg * (stat_total_received - 1) / stat_total_received + pinfo->si_value.sival_int / stat_total_received;
        }
    }
}

// suspend for specified signal
void signalReceive(int signal)
{
    int error;
    sigset_t wait_mask;
    
    // block all signal
    error = sigfillset(&wait_mask);
    if(error)
    {
        printf("ERROR: Failed to set signal mask, %s, Exiting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // unblock specified signal
    error = sigdelset(&wait_mask, SIGINT);
    error = sigdelset(&wait_mask, signal);
    if(error)
    {
        printf("ERROR: Failed to remove signal from set, %s, Exiting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // wait for signal
    sigsuspend(&wait_mask);
}

// send response signal to producer
void signalRespond(int pid)
{
    int error;
    
    error = kill(pid, SIGUSR1);
    if(error)
    {
        printf("ERROR: Failed to send signal to PID %u, %s, Exiting...\n", pid, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

// logs signal and value to file
void logger(int signal, int value)
{
    static FILE* logfile = NULL;
    int error;

    // create file
    if(logfile == NULL)
    {
        logfile = fopen ("log.txt", "w");
        if(logfile == NULL)
        {
            printf("ERROR: Failed to create/open log.txt, %s, Exiting...", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // write entry
    error = fprintf(logfile, "Signal: %d Value: %d\n", signal, value);
    if(error < 0)
    {
        printf("ERROR: Failed to write to log.txt, %s, Exiting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fflush(logfile);
}