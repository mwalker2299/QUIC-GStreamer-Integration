from datetime import datetime
from re import U
import time
import os
import pandas as pd

import numpy as np


def convert_udp_capture(directory):
  server_rtp_seq_num_file = os.path.join(directory, "server_side_timestamps.txt")
  client_rtp_seq_num_file = os.path.join(directory, "client_side_timestamps.txt")

  packet_depature = extract_rtp_data(server_rtp_seq_num_file, decode=False)
  packet_arrival  = extract_rtp_data(client_rtp_seq_num_file, decode=False)
  time_diff = calculate_time_diffs(packet_depature, packet_arrival)

  return time_diff

def convert_quic_capture(directory):
  server_pcap = os.path.join(directory, "server_side_tcpdump.pcap")
  client_pcap = os.path.join(directory, "client_side_tcpdump.pcap")
  ssl_keys    = os.path.join(directory, "SSL.keys")

  server_rtp_seq_num_file = os.path.join(directory, "server_side_timestamps.txt")
  client_rtp_seq_num_file = os.path.join(directory, "client_side_timestamps.txt")

  # As we needed to wait for the SSL keys to become availiable, we must decode pcap now
  os.system("tshark -r " + server_pcap + " -T fields -e frame.time -e quic.stream_data -o \"tls.keylog_file:"+ssl_keys+"\" > " + server_rtp_seq_num_file)
  os.system("tshark -r " + client_pcap + " -T fields -e frame.time -e quic.stream_data -o \"tls.keylog_file:"+ssl_keys+"\" > " + client_rtp_seq_num_file)

  packet_depature = extract_rtp_data(server_rtp_seq_num_file, decode=True)
  packet_arrival  = extract_rtp_data(client_rtp_seq_num_file, decode=True)
  time_diff = calculate_time_diffs(packet_depature, packet_arrival)

  return time_diff

def convert_bit_string_to_rtp_packet_number(bitstring):
  if "80" == bitstring[0:2]:
    packet_number = bitstring[4:8]
    return int(packet_number, 16)
  else:
    return False


def extract_rtp_data(filename, decode):
  print(filename)
  with open(filename, 'r') as rtp_file:
    Lines = rtp_file.readlines()
    
    packet_info = []

    for line in Lines:
      if "<MISSING>" not in line:
        line_contents = str.split(line)
        if len(line_contents) >= 6:
          datetime_format = datetime.strptime(line_contents[3][:-3], '%H:%M:%S.%f')
          time_in_milliseconds = datetime_format.timestamp() * 1000

          if decode:
            for bitstring in str.split(line_contents[5], ','):

              packet_number = convert_bit_string_to_rtp_packet_number(bitstring)
              if packet_number:
                packet_info.append([packet_number, time_in_milliseconds])
          else:
            packet_number = int(line_contents[5])
            packet_info.append([packet_number, time_in_milliseconds])


        
    packet_info = np.asarray(packet_info)


    # We are only interested in the first entry for each rtp packet
    ignore, unique_indices = np.unique(packet_info[:,0], return_index=True)
    packet_info = packet_info[unique_indices,:]

    # Sort by packet sequence numbers
    packet_info = packet_info[np.argsort(packet_info[:, 0])]

    print(np.asarray(packet_info).shape)
    return packet_info

# calculates diff in initial send time and arrival time for each packet.
# If a packet did not arrive at client side, it is marked with a time diff of -1
def calculate_time_diffs(packet_depature, packet_arrival):
  arrival_count = 0
  first_packet_seq = packet_depature[0,0]
  result = np.zeros(packet_depature.shape)

  for i, row in enumerate(packet_depature):
    packet_number = first_packet_seq +i
    result[i,0] = packet_number
    if arrival_count < len(packet_arrival) and packet_arrival[arrival_count,0] == packet_number:
      result[i,1] = packet_arrival[arrival_count,1] - row[1] 
      arrival_count+=1
    else:
      result[i,1] = -1
  
  return result

def convert_results_to_panda(results):
  return pd.DataFrame(data=results, columns=["Packet Num.", "Stack Latency"])

def save_results(time_diff_panda, directory):
  file_path = os.path.join(directory, "time_diff.csv")
  time_diff_panda.to_csv(file_path, index=False)


def load_results(directory):
  file_path = os.path.join(directory, "time_diff.csv")
  return pd.read_csv(file_path)



# packet_depature = extract_rtp_data('server_side_tcpdump.pcap')
# packet_arrival  = extract_rtp_data('client_side_tcpdump.pcap')
# time_diff = calculate_time_diffs(packet_depature, packet_arrival)

      
# apple = np.asarray([[105,0],
#            [206,1],
#            [234,2],
#            [397,3],
#            [386,4],
#            [463,5]])


# banana = np.asarray([[107,0],
#            [209,1],
#            [401,3],
#            [391,4]])


# result = np.zeros(apple.shape)


# print(result)
