# Meeting Conducted at 14:00 on 23/02/22

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Acquired BBB test video (60 secs at 720p)
* Fixed an issue where the jitter buffer wouldn't accept the seqnum-base capacity from the pipeline
* Improved responsiveness of QUIC implementations by reducing the max ack delay and ticking the engine periodically. There is still some added delay occasionally before overall it is much more responsive.
* Implemented TCP elements and got it ready for testing. The existing gstreamer elements 
* Fixed a bug in the packet-per-stream implementation would not processed certain streams.
* Got Single stream quic implementation ready for testing
* Tshark was unable to process rtp over tcp if any loss or reordering occured, so I wrote my own code for anlysisng the tcp paylodas.
* Just need to modify tcp analysis for single stream and gop stream quic implementations and then test can be started.

## Minutes:

- Discussed current status
- Colin explained how congestion works for udp (hint: it doesnt, the application is responsible)
- Colin pointed me towards the rtp multimedia IETF working group whose papers may help with dissertations: https://datatracker.ietf.org/wg/rmcat/documents/
- Colin made clear that its good to start dissertation as soon as possible
- Colin suggested that the TCP_NO_DELAY socket option may improve responsiveness. There are other socket options which may help aswell
- I should use tshark and qviz to identify whether cause of delayed retx
- Colin agreed that if Selective retransmission quic could not work with lsquic, analytical analysis would still be good 
- Colin suggested that analytical analysis could be used for all implementations. If it lines up thats good, otherwise we can discuss why not.
- The second NS AE is out next Thursday


## Plan for next week

- Finish work on analysis script changes for Single stream and GOP per stream QUIC
- Identify cause of delayed retransmission in QUIC and TCP. Fix if necessary.
- Final touches on GOP per stream implementation
- short Dry run to make sure there arent any issues
- Start full run
- Continue work on Dissertation.


