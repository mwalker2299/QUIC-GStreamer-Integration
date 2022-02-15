#!/usr/bin/python

import os
import stack_latency_analysis
import gstreamer_log_analysis

def analyse(results_path):
  try:
    os.path.exists(results_path)
  except:
    print("Analysis failed! Path to results" + results_path + " does not exist")
    return 1

  for dirpath, dirnames, filenames in os.walk(results_path):
      ## We are looking for lead_nodes in the dir tree
      if not dirnames:
        if "Failure" in filenames:
          continue

        # Extract stack latency data
        if "UDP" in dirpath:
          stack_latency = stack_latency_analysis.convert_udp_capture(dirpath)
        else:
          stack_latency = stack_latency_analysis.convert_quic_capture(dirpath)

        stack_latency_pd = stack_latency_analysis.convert_results_to_panda(stack_latency)
        stack_latency_analysis.save_results(time_diff_panda=stack_latency_pd, directory=dirpath)

        # Extract app latency data
        app_latency    = gstreamer_log_analysis.extract_Nal_unit_data(dirpath)
        frame_stats_pd = gstreamer_log_analysis.identify_useful_units(app_latency)
        app_latency_pd = gstreamer_log_analysis.convert_results_to_panda(app_latency)
        gstreamer_log_analysis.save_results(time_diff_panda=app_latency_pd, frame_stats_panda=frame_stats_pd, directory=dirpath)


