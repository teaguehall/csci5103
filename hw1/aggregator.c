#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

// prototypes
int aggregatorMax(void);
int aggregatorMin(void);
int aggregatorSum(void);
int aggrevatorReceived(void);
double aggregatorAvg(void);
void signalHandler(int signal, siginfo_t* pinfo, void* pcontext);
void signalReceive(int signal);
void signalRespond(int pid);
void logger(int signal, int value);

// global data shared between main routine and signal handler
pid_t producer_pids[5];
int producer_vals[5] = {-1, -1, -1, -1, -1}; // -1 indicates inactive producer
int all_terminated = 0;

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
    while(!all_terminated)
    {
        // receive value from each producer
        for(producer = 0; producer < max_producers; producer++)
        {
            // only wait for producer if it's still active
            if(producer_vals[producer] > -1)
            {
                signalReceive(SIGRTMIN + producer);
                signalRespond(producer_pids[producer]);
            }
        }

        // print stats
        printf("Round %u: Avg = %f, Max = %d, Min = %d, Sum = %d, Numbers Received = %d\n", round++, aggregatorAvg(), aggregatorMax(), aggregatorMin(), aggregatorSum(), aggrevatorReceived());
    }

    printf("Aggregator ended\n");
}

// handler for RT signals
void signalHandler(int signal, siginfo_t* pinfo, void* pcontext)
{
    int producer = signal - SIGRTMIN;

    // initialize if first signal received
    if(producer_vals[producer] == -1)
    {
        producer_pids[producer] = pinfo->si_value.sival_int;
        producer_vals[producer] = 0;
    }
    else
    {
        producer_vals[producer] = pinfo->si_value.sival_int;
    }

    // check if all terminated
    for(int i = 0; i < 5; i++)
    {
        if(producer_vals[i] == -1)
        {
            all_terminated = 1;
        }
        else
        {
            all_terminated = 0;
            break;
        }
    }
}

// suspend for specified signal
void signalReceive(int signal)
{
    sigset_t wait_mask;
    int error;
    
    // block all signal
    error = sigfillset(&wait_mask);
    if(error)
    {
        printf("ERROR: Failed to set signal mask, %s, Exiting...", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // unblock specified signal
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

    // create/open file
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

// returns max val
int aggregatorMax(void)
{
    int producer;
    int max = INT_MIN;

    // iterate over all values
    for(producer = 0; producer < 5; producer++)
    {
        // skip values of inactive producer
        if(producer_vals[producer] > -1)
        {
            // set new max if necessary
            if(producer_vals[producer] > max)
            {
                max = producer_vals[producer];
            }
        }
    }

    return max;
}

// returns min val
int aggregatorMin(void)
{
    int producer;
    int min = INT_MAX;

    // iterate over all values
    for(producer = 0; producer < 5; producer++)
    {
        // skip values of inactive producer
        if(producer_vals[producer] > -1)
        {
            // set new max if necessary
            if(producer_vals[producer] < min)
            {
                min = producer_vals[producer];
            }
        }
    }

    return min;
}

// returns average val
double aggregatorAvg(void)
{
    int producer;
    int sum = 0;
    int vals = 0;

    // iterate over all values
    for(producer = 0; producer < 5; producer++)
    {
        // skip values of inactive producer
        if(producer_vals[producer] > -1)
        {
            sum += producer_vals[producer];
            vals++;
        }
    }

    return 1.0 * sum / vals;
}

// sums values
int aggregatorSum(void)
{
    int producer;
    int sum = 0;
    
    // iterate over all values
    for(producer = 0; producer < 5; producer++)
    {
        // skip values of inactive producer
        if(producer_vals[producer] > -1)
        {
            sum += producer_vals[producer];
        }
    }

    return sum;
}

// returns number of received values
int aggrevatorReceived(void)
{
    int producer;
    int vals = 0;
    
    // iterate over all values
    for(producer = 0; producer < 5; producer++)
    {
        // skip values of inactive producer
        if(producer_vals[producer] > -1)
        {
            vals++;
        }
    }

    return vals;   
}