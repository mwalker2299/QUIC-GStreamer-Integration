# Meeting Conducted at 14:00 on 02/03/22

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Activated TCP_NODELAY and TCP_QUICKACK options, reducing sending delay and retx delay.
* Set TCP congestion control scheme to BBR
* Reworked the pipeline to encode data before sending, it now better mimics a real life streaming application.
* Created script for monitoring frame latency.
* QUIC now decrypts PCAP dynamically, reducing necessary storage space
* Got a Tereabyte external SSD just in case.
* Experimented with different lsquic settings to find best choices (e.g. max stream flow control window)
* test runner will restart from the where it was in case of crash
* Added QUIC GOP implementation
* Test loop now monitors client side pipeline to tell when run is finished.
* Mended and unified the disposal code for the QUIC elements, eliminating a bug where the client would not be made aware of connection close. Also set delay_onclose parameter to prevent server from closing the connection while there are outstanding transmissions
* Modified stack_analysis_code for tcp streams to work with QUIC SS and GOP. Also improved memory efficiency to avoid crash.
* Fixed jitterbuffer timing so that packets are declared lost at appropriate time.
* Ensured that datagrams would only contain data from one stream.

## Minutes:

- Discussed 150ms application delay. This normally includes capture and presentation so our resuts may be an underestimate.
- Colin would like me to send over my first attempt at dissertation by tuesday lunchtime.



## Plan for next week

- Check with supervisor about dissertation deadline.
- Start working on dissertation full time


