#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "ERROR: Invalid number of args passed. Specify test case as single number\n"
	exit 1
fi

case $1 in

  1)
    printf "Running test 1\n"
    ;;
	
  2)
    printf "Running test 2\n"
    ;;	

  *)
    printf "ERROR: Unrecognized test case %d. Please specify test case 1 thru 5\n" $1
    ;;
esac
