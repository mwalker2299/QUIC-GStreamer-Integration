# Meeting Conducted at 15:30 on 19/11/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* The quic source and sink gstreamer elements can now send buffers between each other.
* The VM I was using crashed and the GUI no longer worked following this.
* After reinstalling, i found that the VM was not fast enough to playback media using gstreamer.
* My attempts at improving vm performance were not succifient, so I moved to a dual boot.
* Made decision on comparison for dissertation: Can partially-reliable quic over rtp match the latency of udp over rtp while provifing better quality playback?
* Had some issues to debug with video playback.



## Minutes:

* Updated Colin on Current status (See above)
* Colin is happy with point of comparison: quic reliable single stream vs quic reliable stream per frame vs quic semi reliable stream per frame vs quic frame batch per stream vs RTP over UDP.
* Discussed plan for next week (See below)
* https://www.vagrantup.com - docker for VMs
* 


## Plan for next week:

- Debug current issues with gstreamer playback over a network
- Improve gstreamer elements so that they can identify frame types

### Time allowing: 
- Add ability for quicsink to send non-I-frames via an unreliable datagram
- Confirm that playback has no issues with buffering when using quic elements. 
