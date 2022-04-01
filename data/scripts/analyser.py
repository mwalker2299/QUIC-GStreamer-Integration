#!/usr/bin/python

import os
import stack_latency_analysis
import gstreamer_log_analysis
import plot_graphs as plot_graphs
import pandas as pd
import numpy as np


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
            stack_latency_arrival_pd   = stack_latency_analysis.convert_stack_latency_results_to_panda(stack_latency_arrival)
            stack_latency_available_pd = stack_latency_analysis.convert_stack_latency_results_to_panda(stack_latency_available)
            stack_latency_analysis.save_results_stream(arrival_time_diff_panda=stack_latency_arrival_pd, available_time_diff_panda=stack_latency_available_pd, directory=dirpath)
          else:
            stack_latency_pd = stack_latency_analysis.convert_stack_latency_results_to_panda(stack_latency)
            stack_latency_analysis.save_results_datagram(time_diff_panda=stack_latency_pd, directory=dirpath)

          # Extract nal latency data
          nal_latency    = gstreamer_log_analysis.extract_Nal_unit_data(dirpath)
          nal_latency_pd = gstreamer_log_analysis.convert_nal_results_to_panda(nal_latency)
          gstreamer_log_analysis.save_nal_results(time_diff_panda=nal_latency_pd, directory=dirpath)

          # Extract frame latency data
          frame_latency    = gstreamer_log_analysis.extract_frame_data(dirpath)
          frame_latency_pd = gstreamer_log_analysis.convert_frame_results_to_panda(frame_latency)
          frame_stats_pd   = gstreamer_log_analysis.identify_useful_frames(frame_latency, dirpath)
          gstreamer_log_analysis.save_frame_results(time_diff_panda=frame_latency_pd, frame_stats_panda=frame_stats_pd, directory=dirpath)


        except Exception as e:
          tb = traceback.format_exc()
          print("exception while processing" + dirpath)
          print("exception is " + str(e))
          print("traceback is: ", tb)
          print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n")


def calculate_HOL_blocking_delay(results_path):

  try:
    os.path.exists(results_path)
  except:
    print("Analysis failed! Path to results" + results_path + " does not exist")
    return 1
  
  for dirpath, dirnames, filenames in os.walk(results_path):

    if not dirnames:
      if "Failure" in filenames:
          continue

      try:

        # Calculate average HOL blocking delay
        if "UDP" in dirpath or "QUIC_PPS" in dirpath:
          stack_latency_available_csv_filename = os.path.join(dirpath,"Stack_Latency.csv")
          stack_latency_csv_filename           = os.path.join(dirpath,"Stack_Latency.csv")
        else:
          stack_latency_available_csv_filename = os.path.join(dirpath,"Stack_Latency_available.csv")
          stack_latency_csv_filename           = os.path.join(dirpath,"Stack_Latency_arrival.csv")

        
        stack_latency_available_pd = pd.read_csv(stack_latency_available_csv_filename)
        stack_latency_pd = pd.read_csv(stack_latency_csv_filename)

        stack_latency_available_np = pd.DataFrame.to_numpy(stack_latency_available_pd)
        stack_latency_np = pd.DataFrame.to_numpy(stack_latency_pd)

        stack_latency_available_values = stack_latency_available_np[:,1]
        stack_latency_values           = stack_latency_np[:,1]
        packet_numbers                 = stack_latency_np[:,0]

        HOL_blocking_delay_per_packet  = stack_latency_available_values - stack_latency_values

        

        HOL_blocking_delay_per_packet_pd = pd.DataFrame(data=np.vstack((packet_numbers, HOL_blocking_delay_per_packet)).T, columns=["Packet Num.", "HOL Blocking Delay"])

        HOL_blocking_delay_per_packet_pd.to_csv(os.path.join(dirpath, "HOL_Blocking_Delay.csv"),index=False)

      except Exception as e:
        tb = traceback.format_exc()
        print("exception while processing" + dirpath)
        print("exception is " + str(e))
        print("traceback is: ", tb)
        print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n")

