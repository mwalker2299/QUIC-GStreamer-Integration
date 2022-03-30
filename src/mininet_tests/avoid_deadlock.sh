#!/bin/bash
OUTPUT=""  
ITERS=""                                 
usage() {                                 
  echo "This command will run keep_tests_running.sh, restarting it every 30mins to avoid mininet stalls"
  echo "please provide an output dir for the results using -o\nand a number of test iterations via -i"
}
exit_abnormal() {                         
  usage
  exit 1
}
while getopts "o:i:" options; do         
                                          
                                          
  case "${options}" in                    
    o)                                    
      OUTPUT=${OPTARG}                      
      ;;
    i)                                    
      ITERS=${OPTARG}                      
      ;;
    :)                                    
      echo "Error: -${OPTARG} requires an argument."
      exit_abnormal                       
      ;;
    *)                                    
      exit_abnormal                       
      ;;
  esac
done
if [ "$OUTPUT" = "" ]; then                 
  usage
  exit 0                                  
fi 
if [ "$ITERS" = "" ]; then                 
  usage
  exit 0                                  
fi 

OUTPUT=${OUTPUT%/}

while [ ! -f ${OUTPUT}/mininet_results/DONE ] ;
do
      ./keep_test_running.sh -o $OUTPUT -i $ITERS &
      last_pid=$!
      echo ${last_pid}
      sleep 30m
      kill -KILL $last_pid
done