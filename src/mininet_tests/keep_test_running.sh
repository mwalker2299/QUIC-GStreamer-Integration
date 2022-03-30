#!/bin/bash
OUTPUT=""                                   
usage() {                                 
  echo "This command will run the tests using the test_runner.py script. \nIf the test stops due to a crash, this script will automatically restart it"
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
      killall gst-launch-1.0
      mn -c
      python3 test_runner.py -i $ITERS -o $OUTPUT 2>>$OUTPUT/final_results_stderr.txt
done
