Assignment 3 Part 2 can be compiled by running make on the included makefile. This build process will
generate four executable: prodcons.out, producer.out, modifier.out, and consumer.out. Prodcont.out is
the main process that be executed and will fork the other three processes.

Prodcon.out can be executed from the command line and acceptsthe following input arguments:
    - Arg 1 (required): Size of item buffer (i.e. maximum number of items that can fit in buffer)
    - Arg 2 (optional): Number of items produced by producer.

Example:

./prodcons.out 7 3333

will print...

--------------------------------
|           SUCCESS             
--------------------------------
Buffer Size: 7
Produced Items: 1000
Log files outputted locally
--------------------------------

