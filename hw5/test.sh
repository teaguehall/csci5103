#!/bin/bash

$status

if [ "$#" -ne 1 ]; then
    echo "ERROR: Invalid number of args passed. Specify test case as single number\n"
	exit 1
fi

case $1 in

  1)
    printf "\n"
	printf "*************************************************************\n"
	printf "* TEST CASE 1: Starting...\n"
	printf "*************************************************************\n"
	printf "\n"
	
	
	printf "Starting Producer: ./producer 50 BLACK\n"
	./producer 50 BLACK &
	
	printf "Starting Consumer: ./consumer 50 RED\n"
	./consumer 50 RED &

	wait
	
	
	if cmp -s "./Prod_BLACK.log" "./Cons_RED.log"; then
		#echo 'Passed'
		STATUS="PASSED: Prod_BLACK.log = Cons_RED.log"
	else
		#echo 'Failed'
		STATUS="FAILED: Prod_BLACK.log != Cons_RED.log"
	fi
	
	printf "\n"
	printf "*************************************************************\n"
	printf "* %s\n" "$STATUS"
	printf "*************************************************************\n"
	printf "\n"
	
    ;;
	
	
  2)
    printf "\n"
	printf "*************************************************************\n"
	printf "* TEST CASE 2: Starting...\n"
	printf "*************************************************************\n"
	printf "\n"
	
	
	printf "Starting Producer: ./producer 50 BLACK\n"
	./producer 50 BLACK &
	
	printf "Starting Consumer: ./consumer 10 RED\n"
	./consumer 10 RED &

	wait
	
	printf "\n"
	printf "*************************************************************\n"
	printf "* PASSED: Producer process \"did not hang\".\n"
	printf "*************************************************************\n"
	printf "\n"
	
	;;
	
  3)
    printf "\n"
	printf "*************************************************************\n"
	printf "* TEST CASE 3: Starting...\n"
	printf "*************************************************************\n"
	printf "\n"
	
	
	printf "Starting Producer: ./producer 50 BLACK\n"
	./producer 50 BLACK &
	
	printf "Starting Consumer: ./consumer 100 RED\n"
	./consumer 100 RED &

	wait
	
	if cmp -s "./Prod_BLACK.log" "./Cons_RED.log"; then
		#echo 'Passed'
		STATUS="PASSED: Consumer \"did not hang\" AND Prod_BLACK.log = Cons_RED.log"
	else
		#echo 'Failed'
		STATUS="FAILED: Consumer \"did not hang\" BUT Prod_BLACK.log != Cons_RED.log"
	fi
	
	printf "\n"
	printf "*************************************************************\n"
	printf "* %s\n" "$STATUS"
	printf "*************************************************************\n"
	printf "\n"
	
	;;
	
  4)
    printf "\n"
	printf "*************************************************************\n"
	printf "* TEST CASE 4: Starting...\n"
	printf "*************************************************************\n"
	printf "\n"
	
	
	printf "Starting Producer 1: ./producer 50 1BLACK\n"
	./producer 50 1BLACK &
	
	printf "Starting Producer 2: ./producer 50 2BLACK\n"
	./producer 50 2BLACK &
	
	printf "Starting Consumer: ./consumer 200 RED\n"
	./consumer 200 RED &

	wait
	
	printf "\n"
	printf "*************************************************************\n"
	printf "* PASSED: Processes \"did not hang\".\n"
	printf "*************************************************************\n"
	printf "\n"
	
	;;	

  5)
    printf "\n"
	printf "*************************************************************\n"
	printf "* TEST CASE 5: Starting...\n"
	printf "*************************************************************\n"
	printf "\n"
	
	
	printf "Starting Producer: ./producer 50 BLACK\n"
	./producer 50 BLACK &
	
	printf "Starting Consumer 1: ./consumer 50 1RED\n"
	./consumer 50 1RED &
	
	printf "Starting Consumer 2: ./consumer 50 2RED\n"
	./consumer 50 2RED &

	wait
	
	printf "\n"
	printf "*************************************************************\n"
	printf "* PASSED: Processes \"did not hang\".\n"
	printf "*************************************************************\n"
	printf "\n"
	
	;;		
	

  *)
    printf "ERROR: Unrecognized test case %d. Please specify test case 1 thru 5\n" $1
    ;;
esac
