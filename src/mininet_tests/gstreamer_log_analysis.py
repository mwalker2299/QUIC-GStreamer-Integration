from datetime import datetime
from re import U
import time
import os
import pandas as pd
import sys
import numpy as np
np.set_printoptions(threshold=sys.maxsize)


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
    locate_seq = False

    nal_info = []
    nal_timestamp_in_milliseconds = -1
    nal_type = -1

    for line in log_file:
      
      if locate_seq:
        if "Preparing to push" in line:
          
          search_string_left  = "seq="
          search_string_right = ", rtptime="

          search_string_left_index  = line.find(search_string_left)
          search_string_right_index = line.find(search_string_right)

          sequence_number = int(line[search_string_left_index+len(search_string_left):search_string_right_index])
          nal_info.append([sequence_number, nal_type, nal_timestamp_in_milliseconds])
          locate_seq = False
        else:
          continue

      if "NAL TYPE=" in line:
        
        timestamp = line[:17]
        search_string = "NAL TYPE="
        nal_type_index = line.find(search_string)
        nal_type  = int(line[nal_type_index+len(search_string)])

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

    nal_info = []
    nal_timestamp_in_milliseconds = -1
    nal_type = -1
    sequence_number = -1

    for line in log_file:
      
      if "gstrtpbasedepayload.c:394:gst_rtp_base_depayload_handle_buffer" in line:
        search_string_left  = "seqnum "
        search_string_right = ", rtptime"

        search_string_left_index  = line.find(search_string_left)
        search_string_right_index = line.find(search_string_right)

        sequence_number = int(line[search_string_left_index+len(search_string_left):search_string_right_index])
        continue

      if "handle NAL type" in line:

        timestamp = line[:17]
        search_string = "handle NAL type "
        
        nal_type_index = line.find(search_string)
        nal_type  = int(line[nal_type_index+len(search_string)])

        datetime_format = datetime.strptime(timestamp[:-3], '%H:%M:%S.%f')
        nal_timestamp_in_milliseconds = datetime_format.timestamp() * 1000

        nal_info.append([sequence_number, nal_type, nal_timestamp_in_milliseconds])
    
    nal_info = np.asarray(nal_info)

    return nal_info

# calculates difference in time between nal units arriving at the rtp payloader on server side, and the nal units leaving the rtp depayloader on the client side.
# If not all of the fragments which make up a nal unit arrive, that unit is marked with a -1 time difference
def calculate_nal_time_diffs(nal_arrival, nal_depature):
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

  return calculate_nal_time_diffs(nal_arrival=nal_arrival, nal_depature=nal_depature)


def convert_nal_results_to_panda(results):
  return pd.DataFrame(data=results, columns=["Packet Num.", "Nal Type", "App latency"])
  
def save_nal_results(time_diff_panda, directory):
  latency_file_path     = os.path.join(directory, "Nal_latency.csv")
  time_diff_panda.to_csv(latency_file_path, index=False)


# Format of frame arrival, frame type identification, and sequence number identification log lines on server side
'''
0:00:00.046454373 [335m474388[00m 0x555b6482ab00 [37mDEBUG  [00m [00m        videoencoder gstvideoencoder.c:2527:gst_video_encoder_get_frame:<x264enc0>[00m frame_number : 0
0:00:00.046478207 [335m474388[00m 0x555b6482ab00 [37mDEBUG  [00m [00m             x264enc gstx264enc.c:2589:gst_x264_enc_encode_frame:<x264enc0>[00m Output keyframe
0:00:00.082437788 [332m1861129[00m 0x55e57568fb00 [33;01mLOG    [00m [00m        videoencoder gstvideoencoder.c:2097:gst_video_encoder_finish_frame:<x264enc0>[00m frame PTS 1000:00:00.000000000, DTS 1000:00:00.000000000
'''

# Format of sequence number identification log line and complete frame identification log line on client side
'''
0:00:00.167648435 [333m1861130[00m 0x564ef2280800 [33;01mLOG    [00m [00m               libav gstavviddec.c:1785:gst_ffmpegviddec_handle_frame:<avdec_h264-0>[00m Received new data of size 103, dts 0:00:00.119534400, pts:0:00:00.078700000, dur:0:00:00.041666666
0:00:00.169163804 [333m1861130[00m 0x564ef2280800 [37mDEBUG  [00m [00m               libav gstavviddec.c:1687:gst_ffmpegviddec_video_frame:<avdec_h264-0>[00m return flow ok, got frame: 1
'''




