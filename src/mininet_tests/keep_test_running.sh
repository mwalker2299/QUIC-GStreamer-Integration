#!/bin/bash

while [ ! -f /home/matt/T7/mininet_results/DONE ] ;
do
      killall gst-launch-1.0
      mn -c
      python3 test_runner.py -i 1 -o /home/matt/T7/ >test_results_stdout.txt 2>test_results_stderr.txt
done
