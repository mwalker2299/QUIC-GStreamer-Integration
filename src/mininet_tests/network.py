#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from time import sleep

class DumbellTopo( Topo ):
    '''creates a dumbell topology with a configurable number of client and server hosts'''
    def build(self, link_params, servers=1, clients=1):
        server_side_switch = self.addSwitch('s1')
        client_side_switch = self.addSwitch('s2')
        total_hosts = servers + clients

        # Convert params to correct form
        delay =     str(link_params["delay"]) + "ms"
        jitter =    str(link_params["jitter"]) + "ms"
        bandwidth = link_params["bandwidth"]
        loss =      link_params["loss"]  


        for server in range(servers):
            # Each host gets 50%/n of system CPU (where n = servers+clients)
            host = self.addHost('server%s' % (server + 1),
                                  cpu=-1)
            # We wish to be able to control loss and delay through the central link alone
            # To accomplish, all other links have 0ms delay and 0% loss
            self.addLink(host, server_side_switch, delay='0ms', loss=0, bw=bandwidth, max_queue_size=100000)

        for client in range(clients):
            # Each host gets 50%/n of system CPU (where n = servers+clients)
            host = self.addHost('client%s' % (client + 1),
                                  cpu=-1)
            # We wish to be able to control loss and delay through the central link alone
            # To accomplish, all other links have 0ms delay and 0% loss
            self.addLink(host, client_side_switch, delay='0ms', loss=0, bw=bandwidth, max_queue_size=100000)

        # Add final link between switches and set params
        self.addLink(server_side_switch, client_side_switch, delay=delay, loss=loss, jitter=jitter, bw=bandwidth, max_queue_size=100000)

def createDumbellTopo(link_params, servers, clients):
  topo = DumbellTopo(link_params, servers=servers, clients=clients)
  net = Mininet(topo=topo,
            host=CPULimitedHost, link=TCLink)

  return net
