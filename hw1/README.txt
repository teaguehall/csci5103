Assignment 1 can be compiled by running make on the included makefile. This build process will
generate two executables: aggregator.out and producer.out. Aggregator.out can be executed from
the command line and requires an integer input argument to specify how many producers will be
spawned. Running this program will spawn the specified number of producer and print statistics.

Example:

./aggregator.out 3

will print...

--------------------------------
AGGREGATOR FINISHED - STATS
--------------------------------
Max: 700
Min: 1
Average: 133.541667
Signals Received - Producer 1: 9
Signals Received - Producer 2: 8
Signals Received - Producer 3: 7
Signals Received - Total: 24
--------------------------------

Additionally, all received signals will be logged to a local log.txt file.

Included with this submission are 5 data files from Test-datafiles-2. The program successfully
works on all variations of these test files along with Test-datafiles-1 (not included).
