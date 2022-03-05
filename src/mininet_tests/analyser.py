#!/usr/bin/python

import os
import stack_latency_analysis
import gstreamer_log_analysis
import traceback

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

        try:

          # Extract stack latency data
          if "UDP" in dirpath:
            stack_latency = stack_latency_analysis.convert_udp_capture(dirpath)
          elif "TCP" in dirpath:
            stack_latency_arrival, stack_latency_available = stack_latency_analysis.convert_tcp_capture(dirpath)
          elif "QUIC_PPS" in dirpath:
            stack_latency = stack_latency_analysis.convert_quic_packet_capture(dirpath)
          else:
            stack_latency_arrival, stack_latency_available = stack_latency_analysis.convert_quic_stream_capture(dirpath)

          if "UDP" not in dirpath and "QUIC_PPS" not in dirpath:
            stack_latency_arrival_pd   = stack_latency_analysis.convert_results_to_panda(stack_latency_arrival)
            stack_latency_available_pd = stack_latency_analysis.convert_results_to_panda(stack_latency_available)
            stack_latency_analysis.save_results_stream(arrival_time_diff_panda=stack_latency_arrival_pd, available_time_diff_panda=stack_latency_available_pd, directory=dirpath)
          else:
            stack_latency_pd = stack_latency_analysis.convert_results_to_panda(stack_latency)
            stack_latency_analysis.save_results_datagram(time_diff_panda=stack_latency_pd, directory=dirpath)

          # Extract nal latency data
          nal_latency    = gstreamer_log_analysis.extract_Nal_unit_data(dirpath)
          nal_latency_pd = gstreamer_log_analysis.convert_nal_results_to_panda(nal_latency)
          gstreamer_log_analysis.save_nal_results(time_diff_panda=nal_latency_pd, directory=dirpath)

          # Extract frame latency data
          frame_latency    = gstreamer_log_analysis.extract_frame_data(dirpath)
          frame_latency_pd = gstreamer_log_analysis.convert_frame_results_to_panda(frame_latency)
          frame_stats_pd   = gstreamer_log_analysis.identify_useful_frames(frame_latency)
          gstreamer_log_analysis.save_frame_results(time_diff_panda=frame_latency_pd, frame_stats_panda=frame_stats_pd, directory=dirpath)


        except Exception as e:
          tb = traceback.format_exc()
          print("exception while processing" + dirpath)
          print("exception is " + str(e))
          print("traceback is: ", tb)
          print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n")


# analyse("/home/matt/T7/mininet_results/UDP/")
analyse("/home/matt/T7/mininet_results/QUIC_PPS/Delay_10ms/Loss_0%/Jitter_0ms/Buffer_Delay_42ms")