def calculate_total_HOL_blocking_delay(results_path, output_path):

  try:
    os.path.exists(results_path)
  except:
    print("Analysis failed! Path to results" + results_path + " does not exist")
    return 1

  try:
    print(output_path)
    if not os.path.exists(output_path):
      print("output_path: ",output_path," does not exist. Creating...")
      os.mkdir(output_path)
  except:
    print("output_path: ",output_path," does not exist. Creating...")
    os.mkdir(output_path)

  for dirpath, dirnames, filenames in os.walk(results_path):
    if "Jitter_0ms" not in dirnames:
      continue

    delay=plot_graphs.find_value_from_string(str(dirpath), "Delay_", "ms/Loss")
    loss=plot_graphs.find_value_from_string(str(dirpath), "Loss_", "%")
    protocol=plot_graphs.find_value_from_string(str(dirpath), "results/", "/Delay")

    protocol_output_dir = os.path.join(output_path, protocol)
    try:
      os.mkdir(protocol_output_dir)
    except:
      pass

    HOL_blocking_delay_list = []

    for dirpath, dirnames, filenames in os.walk(dirpath):
      if dirnames:
        continue

      if "Failure" in filenames:
          continue

      try:
      
        HOL_blocking_delay_per_packet_pd = pd.read_csv(os.path.join(dirpath, "HOL_Blocking_Delay.csv"))
        HOL_blocking_delay_per_packet_np = pd.DataFrame.to_numpy(HOL_blocking_delay_per_packet_pd)

        HOL_blocking_delay_values = HOL_blocking_delay_per_packet_np[:,1]

        HOL_blocking_delay_list.append(HOL_blocking_delay_values)
        print(HOL_blocking_delay_values.shape)
        print(dirpath)

      except Exception as e:
        tb = traceback.format_exc()
        print("exception while processing" + dirpath)
        print("exception is " + str(e))
        print("traceback is: ", tb)
        print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n")



    # Get average across all buffer delays and iterations
    HOL_summed_across_runs = np.zeros(HOL_blocking_delay_list[0].shape)
    count = 0
    for HOL_blocking_delay in HOL_blocking_delay_list:
        if HOL_blocking_delay.shape != HOL_summed_across_runs.shape:
          if HOL_blocking_delay.shape == 16426 or HOL_blocking_delay.shape == 16416:
            HOL_summed_across_runs = HOL_blocking_delay
            continue
          else:
            # Suggests an error, skip this value:
            continue
        elif np.sum(HOL_blocking_delay) < 0:
          # Suggests an error, skip this value:
          continue
        count += 1
        HOL_summed_across_runs += HOL_blocking_delay

    average_HOL_blocking_delay_per_packet = HOL_summed_across_runs/count

    # Convert to a column vector
    average_HOL_blocking_delay_per_packet = average_HOL_blocking_delay_per_packet[:,None]

    average_HOL_blocking_delay_per_packet_pd = pd.DataFrame(data=average_HOL_blocking_delay_per_packet, columns=["HOL Blocking Delay (ms)"])
    average_HOL_blocking_delay_per_packet_pd.to_csv(os.path.join(protocol_output_dir, "Delay_"+delay+"ms_Loss_"+loss+"%_HOL_Blocking_Delay.csv"),index=False)

    total_HOL_blocking_delay = np.sum(average_HOL_blocking_delay_per_packet)

    total_HOL_blocking_delay_pd = pd.DataFrame(data=[total_HOL_blocking_delay], columns=["Total HOL Blocking Delay (ms)"])
    total_HOL_blocking_delay_pd.to_csv(os.path.join(protocol_output_dir, "Delay_"+delay+"ms_Loss_"+loss+"%_Total_HOL_Blocking_Delay.csv"),index=False)

  plot_graphs.plot_total_HOL_blocking_delay(output_path)

