#!/bin/bash

while [ ! -f /home/matt/T7-2/mininet_results/DONE ] ;
do
      ./keep_test_running.sh &
      last_pid=$!
      echo ${last_pid}
      sleep 30m
      kill -KILL $last_pid
done