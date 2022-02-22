from datetime import datetime
from re import U
import time
import os
import pandas as pd

import numpy as np

def convert_tcp_capture(directory):
  server_rtp_seq_num_file = os.path.join(directory, "server_side_timestamps.txt")
  client_rtp_capture_file = os.path.join(directory, "client_side_timestamps.txt")

  # We have an issue with the tcp packet capture on the client side. After any packet reordering
  # (either due to jitter or loss) the rtp subdissector appears to fail. This seems to be a known
  # issue but there is no fix as of yet. As a result, we must extract the rtp sequence numbers
  # directly from the tcp stream, similar as we did for quic. The server side doesnt experience
  # any loss or packet reordering so tshark can do the work for us in that case.
  packet_depature     = extract_rtp_data_from_datagram(server_rtp_seq_num_file, decode=False)
  packet_arrival      = extract_rtp_data_from_stream_arrival(client_rtp_capture_file)
  packet_available    = extract_rtp_data_from_stream_available(client_rtp_capture_file)
  time_diff_arrival   = calculate_time_diffs(packet_depature, packet_arrival)
  time_diff_available = calculate_time_diffs(packet_depature, packet_available)

  return time_diff_arrival, time_diff_available



def convert_udp_capture(directory):
  server_rtp_seq_num_file = os.path.join(directory, "server_side_timestamps.txt")
  client_rtp_seq_num_file = os.path.join(directory, "client_side_timestamps.txt")

  packet_depature = extract_rtp_data_from_datagram(server_rtp_seq_num_file, decode=False)
  packet_arrival  = extract_rtp_data_from_datagram(client_rtp_seq_num_file, decode=False)
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

  packet_depature = extract_rtp_data_from_datagram(server_rtp_seq_num_file, decode=True)
  packet_arrival  = extract_rtp_data_from_datagram(client_rtp_seq_num_file, decode=True)
  time_diff = calculate_time_diffs(packet_depature, packet_arrival)

  return time_diff

def convert_bit_string_to_rtp_packet_number(bitstring):
  if "8060" == bitstring[0:4] or "80e0" == bitstring[0:4]:
    packet_number = bitstring[4:8]
    return int(packet_number, 16)
  else:
    return -1

def does_new_data_fit_in_existing_chunk(stream, new_data):
  for i, stream_chunk in enumerate(stream):
    if new_data[0] == stream_chunk[0] + new_data[1]:
      # Fits, update values
      stream_chunk[0] =  new_data[0]
      stream_chunk[1] += new_data[1]
      stream_chunk[2] += new_data[2]
      try:
        next_stream_chunk = stream[i+1]
        if next_stream_chunk[0] == new_data[0] + next_stream_chunk[1]:
                # Missing piece has been found, combine chunks
                next_stream_chunk = stream.pop(i+1)
                stream_chunk[0] =  next_stream_chunk[0]
                stream_chunk[1] += next_stream_chunk[1]
                stream_chunk[2] += next_stream_chunk[2]
      finally:
        return True
  else:
    return False


def extract_rtp_data_from_datagram(filename, decode):
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
              if packet_number > -1:
                packet_info.append([packet_number, time_in_milliseconds])
          else:
            for packet in str.split(line_contents[5], ','):

              packet_number = int(packet)
              packet_info.append([packet_number, time_in_milliseconds])


        
    packet_info = np.asarray(packet_info)


    # We are only interested in the first entry for each rtp packet
    ignore, unique_indices = np.unique(packet_info[:,0], return_index=True)
    packet_info = packet_info[unique_indices,:]

    # Sort by packet sequence numbers
    packet_info = packet_info[np.argsort(packet_info[:, 0])]

    return packet_info

