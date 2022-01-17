#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNetConnections
from mininet.log import setLogLevel
from time import sleep
class SingleSwitchTopo( Topo ):
    '''Single switch connected to n hosts.'''
    def build( self, n=2 ):
        switch = self.addSwitch( 's1' )
        for h in range(n):
            # Each host gets 50%/n of system CPU
            host = self.addHost( 'h%s' % (h + 1),
                                  cpu=.5/n )
            # 10 Mbps, 5ms delay, 2% loss, 1000 packet queue
            self.addLink( host, switch, bw=10, delay='5ms', loss=2,
                          max_queue_size=1000, use_htb=True )

def basicPipelineTest():
    '''Create network and run simple performance test'''
    topo = SingleSwitchTopo( n=2 )
    net = Mininet( topo=topo,
	           host=CPULimitedHost, link=TCLink )
    net.start()
    print( "Dumping network connections" )
    dumpNetConnections( net )
    print( "Running Gstreamer pipeline on h1" )
    h1 = net.get('h1')
    h2 = net.get('h2')
    print(h2)
    result = h1.cmd('gst-launch-1.0 -vvv audiotestsrc num-buffers=100 ! udpsink')
    print( result )
    h1.cmd('tcpdump > /tmp/tcpdump.out &')
    sleep(1)
    net.ping([h1,h2])
    sleep(1)
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    basicPipelineTest()