# Iterates through each line in the server side logs. 
# For frames we collect the arrival (at the encoder) timestamps and the type (keyframe?)
# We also record the PTS so that we can match the frame to the decoded frame message in the client logs
def extract_frame_data_from_server_logs(filename):
  with open(filename, 'r') as log_file:

    frame_info = []
    frame_timestamp_in_milliseconds = -1
    frame_number = -1
    frame_type = -1
    frame_pts = ""

    for line in log_file:

      if "frame_number :" in line:
        timestamp = line[:17]
        datetime_format = datetime.strptime(timestamp[:-3], '%H:%M:%S.%f')
        frame_timestamp_in_milliseconds = datetime_format.timestamp() * 1000

        search_string_left  = "frame_number : "
        search_string_left_index = line.find(search_string_left)

        frame_number = int(line[search_string_left_index+len(search_string_left):])

      elif "Output keyframe" in line:
        frame_type = 1
      
      elif "frame PTS" in line:

        

        search_string_left  = "frame PTS "
        search_string_right = ", DTS "
        search_string_left_index  = line.find(search_string_left)
        search_string_right_index = line.find(search_string_right)

        frame_pts = line[search_string_left_index+len(search_string_left):search_string_right_index]
        frame_info.append([frame_number, frame_type, frame_pts, frame_timestamp_in_milliseconds])
        frame_type = 0


    return frame_info

# Iterates through each line in the client side logs. 
# Records the pts of each frame that was successfully decoded ("return flow ok, got frame: 1" appears in the logs)
# While each element runs on a seperate thread, we can rely on the fact that the decoder logs will be in order
def extract_frame_data_from_client_logs(filename):
  with open(filename, 'r') as log_file:

    frame_info = []
    frame_timestamp_in_milliseconds = -1
    frame_pts = ""

    for line in log_file:

      if "Received new data of size" in line:

        search_string_left  = ", pts:"
        search_string_right = ", dur:"
        search_string_left_index  = line.find(search_string_left)
        search_string_right_index = line.find(search_string_right)

        frame_pts = line[search_string_left_index+len(search_string_left):search_string_right_index]


      elif "return flow ok, got frame: 1" in line:
        timestamp = line[:17]
        datetime_format = datetime.strptime(timestamp[:-3], '%H:%M:%S.%f')
        frame_timestamp_in_milliseconds = datetime_format.timestamp() * 1000
        frame_info.append([frame_pts, frame_timestamp_in_milliseconds])

    return frame_info

# calculates difference in time between frames arriving at the encoder on server side, and the frames leaving the decoder on the client side.
# If a frame is not successfully decoded, it is marked with a -1
def calculate_frame_time_diffs(frame_arrival, frame_depature):
  depature_count = 0
  result = np.zeros((len(frame_arrival),3))

  for i, row in enumerate(frame_arrival):
    frame_number            = row[0]
    frame_type              = row[1]
    frame_base_pts          = row[2]
    frame_arrival_timestamp = row[3]

    frame_base_pts_in_milliseconds = (datetime.strptime(frame_base_pts[5:-3], '%M:%S.%f')).timestamp() * 1000


    result[i,0] = frame_number
    result[i,1] = frame_type

    if depature_count < len(frame_depature):
      frame_decode_pts         = frame_depature[depature_count][0]
      frame_depature_timestamp = frame_depature[depature_count][1]
      frame_decode_pts_in_milliseconds = ((datetime.strptime(frame_decode_pts[2:-3], '%M:%S.%f')).timestamp() * 1000)+0.1 #PTS within client side logs has been reduced by 1 microsecond, not sure why this happens.


    if depature_count < len(frame_depature) and frame_decode_pts_in_milliseconds >= frame_base_pts_in_milliseconds and frame_decode_pts_in_milliseconds < (frame_base_pts_in_milliseconds + 1000/24):
      result[i,2] = frame_depature_timestamp - frame_arrival_timestamp 
      depature_count+=1
    else:
      result[i,2] = -1
  
  return result


