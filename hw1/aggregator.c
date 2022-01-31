#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

// global data shared between main routine and signal handler
pid_t producer_pids[5];
int producer_vals[5];
int producer_initialized[5] = {0, 0, 0, 0, 0};
int producer_terminated[5] = {0, 0, 0, 0, 0}; 

// SIGRTMIN handler
void signalHandler(int signal, siginfo_t* pinfo, void* pcontext)
{
    int producer = signal - SIGRTMIN;

    // initialize if first signal received
    if(!producer_initialized[producer])
    {
        producer_pids[producer] = pinfo->si_value.sival_int;
        producer_initialized[producer] = 1;

        printf("Producer %u sent initialize PID value = %d!\n", producer, pinfo->si_value.sival_int);
    }
    else
    {
        // check if negative 
        if(pinfo->si_value.sival_int == -1)
        {
            producer_terminated[producer] = 1;

            printf("Producer %u sent termination value!\n", signal);
        }
        else
        {
            producer_vals[producer] = pinfo->si_value.sival_int;
        }
    }
}

void main(int argc, char* argv[])
{
    sigset_t blocked_mask, old_mask;
    sigset_t wait_mask;

    struct sigaction new_action, old_action;

    int error;
    int producer, max_producers;
    pid_t aggregator_pid;
    
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
    // TODO - apparently we can't just save the initial PID returned from fork?
    for(producer = 0; producer < max_producers; producer++)
    {
        producer_pids[producer] = fork();

        // create producer process
        if(producer_pids[producer] == 0)
        {
            // write current producer ID to arg vector
            sprintf(str_producer_id, "%u", producer);
            
            execvp(producer_args[0], producer_args);
        }

    }

    // wait for all signals to intialize
    for(producer = 0; producer < max_producers; producer++)
    {
        // block all signals
        error = sigfillset(&wait_mask);
        if(error)
        {
            printf("ERROR: Failed to set signal mask, %s, Exiting...", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // remove current producer signal
        error = sigdelset(&wait_mask, SIGRTMIN + producer);
        if(error)
        {
            printf("ERROR: Failed to remove signal from set, %s, Exiting...", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // wait for signal
        sigsuspend(&wait_mask);
    }

    // wait for initialization signals from each process
    while(1)
    {
        sleep(1);
        printf("Aggregator alive..\n");
    }

    // TODO
    printf("producers have initialized!\n");

    sleep(1);
    printf("Aggregator ended\n");
}