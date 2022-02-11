#!/usr/bin/python

import os
import stack_latency_analysis

def analyse(results_path):
  try:
    os.path.exists(results_path)
  except:
    print("Analysis failed! Path to results" + results_path + " does not exist")
    return 1

  for dirpath, dirnames, filenames in os.walk(results_path):
      ## We are looking for lead_nodes in the dir tree
      if not dirnames:
        if "UDP" in dirpath:
          time_diff = stack_latency_analysis.convert_udp_capture(dirpath)
        else:
          time_diff = stack_latency_analysis.convert_quic_capture(dirpath)
        stack_latency_analysis.save_results(directory=dirpath, time_diff_array=time_diff)
