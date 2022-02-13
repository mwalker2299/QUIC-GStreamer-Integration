from datetime import datetime
from re import U
import time
import os
import pandas as pd

import numpy as np


# Format of Nal unit type identification log line and last sequence number identification log line on server side
'''
0:00:17.814566771 [336m1507487[00m 0x55806ef5c520 [37mDEBUG  [00m [00m          rtph264pay gstrtph264pay.c:821:gst_rtp_h264_pay_payload_nal:<rtph264pay0>[00m Processing Buffer with NAL TYPE=1 0:00:17.720000000
0:00:17.814660170 [336m1507487[00m 0x55806ef5c520 [33;01mLOG    [00m [00m      rtpbasepayload gstrtpbasepayload.c:1344:gst_rtp_base_payload_prepare_push:<rtph264pay0>[00m Preparing to push list with size 6, seq=23319, rtptime=526903185, pts 0:00:17.720000000
'''

# Format of sequence number identification log line and complete Nal unit identification log line on client side
'''
0:00:00.197921663 [334m476745[00m 0x561e00ca16a0 [33;01mLOG    [00m [00m    rtpbasedepayload gstrtpbasedepayload.c:394:gst_rtp_base_depayload_handle_buffer:<rtph264depay0>[00m discont 0, seqnum 6809, rtptime 2331433938, pts 0:00:00.002168297, dts 0:00:00.002797405
0:00:00.197936443 [334m476745[00m 0x561e00ca16a0 [37mDEBUG  [00m [00m        rtph264depay gstrtph264depay.c:892:gst_rtp_h264_depay_handle_nal:<rtph264depay0>[00m handle NAL type 8
'''

# Iterates through each line in the server side logs. 
# It first identifies the type of a Nal unit and the timestamp at which it arrived. 
# After finding the Nal unit, it searches the succeeding lines for the last sequence number of the rtp packets
# which will carry the nal unit.
def extract_Nal_unit_data_from_server_logs(filename):
  with open(filename, 'r') as log_file:
    Lines = log_file.readlines()
    locate_seq = False

    nal_info = []
    nal_timestamp_in_milliseconds = -1
    nal_type = -1

    for line in Lines:
      
      if locate_seq:
        if "Preparing to push" in line:
          line_contents = str.split(line)
          sequence_number = int(line_contents[15][4:-1])
          nal_info.append([sequence_number, nal_type, nal_timestamp_in_milliseconds])
          locate_seq = False
        else:
          continue

      if "NAL TYPE=" in line:
        line_contents = str.split(line)
        
        timestamp = line_contents[0]
        nal_type  = int(line_contents[12][-1])

        datetime_format = datetime.strptime(timestamp[:-3], '%H:%M:%S.%f')
        nal_timestamp_in_milliseconds = datetime_format.timestamp() * 1000

        locate_seq = True

    nal_info = np.asarray(nal_info)

    return nal_info

# Iterates through each line in the client side logs. 
# While we iterate through the lines, we keep a record of the last seq num we have seen.
# Then, when we encounter a completed Nal unit ("handle NAL type" appears), we note the unit type
# and the timestamp at which it was completed. This will allow us to determine the app latency,
# as well as determining which nal units are key-frames
def extract_Nal_unit_data_from_client_logs(filename):
  with open(filename, 'r') as log_file:
    Lines = log_file.readlines()

    nal_info = []
    nal_timestamp_in_milliseconds = -1
    nal_type = -1
    sequence_number = -1

    for line in Lines:
      
      if "gstrtpbasedepayload.c:394:gst_rtp_base_depayload_handle_buffer" in line:
        line_contents = str.split(line)
        sequence_number = int(line_contents[11][:-1])
        continue

      if "handle NAL type" in line:
        line_contents = str.split(line)
        
        timestamp = line_contents[0]
        nal_type  = int(line_contents[11])

        datetime_format = datetime.strptime(timestamp[:-3], '%H:%M:%S.%f')
        nal_timestamp_in_milliseconds = datetime_format.timestamp() * 1000

        nal_info.append([sequence_number, nal_type, nal_timestamp_in_milliseconds])
    
    nal_info = np.asarray(nal_info)

    return nal_info

