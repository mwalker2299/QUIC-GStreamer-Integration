
#/home/matt/Documents/QUIC-GStreamer-Integration/src/mininet_tests/1400_MTU_Packet_Sizes.txt
#/home/matt/Documents/QUIC-GStreamer-Integration/src/mininet_tests/1398_MTU_Packet_Sizes.txt
from cProfile import label
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
from pyparsing import col

def find_value_from_string(string,left_search_string,right_search_string):
  if len(left_search_string) > 0:
    left_search_string_index  = string.find(left_search_string)+len(left_search_string)
  else:
    left_search_string_index = 0
  right_search_string_index = string.find(right_search_string)

  # print(left_search_string_index, right_search_string_index)

  return string[left_search_string_index:right_search_string_index]


def plot_stack_latency_available(directory, csv_filename):
  csv_path = os.path.join(directory, csv_filename)

  stack_latency_available_pd = pd.read_csv(csv_path)
  stack_latency_available_np = pd.DataFrame.to_numpy(stack_latency_available_pd)

  latencies = stack_latency_available_np[14000:14500, 1]
  loss_event_latencies = []
  loss_event_packets   = []

  packet_numbers = stack_latency_available_np[14000:14500, 0]

  prev_value = 0
  for i in np.arange(len(latencies)):
    if prev_value != 0:
      if prev_value+1 < latencies[i]:
        loss_event_latencies.append(latencies[i])
        loss_event_packets.append(packet_numbers[i])
    prev_value = latencies[i]

  frames = [14003,14012,14021,14030,14039,14048,14057,14066,14075,14084,14093,14145,14154,14163,14172,14181,14190,14199,14208,14217,14226,14235,14244,14253,14262,14271,14324,14333,14342,14351,14360,14369,14378,14387,14396,14405,14414,14423,14432,14441,14450]
  gops   = [14093,14271,14450]

  font = {'family' : 'normal',
        'weight' : 'bold',
        'size'   : 12}

  plt.rc('font', **font)



  delay=find_value_from_string(str(directory), "Delay_", "ms/Loss")
  loss=find_value_from_string(str(directory), "Loss_", "%")
  protocol=find_value_from_string(str(directory), "results/", "/Delay")

  plt.ylim(0,1000)
  plt.plot(packet_numbers, latencies, '.', markersize=3.0, color='orange', alpha=1)
  plt.plot(loss_event_packets, loss_event_latencies, '.', markersize=6.0, color='red', alpha=1 )
  for frame in frames:
    plt.axvline(x=frame, color='green', linewidth=0.4, alpha=1)
  for gop in gops:
    plt.axvline(x=gop, color='blue', linewidth=0.4, alpha=1)

  # plt.xticks([0,2000,4000,6000,8000,10000,12000,14000,len(packet_numbers)-1])
  # plt.title('Stack Latency (Availability) - Protocol: '+str(protocol)+', Delay: '+ str(delay)+ 'ms, Loss: '+ str(loss)+'%')
  plt.xlabel('Packet Number', fontsize=16)
  plt.ylabel('Latency (ms)', fontsize=16)
  fig = plt.gcf()
  fig.savefig(os.path.join(directory, protocol+"_Stack_Latency_Available.png"))
  plt.clf()
  plt.close()


def plot_total_HOL_blocking_delay(path_to_totals):

  try:
    os.path.exists(path_to_totals)
  except:
    print("Plotting failed! Path to totals: " + path_to_totals + " does not exist")
    return 1

  for dirpath, dirnames, filenames in os.walk(path_to_totals):
    if dirnames:
      continue

    if   "QUIC_PPS" in dirpath:
      protocol = "QUIC_PPS"
    elif "QUIC_SS" in dirpath:
      protocol = "QUIC_SS"
    elif "TCP" in dirpath:
      protocol = "TCP"
    elif "QUIC_FPS" in dirpath:
      protocol = "QUIC_FPS"
    elif "QUIC_GOP" in dirpath:
      protocol = "QUIC_GOP"
    else:
      protocol = "UDP"

    graph_data = {}

    for file in filenames:
      if ".png" in file or "Total" not in file:
        continue
      delay=int(find_value_from_string(str(file), "Delay_", "ms_"))
      loss="Loss: "+find_value_from_string(str(file), "_Loss_", "%_")+"%"
      print(str(dirpath))
      print(delay)
      print(loss)
    

      if loss not in graph_data:
        graph_data[loss] = [[],[]]

      total_HOL_delay_pd = pd.read_csv(os.path.join(dirpath,file))
      total_HOL_delay_np = pd.DataFrame.to_numpy(total_HOL_delay_pd)
      total_value = int(total_HOL_delay_np)

      graph_data[loss][0].append(delay)
      graph_data[loss][1].append(total_value)


    loss_values = graph_data.keys()
    
    ind = np.arange(5)
    bars = []
    width = 0.15
    for i,loss in enumerate(loss_values):
      i-=2
      bars.append(plt.bar(ind+(width*i), graph_data[loss][1], width))
      
    
    font = {'family' : 'normal',
        'weight' : 'bold',
        'size'   : 12}

    plt.rc('font', **font)
      
    plt.ylim(0,7500000)
    plt.xlabel('Propagation Delay (ms)', fontsize=16)
    plt.ylabel('Total HOL Blocking Delay (ms)', fontsize=16)
    plt.legend(list(loss_values))
    plt.xticks(ind, labels=['10', '30', '50', '100', '150'])
    fig = plt.gcf()
    fig.savefig(os.path.join(dirpath, protocol+"_Total_HOL_delay.png"),bbox_inches='tight')
    plt.clf()
    plt.close()
    

