#!/usr/bin/python

import os
from time import sleep
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNetConnections
import threading
import network

def serverThread(serverNode, cmd, lock):
  with lock:
    print("Server: lock acquired")
    print("Server: " + cmd)
    result = serverNode.cmd(cmd)
    print(result)
  
  print("Server: lock released (Done)")

def clientThread(clientNode, cmd, lock):
  print("Client: starting client")
  # Client should run in background to allow thread to kill process when signaled
  cmd = cmd + " &"
  print("Client: " + cmd)
  clientNode.cmd(cmd)
  cmd_pid = clientNode.cmd("echo $!")
  print("Client: Started Process in background, waiting for server completing")
  with lock:
    print("Client: lock acquired, killing process " + cmd_pid)
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





def basicPipelineTest(net, server_command, client_command, protocol_name, log_path, log_level):
    '''Create network and run simple performance test'''

    # Setup logging paths
    server_gstreamer_log_path = os.path.join(log_path, "gst-server.log")
    server_lsquic_log_path    = os.path.join(log_path, "lsquic-server.log")
    client_gstreamer_log_path = os.path.join(log_path, "gst-client.log")
    client_lsquic_log_path    = os.path.join(log_path, "lsquic-client.log")
    tcpdump_log_path          = os.path.join(log_path, "tcpdump.pcap")

    # Create TCPDump command
    tcpdump_command = "tcpdump -i s2-eth1 > " + tcpdump_log_path


    # Start network and retrieve nodes
    net.start()
    print( "Dumping network connections" )
    dumpNetConnections( net )
    server             = net.get('server1')
    client             = net.get('client1')
    client_side_switch = net.get('s2') # Due to mininet restrictions on names, I am restricted to naming the switches s1, s2 rather than something more descriptive

    # Substitute server address and lsquic log location into client and server commands
    # UDP requires that client and server are given clients address, while QUIC is the opposite
    if "UDP" in protocol_name:
      print("PROTOCOL IS UDP")
      server_command = server_command.format(addr=client.IP())
      client_command = client_command.format(addr=client.IP())
    else:
      print("PROTOCOL IS QUIC")
      server_command = server_command.format(addr=server.IP(), lsquic_log=server_lsquic_log_path)
      client_command = client_command.format(addr=server.IP(), lsquic_log=client_lsquic_log_path)

    # Set up gstreamer logging on server and client
    server.cmd('export GST_DEBUG=' + log_level)
    server.cmd('export GST_DEBUG_FILE=' + server_gstreamer_log_path)

    client.cmd('export GST_DEBUG=' + log_level)
    client.cmd('export GST_DEBUG_FILE=' + client_gstreamer_log_path)

    # Create lock for synchronising threads. 
    # The clients will not stop on their own, and need to be killed once 
    # the server completes its transmission
    lock = threading.Lock()

    # Create threads for server node, client node and client-side switch node (for tcpdump)
    server_thread  = threading.Thread(target=serverThread, args=(server,server_command,lock))
    client_thread  = threading.Thread(target=clientThread, args=(client,client_command,lock))
    monitor_thread = threading.Thread(target=monitorThread, args=(client_side_switch,tcpdump_command,lock))

    # Start threads, wait for thread completion and then stop the network
    server_thread.start()
    client_thread.start()
    monitor_thread.start()

    server_thread.join()
    client_thread.join()
    monitor_thread.join()

    net.stop()

def run_test(test_params, server_command, client_command, protocol_name, log_path, log_level):
  # print(type(log_path))

  net = network.createDumbellTopo(test_params, servers=1, clients=1)
  basicPipelineTest(net, server_command, client_command, protocol_name, log_path, log_level)
  print("Test Successful")