# calculates difference in time between nal units arriving at the rtp payloader on server side, and the nal units leaving the rtp depayloader on the client side.
# If not all of the fragments which make up a nal unit arrive, that unit is marked with a -1 time difference
def calculate_time_diffs(nal_arrival, nal_depature):
  depature_count = 0
  result = np.zeros(nal_arrival.shape)

  for i, row in enumerate(nal_arrival):
    packet_number = row[0]
    nal_type = row[1]
    arrival_timestamp = row[2]

    result[i,0] = packet_number
    result[i,1] = nal_type

    if depature_count < len(nal_depature) and nal_depature[depature_count,0] == packet_number:
      if nal_type != nal_depature[depature_count,1]:
        raise Exception
      result[i,2] = nal_depature[depature_count,2] - arrival_timestamp 
      depature_count+=1
    else:
      result[i,2] = -1
  
  return result






def extract_Nal_unit_data(directory):
  server_log = os.path.join(directory, "gst-server.log")
  client_log = os.path.join(directory, "gst-client.log")


  nal_arrival  = extract_Nal_unit_data_from_server_logs(server_log)
  nal_depature = extract_Nal_unit_data_from_client_logs(client_log)

  return calculate_time_diffs(nal_arrival=nal_arrival, nal_depature=nal_depature)

# calculate_time_diffs will provide a -1 timestamp to any nal units which did not arrive intact
# We can then identify which frames are useful. An I-frame may sometimes be split into two consectutive 
# Nal units of type 1, so the function takes this into account.
def identify_useful_units(nal_time_diff):
  last_i_frame_was_complete = False
  total_frames           = 0
  total_p_frame_count    = 0
  total_i_frame_count    = 0
  complete_i_frame_count = 0
  complete_p_frame_count = 0
  useful_i_frame_count   = 0
  useful_p_frame_count   = 0
  ratio_complete_frames  = 0
  ratio_useful_frames    = 0
  

  prev_row = [0,0,0]


  for row in nal_time_diff:
    nal_type = row[1]
    complete_unit = (int(row[2]) != -1)

    if nal_type != 1 and nal_type != 5:
      prev_row = row
      continue

    if nal_type == 5:
      # We do not want to increase the frame count twice in the case that an I-frame is split across two Nal units
      if prev_row[1] != 5:
        total_frames += 1
        total_i_frame_count += 1
        last_i_frame_was_complete = complete_unit
      else:
        last_i_frame_was_complete = (complete_unit and last_i_frame_was_complete)

    if nal_type == 1:
      total_frames += 1
      total_p_frame_count += 1
      if prev_row[1] == 5 and last_i_frame_was_complete:
        complete_i_frame_count += 1
        useful_i_frame_count   += 1

      if complete_unit:
        complete_p_frame_count += 1
        if last_i_frame_was_complete:
          useful_p_frame_count += 1
    
    prev_row = row

  ratio_complete_frames = ((complete_i_frame_count+complete_p_frame_count)/total_frames)*100
  ratio_useful_frames = ((useful_i_frame_count+useful_p_frame_count)/total_frames)*100

  results = {
  'Total Frames': [total_frames],
  'Total I-Frames': [total_i_frame_count],
  'Complete I-Frames': [complete_i_frame_count],
  'Useful I-Frames': [useful_i_frame_count],
  'Total P-Frames': [total_p_frame_count],
  'Complete P-Frames': [complete_p_frame_count],
  'Useful P-Frames': [useful_p_frame_count],
  '\% Complete Frames': [ratio_complete_frames],
  '\% Useful Frames': [ratio_useful_frames]
  }

  return pd.DataFrame(results)


def convert_results_to_panda(results):
  return pd.DataFrame(data=results, columns=["Packet Num.", "Nal Type", "App latency"])
  
def save_results(time_diff_panda, frame_stats_panda, directory):
  latency_file_path     = os.path.join(directory, "App_latency.csv")
  frame_stats_file_path = os.path.join(directory, "Frame_stats.csv")
  time_diff_panda.to_csv(latency_file_path, index=False)
  frame_stats_panda.to_csv(frame_stats_file_path, index=False)





# string  = "0:00:00.197921663 [334m476745[00m 0x561e00ca16a0 [33;01mLOG    [00m [00m    rtpbasedepayload gstrtpbasedepayload.c:394:gst_rtp_base_depayload_handle_buffer:<rtph264depay0>[00m discont 0, seqnum 6809, rtptime 2331433938, pts 0:00:00.002168297, dts 0:00:00.002797405"
# string2 = "0:00:00.197936443 [334m476745[00m 0x561e00ca16a0 [37mDEBUG  [00m [00m        rtph264depay gstrtph264depay.c:892:gst_rtp_h264_depay_handle_nal:<rtph264depay0>[00m handle NAL type 8"

# line_contents = str.split(string)

# print(int(line_contents[11][:-1]))

# line_contents2 = str.split(string2)

# print(line_contents2[0] + "     " + str(int(line_contents2[11])))