def calculate_average_useful_frames(results_path, output_path):

  try:
    os.path.exists(results_path)
  except:
    print("Analysis failed! Path to results" + results_path + " does not exist")
    return 1

  try:
    if not os.path.exists(output_path):
      print("output_path: ",output_path," does not exist. Creating...")
      os.mkdir(output_path)
  except:
    print("output_path: ",output_path," does not exist. Creating...")
    os.mkdir(output_path)

  protocol_frame_usefulness = {}

  for dirpath, dirnames, filenames in os.walk(results_path):
    if "Iteration" not in '\t'.join(dirnames):
      continue

    propagation_delay=float(plot_graphs.find_value_from_string(str(dirpath), "Delay_", "ms/Loss"))
    loss=float(plot_graphs.find_value_from_string(str(dirpath), "Loss_", "%"))
    buffer_delay=float(plot_graphs.find_value_from_string(str(dirpath), "Buffer_Delay_", "ms/Bandwidth"))
    protocol=plot_graphs.find_value_from_string(str(dirpath), "results/", "/Delay")


    if protocol not in protocol_frame_usefulness:
      protocol_frame_usefulness[protocol] = {}

    if loss not in protocol_frame_usefulness[protocol]:
      protocol_frame_usefulness[protocol][loss] = {}

    if propagation_delay not in protocol_frame_usefulness[protocol][loss]:
      protocol_frame_usefulness[protocol][loss][propagation_delay] = {}

    if buffer_delay not in protocol_frame_usefulness[protocol][loss][propagation_delay]:
      protocol_frame_usefulness[protocol][loss][propagation_delay][buffer_delay] = 0.0

      

    protocol_output_dir = os.path.join(output_path, protocol)
    try:
      os.mkdir(protocol_output_dir)
    except:
      pass

    average_frame_useful = []

    for dirpath, dirnames, filenames in os.walk(dirpath):
      if dirnames:
        continue

      if "Failure" in filenames:
          continue

      if "Frame_stats.csv" not in filenames:
        continue
      
      Frame_stats_pd = pd.read_csv(os.path.join(dirpath, "Frame_stats.csv"))
      Frame_stats_np = pd.DataFrame.to_numpy(Frame_stats_pd)

      useful_frames_value = Frame_stats_np[0,-1]
      average_frame_useful.append(useful_frames_value)


    average_frame_useful_np = np.asarray(average_frame_useful)

    try:
      average_frame_useful_across_runs = np.round(np.average(average_frame_useful_np),2)
    except:
      average_frame_useful_across_runs = 0

    if np.isnan(average_frame_useful_across_runs):
      average_frame_useful_across_runs = 0

    protocol_frame_usefulness[protocol][loss][propagation_delay][buffer_delay] =  average_frame_useful_across_runs

  protocols = list(protocol_frame_usefulness.keys())

  for protocol in protocols:
    protocol_output_dir = os.path.join(output_path, protocol)
    data = []
    loss_params = []
    propagation_delay_params = []
    buffer_delay_params = []

    for loss in protocol_frame_usefulness[protocol].keys():
      loss_params.append(loss)
      propagation_delay_params = []
      for propagation_delay in protocol_frame_usefulness[protocol][loss].keys():
        
        propagation_delay_params.append(propagation_delay)
        buffer_delay_params = []
        for buffer_delay in protocol_frame_usefulness[protocol][loss][propagation_delay].keys():
          buffer_delay_params.append(buffer_delay)
          for useful_frame_ratio in [protocol_frame_usefulness[protocol][loss][propagation_delay][buffer_delay]]:
            data.append([loss, propagation_delay, buffer_delay, useful_frame_ratio])

    average_frame_useful_across_runs_pd = pd.DataFrame(data=data, columns=['Loss (%)', 'Delay (ms)', 'Buffer Delay (ms)', 'Useful Frames (%)'])
    with pd.option_context('display.max_rows', None, 'display.max_columns', None):
      print(protocol)
      print(average_frame_useful_across_runs_pd)
    average_frame_useful_across_runs_pd.to_csv(os.path.join(protocol_output_dir, "AVG_Frame_Usefulness.csv"),index=False)

    plot_graphs.plot_frame_usefulness(os.path.join(protocol_output_dir, "AVG_Frame_Usefulness-42.png"), protocol_frame_usefulness[protocol], loss_params, propagation_delay_params, 42)
    plot_graphs.plot_frame_usefulness(os.path.join(protocol_output_dir, "AVG_Frame_Usefulness-84.png"), protocol_frame_usefulness[protocol], loss_params, propagation_delay_params, 84)
    plot_graphs.plot_frame_usefulness(os.path.join(protocol_output_dir, "AVG_Frame_Usefulness-126.png"), protocol_frame_usefulness[protocol], loss_params, propagation_delay_params, 126)
    plot_graphs.plot_frame_usefulness(os.path.join(protocol_output_dir, "AVG_Frame_Usefulness-168.png"), protocol_frame_usefulness[protocol], loss_params, propagation_delay_params, 168)