def plot_total_send_delay(path_to_totals):

  try:
    os.path.exists(path_to_totals)
  except:
    print("Plotting failed! Path to totals: " + path_to_totals + " does not exist")
    return 1

  for dirpath, dirnames, filenames in os.walk(path_to_totals):
    if dirnames:
      continue

    if   "QUIC_PPS" in dirpath:
      protocol = "QUIC_PPS"
    elif "QUIC_SS" in dirpath:
      protocol = "QUIC_SS"
    elif "TCP" in dirpath:
      protocol = "TCP"
    elif "QUIC_FPS" in dirpath:
      protocol = "QUIC_FPS"
    elif "QUIC_GOP" in dirpath:
      protocol = "QUIC_GOP"
    else:
      protocol = "UDP"

    graph_data = {}

    for file in filenames:
      if ".png" in file or "Total" not in file:
        continue
      delay=int(find_value_from_string(str(file), "Delay_", "ms_"))
      loss="Loss: "+find_value_from_string(str(file), "_Loss_", "%_")+"%"
      print(str(dirpath) + str(dirnames))
      print(delay)
      print(loss)
    

      if loss not in graph_data:
        graph_data[loss] = [[],[]]

      total_send_delay_pd = pd.read_csv(os.path.join(dirpath,file))
      total_send_delay_np = pd.DataFrame.to_numpy(total_send_delay_pd)
      total_value = int(total_send_delay_np)

      graph_data[loss][0].append(delay)
      graph_data[loss][1].append(total_value)


    loss_values = graph_data.keys()
    
    ind = np.arange(5)
    bars = []
    width = 0.15
    for i,loss in enumerate(loss_values):
      i-=2
      bars.append(plt.bar(ind+(width*i), graph_data[loss][1], width))
      
    
    font = {'family' : 'normal',
        'weight' : 'bold',
        'size'   : 12}

    plt.rc('font', **font)
      
    plt.ylim(0,1200000)
    plt.xlabel('Propagation Delay (ms)', fontsize=16)
    plt.ylabel('Total Send Delay (ms)', fontsize=16)
    plt.legend(list(loss_values))
    plt.xticks(ind, labels=['10', '30', '50', '100', '150'])
    fig = plt.gcf()
    fig.savefig(os.path.join(dirpath, protocol+"_Total_Send_delay.png"),bbox_inches='tight')
    plt.clf()
    plt.close()



def plot_frame_usefulness(output_path, frame_usefulness_dictionary, loss_params, propagation_delay_params, buffer_delay):

  num_loss_params = len(loss_params)
  num_propogation_delay_params = len(propagation_delay_params)

  # reverse loss for better visualisation
  loss_params = np.flip(loss_params)

  propagation_delay_params_string = []
  for propagation_delay_param in propagation_delay_params:
    propagation_delay_params_string.append(str(propagation_delay_param))

  loss_params_string = []
  for loss_param in loss_params:
    loss_params_string.append(str(loss_param))





  useful_frame_values = []
  for i,loss in enumerate(frame_usefulness_dictionary.keys()):
    useful_frame_values_for_give_loss = []
    for propagation_delay in frame_usefulness_dictionary[loss].keys():
      print(frame_usefulness_dictionary[loss][propagation_delay])
      for useful_frame_ratio in [frame_usefulness_dictionary[loss][propagation_delay][buffer_delay]]:
        useful_frame_values_for_give_loss.append(useful_frame_ratio)
    useful_frame_values.append(useful_frame_values_for_give_loss)

  useful_frame_values = np.flip(useful_frame_values, axis=1)
  useful_frame_values = np.array(useful_frame_values)
  
  useful_frame_values = np.flip(useful_frame_values)

  colors = ['r','b','g','y','b','p']

  font = {'family' : 'normal',
        'weight' : 'bold',
        'size'   : 12}

  plt.rc('font', **font)

  fig=plt.figure(dpi=800)
  ax1=fig.add_subplot(111, projection='3d')
  ax1.set_xlabel('Propagation Delay (ms)', fontsize=16)
  ax1.set_ylabel('Loss (%)', fontsize=16)
  ax1.set_zlabel('Useful Frames (%)', fontsize=16)
  xlabels = np.array(propagation_delay_params_string)
  xpos = np.arange(xlabels.shape[0])
  ylabels = np.array(loss_params_string)
  ypos = np.arange(ylabels.shape[0])

  xposM, yposM = np.meshgrid(xpos, ypos, copy=False)

  zpos=useful_frame_values
  zpos = zpos.ravel()

  dx=0.5
  dy=0.5
  dz=zpos

  ax1.w_xaxis.set_ticks(xpos + dx/2.)
  ax1.w_xaxis.set_ticklabels(xlabels)

  ax1.w_yaxis.set_ticks(ypos + dy/2.)
  ax1.w_yaxis.set_ticklabels(ylabels)

  values = np.linspace(0.2, 1., xposM.ravel().shape[0])
  colors = cm.rainbow(values)
  ax1.bar3d(xposM.ravel(), yposM.ravel(), dz*0, dx, dy, dz, color=colors)
  plt.show()

  fig.savefig(output_path,bbox_inches='tight')
  plt.clf()
  plt.close()