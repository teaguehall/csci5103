#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void main(int argc, char* argv[])
{
    int producer, max_producers;
    pid_t producer_pids[5]; // allocate for max number of producers
    char* producer_args[]= { "./producer.out", NULL};

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

    // create producers
    for(producer = 0; producer < max_producers; producer++)
    {
        producer_pids[producer] = fork();

        // create producer process
        if(producer_pids[producer] == 0)
        {
            execvp(producer_args[0], producer_args);
        }
        
    }

    sleep(1);
    printf("Aggregator ended\n");
}