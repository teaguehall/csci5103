#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void main(int argc, char* argv[])
{
    int producer, max_producers;
    pid_t aggregator_pid;
    pid_t producer_pids[5]; // allocate for max number of producers
    char str_aggregator_pid[1024];
    char str_producer_id[1024];
    char* producer_args[]= { "./producer.out", str_aggregator_pid, str_producer_id};

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

    // write aggregator pid to arg vector
    sprintf(str_aggregator_pid, "%u", getpid());


    // create producers
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

    sleep(1);
    printf("Aggregator ended\n");
}