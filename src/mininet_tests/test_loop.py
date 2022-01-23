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

def streamServerThread(serverNode, cmd, timeout, lock):
  with lock:
    print("Stream Server: lock acquired")
    print("Stream Server: " + cmd)

    # To account for high loss scenarios where 
    # initial connection attempt fails to reach
    # server, we run server in background.
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

    cmd_done = serverNode.cmd("kill -0 " + cmd_pid)
    print(cmd_done)

    if not cmd_done:
      print("Stream Server: Process still running, allowing more time before killing manually")
      sleep(timeout)
      print("Stream Server: Killing process " + cmd_pid)
      serverNode.cmd('kill ' + cmd_pid)
    else:
      print("Stream Server: process terminated within timeout.")
  
  # We release the lock when done, allowing the other threads to complete
  print("Stream Server: lock released (Done)")

def streamClientThread(clientNode, cmd, lock):
  print("Stream Client: starting client")
  # Client should run in background to allow thread to kill process when signaled
  cmd = cmd + " &"
  print("Stream Client: " + cmd)
  clientNode.cmd(cmd)
  cmd_pid = clientNode.cmd("echo $!")
  print("Stream Client: Started Process in background, waiting for server completing")
  with lock:
    print("Stream Client: lock acquired, killing process " + cmd_pid)
    clientNode.cmd('kill ' + cmd_pid)

def crossTrafficClientThread(clientNode, cmd, lock):
  print("Cross Traffic Client: starting client")
  # Client should run in background to allow thread to kill process when signaled
  cmd = cmd + " &"
  print("Cross Traffic Client: " + cmd)
  clientNode.cmd(cmd)
  cmd_pid = clientNode.cmd("echo $!")
  print("Cross Traffic Client: Started Process in background, waiting for server completing")
  with lock:
    print("Cross Traffic Client: lock acquired, killing process " + cmd_pid)
    clientNode.cmd('kill ' + cmd_pid)

def monitorThread(node, cmd, lock):
  print("Monitor: starting monitor")
  # Monitor should run in background to allow thread to kill process when signaled
  cmd = cmd + " &"
  print("Monitor: " + cmd)
  node.cmd(cmd)
  cmd_pid = node.cmd("echo $!")
  print("Monitor: Started Process in background, waiting for server completing")
  with lock:
    print("Monitor: lock acquired, killing process " + cmd_pid)
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
  server_gstreamer_log_path = os.path.join(log_path, "gst-server.log")
  server_lsquic_log_path    = os.path.join(log_path, "lsquic-server.log")
  client_gstreamer_log_path = os.path.join(log_path, "gst-client.log")
  client_lsquic_log_path    = os.path.join(log_path, "lsquic-client.log")
  tcpdump_log_path          = os.path.join(log_path, "tcpdump.pcap")

  # Create TCPDump command string
  tcpdump_command = "tcpdump -i s1-eth1 > " + tcpdump_log_path


  # Start network and retrieve nodes
  net.start()
  print( "Dumping network connections" )
  dumpNetConnections( net )
  stream_server = net.get('server1')
  stream_client = net.get('client1')
  if cross_traffic_enabled:
    cross_traffic_client = net.get('client2')
    cross_traffic_server = net.get('server2')

  client_side_switch = net.get('s2') # Due to mininet restrictions on names, I am restricted to naming the switches s1, s2 rather than something more descriptive

  # Substitute stream_server address and lsquic log location into stream_client and stream_server commands
  # UDP requires that stream_client and stream_server are given the stream_clients address, while QUIC is the opposite
  if "UDP" in protocol_name:
    print("PROTOCOL IS UDP")
    stream_server_command = stream_server_command.format(addr=stream_client.IP())
    stream_client_command = stream_client_command.format(addr=stream_client.IP(), buffer_delay=test_params["buffer_delay"])
  else:
    print("PROTOCOL IS QUIC")
    stream_server_command = stream_server_command.format(addr=stream_server.IP(), lsquic_log=server_lsquic_log_path)
    stream_client_command = stream_client_command.format(addr=stream_server.IP(), lsquic_log=client_lsquic_log_path, buffer_delay=test_params["buffer_delay"])

  if cross_traffic_enabled:
    ct_command = ct_command.format(addr=cross_traffic_server.IP())

  # Set up gstreamer logging on stream_server and stream_client
  stream_server.cmd('export GST_DEBUG=' + log_level)
  stream_server.cmd('export GST_DEBUG_FILE=' + server_gstreamer_log_path)

  stream_client.cmd('export GST_DEBUG=' + log_level)
  stream_client.cmd('export GST_DEBUG_FILE=' + client_gstreamer_log_path)

  # Create lock for synchronising threads. 
  # The clients will not stop on their own, and need to be killed once 
  # the stream_server completes its transmission
  lock = threading.Lock()

  # Create threads for stream_server node, stream_client node and client-side switch node (for tcpdump)
  stream_server_thread  = threading.Thread(target=streamServerThread, args=(stream_server,stream_server_command,timeout,lock))
  stream_client_thread  = threading.Thread(target=streamClientThread, args=(stream_client,stream_client_command,lock))

  if cross_traffic_enabled:
    ct_client_thread =  threading.Thread(target=crossTrafficClientThread, args=(cross_traffic_client,ct_command,lock))

  monitor_thread = threading.Thread(target=monitorThread, args=(client_side_switch,tcpdump_command,lock))

  # Start threads, wait for thread completion and then stop the network
  stream_server_thread.start()
  stream_client_thread.start()

  if cross_traffic_enabled:
    ct_client_thread.start()

  monitor_thread.start()

  stream_server_thread.join()
  stream_client_thread.join()

  if cross_traffic_enabled:
    ct_client_thread.join()

  monitor_thread.join()

  net.stop()
  print("Test Successful")

