# Meeting Conducted at 14:00 on 12/01/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Started working on analysis scripts. Ran into issue with QUIC traffic being encrypted making it difficult to monitor. There did not seem to be a way to do this to tcpdump. tshark was capable of this, but required installing the latest version from source. Unfortunately, while tshark can extarct RTP packet info from RTP over UDP transmission, It is not capable of doing this for RTP over QUIC. To work around this, I have written a script which will extarct the timestamp and stream data from QUIC packets, and convert stream data corresponding to RTP sequence numbers to decimal.  The script can then comapre the difference in time stamp at client and server side switches in order to determine stack latency.
* Additionally, I also noticed that the current parameter setup will take far too long to run, so i sent a reduced parameter set to Colin to get his opinion


## Minutes:

* Went over steps to extract rtp data. Colin pointed me towards Qlog and QViz as useful tools for visualising QUIC transmissions.
* Discusses reducing parameter values and parellelisng work on stlinux machines.
* Had a conversation about different interenet connection mediums and their advantages.
* Talked about ways to improve wifi signals.

## Plan for next week

- Research video quality metrics and finish analysis scripts.
- Start looking into best way to parallelize tests.