def extract_rtp_data_from_stream_available(filename):
  with open(filename, 'r') as rtp_file:
    Lines = rtp_file.readlines()

    packet_info = []

    # We have no guarentee that packets will arrive in order
    # So we keep a list of list representing the chunks of the stream.
    # When the missing packets arrive these can be combined.
    # The first value is the last seqnum received for this chunk
    # The second is the amount of data associated with this chunck
    # The third is payload data for this chunk
    # For the first chunk, the length of the payload data may be less than the amount of data associated with the chunk
    # This is becuase we will process data from the first chunck where possible to reduce memory load
    stream = [[1,0,""]]
    first_stream_chunk = stream[0]
    data_available = False


    for i, line in enumerate(Lines):
      line_contents = str.split(line)
      if len(line_contents) >= 7:
        datetime_format = datetime.strptime(line_contents[3][:-3], '%H:%M:%S.%f')
        time_in_milliseconds = datetime_format.timestamp() * 1000

        seq_num = int(line_contents[5])

        # payload is displayed in hex so the size in bytes is half the length
        payload = line_contents[6]
        payload_length = len(payload)/2


        if (payload_length + first_stream_chunk[0]) == seq_num:
          first_stream_chunk[0] = seq_num
          first_stream_chunk[1] += payload_length
          first_stream_chunk[2] += payload
          data_available = True


          if (len(stream)>1):
            next_stream_chunk = stream[1]
            if next_stream_chunk[0] == seq_num + next_stream_chunk[1]:
              # Missing piece has been found, combine chunks
              next_stream_chunk = stream.pop(1)
              first_stream_chunk[0] =  next_stream_chunk[0]
              first_stream_chunk[1] += next_stream_chunk[1]
              first_stream_chunk[2] += next_stream_chunk[2]
            else:
              # We are still missing data (i.e. there was more than one packet lost)
              pass
              
          else:
            # Currently there is no data missing
            pass

        else:
          data_available = False
          new_data = [seq_num, payload_length, payload]
          if not (does_new_data_fit_in_existing_chunk(stream, new_data)):
            # Need a new stream chunk
            stream.append(new_data)




        if data_available:
          
          #- Each character in the hex string represents 4 bits (0.5 bytes)
          #- Each rtp packet in the stream begins with 4 hex characters (2 bytes) denoting the packet length
          #- Thus we can not process a packet until we have buffered a string of length 4 + (length_in_bytes*2)
          stream_buffer = first_stream_chunk[2]
          next_rtp_packet_length   = int(stream_buffer[:4],16)
          min_buffer_length = 4 + (next_rtp_packet_length*2)
          while (min_buffer_length) <= len(stream_buffer):
            rtp_packet = stream_buffer[4:min_buffer_length]

            packet_number = convert_bit_string_to_rtp_packet_number(rtp_packet)
            if packet_number > -1:
              packet_info.append([packet_number, time_in_milliseconds])


            # remove this packet from buffer
            stream_buffer = stream_buffer[min_buffer_length:]
            first_stream_chunk[2] = stream_buffer

            # There may still be data in the buffer
            try:
              next_rtp_packet_length   = int(stream_buffer[:4],16)
              min_buffer_length = 4 + (next_rtp_packet_length*2)
            except:
              break
            
          

    packet_info = np.asarray(packet_info)  

    # We are only interested in the first entry for each rtp packet
    ignore, unique_indices = np.unique(packet_info[:,0], return_index=True)
    packet_info = packet_info[unique_indices,:]

    # Sort by packet sequence numbers
    packet_info = packet_info[np.argsort(packet_info[:, 0])]

    return packet_info


def extract_rtp_data_from_stream_arrival(filename):
  with open(filename, 'r') as rtp_file:
    Lines = rtp_file.readlines()

    packet_info = []
    stream_info = []
    payloads    = []

    # We have no guarentee that packets will arrive in order
    # So we keep a list of the timestamps, tcp sequence numbers
    # and payloads. We then sort by the sequence number and iterate
    # though each list element, extracting the rtp sequence numbers
    # as we go. This is slow but I cannot think of a better way to 
    # extract the arrival time


    for i, line in enumerate(Lines):
      line_contents = str.split(line)
      if len(line_contents) >= 7:
        datetime_format = datetime.strptime(line_contents[3][:-3], '%H:%M:%S.%f')
        time_in_milliseconds = datetime_format.timestamp() * 1000

        seq_num = int(line_contents[5])
        payload = line_contents[6]

        stream_info.append([seq_num, time_in_milliseconds])
        payloads.append([payload])

    stream_info = np.asarray(stream_info)
    payloads = np.asarray(payloads)  

    # We are only interested in the first entry for each tcp packet (in case of duplicate packets arriving)
    ignore, unique_indices = np.unique(stream_info[:,0], return_index=True)
    stream_info = stream_info[unique_indices,:]
    payloads    = payloads[unique_indices,:]

    # Sort by packet sequence numbers
    sort_indices = np.argsort(stream_info[:, 0])

    stream_info = stream_info[sort_indices]
    payloads    = payloads[sort_indices]

    payload_buffer = ""

    for i, payload in enumerate(payloads):
      time_in_milliseconds = stream_info[i,1]
      payload_buffer += payload[0]

      next_rtp_packet_length   = int(payload_buffer[:4],16)
      min_buffer_length = 4 + (next_rtp_packet_length*2)

      while (min_buffer_length) <= len(payload_buffer):
        rtp_packet = payload_buffer[4:min_buffer_length]
        packet_number = convert_bit_string_to_rtp_packet_number(rtp_packet)

        if packet_number > -1:
          packet_info.append([packet_number, time_in_milliseconds])

        # remove this packet from buffer
        payload_buffer = payload_buffer[min_buffer_length:]

        # There may still be data in the buffer
        try:
          next_rtp_packet_length   = int(payload_buffer[:4],16)
          min_buffer_length = 4 + (next_rtp_packet_length*2)
        except:
          break
      

    packet_info = np.asarray(packet_info)  

    # We are only interested in the first entry for each rtp packet
    ignore, unique_indices = np.unique(packet_info[:,0], return_index=True)
    packet_info = packet_info[unique_indices,:]

    # Sort by packet sequence numbers
    packet_info = packet_info[np.argsort(packet_info[:, 0])]

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
  file_path = os.path.join(directory, "Stack_Latency.csv")
  time_diff_panda.to_csv(file_path, index=False)

# TCP has two different stack latency csv's to save
def save_results_tcp(arrival_time_diff_panda, available_time_diff_panda, directory):
  file_path_available = os.path.join(directory, "Stack_Latency_available.csv")
  file_path_arrival   = os.path.join(directory, "Stack_Latency_arrival.csv")

  available_time_diff_panda.to_csv(file_path_available, index=False)
  arrival_time_diff_panda.to_csv(file_path_arrival, index=False)


def load_results(directory):
  file_path = os.path.join(directory, "Stack_Latency.csv")
  return pd.read_csv(file_path)


