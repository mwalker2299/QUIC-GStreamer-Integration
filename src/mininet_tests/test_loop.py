#!/usr/bin/python

from cgi import test
import os
from time import sleep
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNetConnections
import threading
import network

def streamControlThread(serverNode, cmd, timeout, event, log_path):
  print("Stream Control: " + cmd)
  run_time = 0

  # To account for high loss scenarios where 
  # initial connection attempt fails to reach
  # server, we run the controlling pipeine in background.
  # If the process has terminated within
  # The timeout, then we signal the other
  # threads to end their processes.
  # Otherwise, we allow more time.
  # If after this time, the process still
  # has not completed, we assume a stall occured
  # and kill the process manually
  cmd = cmd + " &"
  serverNode.cmd(cmd)
  cmd_pid = serverNode.cmd("echo $!")

  sleep(timeout)
  run_time += timeout

  cmd_done = serverNode.cmd("kill -0 " + cmd_pid)
  print(cmd_done)

  if "No such process" in cmd_done:
    print("Stream Control: process terminated within timeout.")
    serverNode.cmd("touch " + os.path.join(log_path,"Success"))
  else:
    while (1):

     print("Stream Control: Process still running, allowing more time")
     sleep(timeout/5)
     run_time += timeout/5

     cmd_done = serverNode.cmd("kill -0 " + cmd_pid)
     print(cmd_done)

     if "No such process" in cmd_done:
      print("Stream Control: process terminated after " + str(run_time) + " seconds.")
      serverNode.cmd("touch " + os.path.join(log_path,"Success"))
      break
     elif run_time >= 3*timeout:
       print("Stream Control: Process still running after " + str(run_time) + " seconds, killing process " + cmd_pid + " manually")
       serverNode.cmd('kill ' + cmd_pid)
       serverNode.cmd("touch " + os.path.join(log_path,"Failure"))
       break


  # We set the event when done, allowing the other threads to complete
  event.set()
  print("Stream Server: event set (Done)")

# This thread is used by the node which we wish to kill after the control node has completed.
# For most implementations, we wish to wait for the client to finish execution so that we can 
# get an accurate picture of the time taken to decode all frames. If we killed the client
# when the server is done sending, we may lose some info. The only case where this is not applicable
# is udp, since the client has no concept of connection and thus cant send an eos when the connection closes.
# In the UDP case this is not an issue, as there will be no added delay due to packet loss.
def streamWaitingThread(clientNode, cmd, event):
  print("Stream Slave: starting client")
  # Process should run in background to allow thread to kill process when signaled
  cmd = cmd + " &"
  print("Stream Slave: " + cmd)
  clientNode.cmd(cmd)
  cmd_pid = clientNode.cmd("echo $!")
  print("Stream Slave: Started Process in background, waiting for control thread signal")
  event.wait()

  print("Stream Slave: event acquired, killing process " + cmd_pid)
  clientNode.cmd('kill ' + cmd_pid)

def crossTrafficClientThread(clientNode, cmd, event):
  print("Cross Traffic Client: starting client")
  # Client should run in background to allow thread to kill process when signaled
  cmd = cmd + " &"
  print("Cross Traffic Client: " + cmd)
  clientNode.cmd(cmd)
  cmd_pid = clientNode.cmd("echo $!")
  print("Cross Traffic Client: Started Process in background, waiting for server completing")
  event.wait()

  print("Cross Traffic Client: event acquired, killing process " + cmd_pid)
  clientNode.cmd('kill ' + cmd_pid)

def monitorThread(node, cmd, event):
  print("Monitor: starting monitor")
  # Monitor should run in background to allow thread to kill process when signaled
  cmd = cmd + " &"
  print("Monitor: " + cmd)
  node.cmd(cmd)
  cmd_pid = node.cmd("echo $!")
  print("Monitor: Started Process in background, waiting for server completing")
  event.wait()

  print("Monitor: event acquired, killing process " + cmd_pid)
  node.cmd('kill ' + cmd_pid)