def extract_frame_data(directory):
  server_log = os.path.join(directory, "gst-server.log")
  client_log = os.path.join(directory, "gst-client.log")


  frame_arrival  = extract_frame_data_from_server_logs(server_log)
  frame_depature = extract_frame_data_from_client_logs(client_log)

  return calculate_frame_time_diffs(frame_arrival=frame_arrival, frame_depature=frame_depature)

# calculate_time_diffs will provide a -1 timestamp to any frames which could not be decoded.
# We can then identify which frames are useful (Any complete P-frames following a missing I-frame are useless)
def identify_useful_frames(frame_time_diff):
  last_i_frame_was_complete = False
  total_frames           = 0
  useful_frames          = 0
  complete_frames        = 0
  total_p_frame_count    = 0
  total_i_frame_count    = 0
  complete_i_frame_count = 0
  complete_p_frame_count = 0
  useful_i_frame_count   = 0
  useful_p_frame_count   = 0
  ratio_complete_frames  = 0
  ratio_useful_frames    = 0
  

  last_i_frame_was_complete = False


  for row in frame_time_diff:
    total_frames+=1
    frame_type = row[1]
    successfully_decoded = (int(row[2]) != -1)

    if frame_type == 1:
      total_i_frame_count+=1
      if successfully_decoded:
        last_i_frame_was_complete = True
        complete_i_frame_count+=1
        complete_frames+=1
        useful_i_frame_count+=1
        useful_frames+=1
      else:
        last_i_frame_was_complete = False

    elif frame_type == 0:
      total_p_frame_count+=1
      if successfully_decoded:
        complete_p_frame_count+=1
        complete_frames+=1
        if last_i_frame_was_complete:
          useful_p_frame_count+=1
          useful_frames+=1

  ratio_complete_frames = ((complete_i_frame_count+complete_p_frame_count)/total_frames)*100
  ratio_useful_frames = ((useful_i_frame_count+useful_p_frame_count)/total_frames)*100

  if (total_p_frame_count + total_i_frame_count) != total_frames:
    print("Total P-frames + Total I-frames does not equal total frames")
    raise ValueError
  elif (complete_p_frame_count + complete_i_frame_count) != complete_frames:
    print("Complete P-frames + Complete I-frames does not equal Complete frames")
    raise ValueError
  elif (useful_p_frame_count + useful_i_frame_count) != useful_frames:
    print("Useful P-frames + Useful I-frames does not equal Useful frames")
    raise ValueError
  

  results = {
  'Total Frames': [total_frames],
  'Complete Frames': [complete_frames],
  'Useful Frames': [useful_frames],
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


def convert_frame_results_to_panda(results):
  return pd.DataFrame(data=results, columns=["Frame Num.", "KeyFrame?", "App latency"])
  
def save_frame_results(time_diff_panda, frame_stats_panda, directory):
  latency_file_path     = os.path.join(directory, "Frame_latency.csv")
  frame_stats_file_path = os.path.join(directory, "Frame_stats.csv")
  time_diff_panda.to_csv(latency_file_path, index=False)
  frame_stats_panda.to_csv(frame_stats_file_path, index=False)




# string  = "0:00:17.814566771 [336m1507487[00m 0x55806ef5c520 [37mDEBUG  [00m [00m          rtph264pay gstrtph264pay.c:821:gst_rtp_h264_pay_payload_nal:<rtph264pay0>[00m Processing Buffer with NAL TYPE=1 0:00:17.720000000"
# string2 = "0:00:00.175037545 [335m 3454[00m 0x55b7aa9e4120 [37mDEBUG  [00m [00m          rtph264pay gstrtph264pay.c:821:gst_rtp_h264_pay_payload_nal:<rtph264pay0>[00m Processing Buffer with NAL TYPE=7 0:00:00.000000000"

# line_contents = str.split(string)

# print(int(line_contents[12][-1]))

# line_contents2 = str.split(string2)

# print(line_contents2[0] + "     " + str(int(line_contents2[11])))