def calculate_send_delay(results_path):
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

          server_rtp_seq_num_file = os.path.join(dirpath, "server_side_timestamps.txt")

          # Extract stack latency data
          if "UDP" in dirpath:
            packet_depatures = stack_latency_analysis.extract_rtp_data_from_udp_capture(server_rtp_seq_num_file)[:,1]
          elif "TCP" in dirpath:
            packet_depatures = stack_latency_analysis.extract_rtp_data_from_tcp_stream_available(server_rtp_seq_num_file)[:,1]
          elif "QUIC_PPS" in dirpath:
            packet_depatures = stack_latency_analysis.extract_rtp_data_from_quic_datagram(server_rtp_seq_num_file)[:,1]
          else:
            packet_depatures = stack_latency_analysis.extract_rtp_data_from_quic_stream_arrival(server_rtp_seq_num_file)[:,1]



          # Start from zero
          packet_depatures = packet_depatures - packet_depatures[0]

          packet_send_times = gstreamer_log_analysis.determine_packet_send_time(dirpath)

          time_difference = packet_depatures - packet_send_times[:,1]
          time_difference[time_difference<0] = 0


          packet_sending_delay = np.vstack((packet_send_times[:,0], time_difference)).T

          print(np.sum(packet_sending_delay))
          print(dirpath)


          packet_sending_delay_pd = pd.DataFrame(data=packet_sending_delay, columns=["Packet No.", "Sending Delay (ms)"])
          packet_sending_delay_pd.to_csv(os.path.join(dirpath, "Sending_Delay.csv"),index=False)



        except Exception as e:
          tb = traceback.format_exc()
          print("exception while processing" + dirpath)
          print("exception is " + str(e))
          print("traceback is: ", tb)
          print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n")

def calculate_total_sending_delay(results_path, output_path):

  try:
    os.path.exists(results_path)
  except:
    print("Analysis failed! Path to results" + results_path + " does not exist")
    return 1

  try:
    print(output_path)
    if not os.path.exists(output_path):
      print("output_path: ",output_path," does not exist. Creating...")
      os.mkdir(output_path)
  except:
    print("output_path: ",output_path," does not exist. Creating...")
    os.mkdir(output_path)

  for dirpath, dirnames, filenames in os.walk(results_path):
    if "Jitter_0ms" not in dirnames:
      continue

    delay=plot_graphs.find_value_from_string(str(dirpath), "Delay_", "ms/Loss")
    loss=plot_graphs.find_value_from_string(str(dirpath), "Loss_", "%")
    protocol=plot_graphs.find_value_from_string(str(dirpath), "results/", "/Delay")

    protocol_output_dir = os.path.join(output_path, protocol)
    try:
      os.mkdir(protocol_output_dir)
    except:
      pass

    send_delay_list = []

    for dirpath, dirnames, filenames in os.walk(dirpath):
      if dirnames:
        continue

      if "Failure" in filenames:
          continue

      try:
      
        send_delay_per_packet_pd = pd.read_csv(os.path.join(dirpath, "Sending_Delay.csv"))
        send_delay_per_packet_np = pd.DataFrame.to_numpy(send_delay_per_packet_pd)

        send_delay_values = send_delay_per_packet_np[:,1]

        send_delay_list.append(send_delay_values)

      except Exception as e:
        tb = traceback.format_exc()
        print("exception while processing" + dirpath)
        print("exception is " + str(e))
        print("traceback is: ", tb)
        print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n")



    # Get average across all buffer delays and iterations
    send_delay_summed_across_runs = np.zeros(send_delay_list[0].shape)
    count = 0
    for send_delay in send_delay_list:
        if send_delay.shape != send_delay_summed_across_runs.shape:
          if send_delay.shape == 16426 or send_delay.shape == 16416:
            send_delay_summed_across_runs = send_delay
            continue
          else:
            # Suggests an error, skip this value:
            continue
        elif np.sum(send_delay) < 0:
          # Suggests an error, skip this value:
          continue
        count += 1
        send_delay_summed_across_runs += send_delay

    average_send_delay_per_packet = send_delay_summed_across_runs/count

    # Convert to a column vector
    average_send_delay_per_packet = average_send_delay_per_packet[:,None]

    average_send_delay_per_packet_pd = pd.DataFrame(data=average_send_delay_per_packet, columns=["Send Delay (ms)"])
    average_send_delay_per_packet_pd.to_csv(os.path.join(protocol_output_dir, "Delay_"+delay+"ms_Loss_"+loss+"%_Send_Delay.csv"),index=False)

    total_send_delay = np.sum(average_send_delay_per_packet)

    total_send_delay_pd = pd.DataFrame(data=[total_send_delay], columns=["Total Send Delay (ms)"])
    total_send_delay_pd.to_csv(os.path.join(protocol_output_dir, "Delay_"+delay+"ms_Loss_"+loss+"%_Total_Send_Delay.csv"),index=False)

  plot_graphs.plot_total_send_delay(output_path)


analyse("mininet_results/")
calculate_send_delay("mininet_results/")
calculate_HOL_blocking_delay("mininet_results/")
calculate_average_useful_frames("/home/matt/T7-2/mininet_results/", "/home/matt/T7-2/frames/")
calculate_total_HOL_blocking_delay("mininet_results/", "Total_HOL/")
calculate_total_sending_delay("mininet_results/", "Total_Send/")