def run_test(test_params, stream_server_command, stream_client_command, ct_command, timeout, protocol_name, log_path, log_level):
  # print(type(log_path))

  cross_traffic_enabled = test_params["cross_traffic"]

  if cross_traffic_enabled:
    num_servers = 2
    num_clients = 2
  else:
    num_servers = 1
    num_clients = 1

  # Create network topology
  net = network.createDumbellTopo(test_params, servers=num_servers, clients=num_clients)
  
  
  # Setup logging paths
  server_gstreamer_log_path    = os.path.join(log_path, "gst-server.log")
  server_lsquic_log_path       = os.path.join(log_path, "lsquic-server.log")
  client_gstreamer_log_path    = os.path.join(log_path, "gst-client.log")
  client_lsquic_log_path       = os.path.join(log_path, "lsquic-client.log")
  server_keylog_path           = os.path.join(log_path, "SSL.keys")
  



  # Start network and retrieve nodes
  net.start()
  print( "Dumping network connections" )
  dumpNetConnections( net )
  stream_server = net.get('server1')
  stream_client = net.get('client1')
  if cross_traffic_enabled:
    cross_traffic_client = net.get('client2')
    cross_traffic_server = net.get('server2')

  # Due to mininet restrictions on names, I am restricted to naming the switches s1, s2 rather than something more descriptive
  client_side_switch = net.get('s2') 
  server_side_switch = net.get('s1')

  # Substitute stream_server address and lsquic log location into stream_client and stream_server commands
  # UDP requires that stream_client and stream_server are given the stream_clients address, while QUIC is the opposite
  if "UDP" in protocol_name:
    print("PROTOCOL IS UDP")
    client_side_tshark_command = "tshark -T fields -e frame.time -e rtp.seq -i s2-eth1 > " + os.path.join(log_path, "client_side_timestamps.txt")
    server_side_tshark_command = "tshark -T fields -e frame.time -e rtp.seq -i s1-eth1 > " + os.path.join(log_path, "server_side_timestamps.txt")

    stream_server_command = stream_server_command.format(addr=stream_client.IP())
    stream_client_command = stream_client_command.format(addr=stream_client.IP(), buffer_delay=test_params["buffer_delay"])
  elif "QUIC" in protocol_name:
    print("PROTOCOL IS QUIC")
    client_side_tshark_command = "tshark -d udp.port==5000,quic -T fields -e frame.time -e quic.stream.stream_id -e quic.stream.offset -e quic.stream.fin -e quic.stream_data -o \"tls.keylog_file:"+os.path.join(log_path, "SSL.keys")+"\" -i s2-eth1 > " + os.path.join(log_path, "client_side_timestamps.txt")
    server_side_tshark_command = "tshark -d udp.port==5000,quic -T fields -e frame.time -e quic.stream.stream_id -e quic.stream.offset -e quic.stream.fin -e quic.stream_data -o \"tls.keylog_file:"+os.path.join(log_path, "SSL.keys")+"\" -i s1-eth1 > " + os.path.join(log_path, "server_side_timestamps.txt")

    stream_server_command = stream_server_command.format(addr=stream_server.IP(), lsquic_log=server_lsquic_log_path, keylog=server_keylog_path)
    stream_client_command = stream_client_command.format(addr=stream_server.IP(), lsquic_log=client_lsquic_log_path, buffer_delay=test_params["buffer_delay"])
  else:
    print("PROTOCOL IS TCP")
    client_side_tshark_command = "tshark -d tcp.port==5000,rtp -T fields -e frame.time -e tcp.nxtseq -e tcp.payload -i s2-eth1 -o tcp.reassemble_out_of_order:TRUE > " + os.path.join(log_path, "client_side_timestamps.txt")
    server_side_tshark_command = "tshark -d tcp.port==5000,rtp -T fields -e frame.time -e tcp.nxtseq -e tcp.payload -i s1-eth1 -o tcp.reassemble_out_of_order:TRUE > " + os.path.join(log_path, "server_side_timestamps.txt")

    stream_server_command = stream_server_command.format(addr=stream_server.IP(), lsquic_log=server_lsquic_log_path, keylog=server_keylog_path)
    stream_client_command = stream_client_command.format(addr=stream_server.IP(), lsquic_log=client_lsquic_log_path, buffer_delay=test_params["buffer_delay"])

  if cross_traffic_enabled:
    ct_command = ct_command.format(addr=cross_traffic_server.IP())

  # Set up gstreamer logging on stream_server and stream_client
  stream_server.cmd('export GST_DEBUG=' + log_level)
  stream_server.cmd('export GST_DEBUG_FILE=' + server_gstreamer_log_path)

  stream_client.cmd('export GST_DEBUG=' + log_level)
  stream_client.cmd('export GST_DEBUG_FILE=' + client_gstreamer_log_path)

  # Create event for synchronising threads. 
  # The clients will not stop on their own, and need to be killed once 
  # the stream_server completes its transmission
  event = threading.Event()

  # Create threads for stream_server node, stream_client node and client-side switch node (for tshark)
  if "UDP" in protocol_name:
    stream_server_thread  = threading.Thread(target=streamControlThread, args=(stream_server,stream_server_command,timeout,event,log_path))
    stream_client_thread  = threading.Thread(target=streamWaitingThread, args=(stream_client,stream_client_command,event))
  else:
    stream_server_thread  = threading.Thread(target=streamWaitingThread, args=(stream_server,stream_server_command,event))
    stream_client_thread  = threading.Thread(target=streamControlThread, args=(stream_client,stream_client_command,timeout,event,log_path))

  if cross_traffic_enabled:
    ct_client_thread =  threading.Thread(target=crossTrafficClientThread, args=(cross_traffic_client,ct_command,event))

  client_side_monitor_thread = threading.Thread(target=monitorThread, args=(client_side_switch,client_side_tshark_command,event))
  server_side_monitor_thread = threading.Thread(target=monitorThread, args=(server_side_switch,server_side_tshark_command,event))

  # Start threads, wait for thread completion and then stop the network
  # We start the monitor threads first and sleep for 1 second so that tshark
  # Does not miss any packets
  client_side_monitor_thread.start()
  server_side_monitor_thread.start()
  sleep(1)

  udpBw = str(test_params['bandwidth']) + 'M'
  net.iperf( ( stream_server, stream_client ), l4Type='UDP', udpBw=udpBw )

  stream_server_thread.start()
  stream_client_thread.start()

  if cross_traffic_enabled:
    ct_client_thread.start()

  stream_server_thread.join()
  stream_client_thread.join()

  if cross_traffic_enabled:
    ct_client_thread.join()

  client_side_monitor_thread.join()
  server_side_monitor_thread.join()

  net.stop()
  print("Test Successful")

