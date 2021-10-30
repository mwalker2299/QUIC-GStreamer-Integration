# Meeting Conducted at 15:30 on 21/10/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Posted on GStreamer message boards. They recommended chopmydata but this seemed to break things
* Worked on getting client element functional. Ran into issue with boringssl during the handshake.
* Gstreamer provides parser elements that can split streams into frames

## Minutes:
 * Updated Colin on status. Discussed BoringSSL bug. Colin is happy for me to continue with and older (but still quic compatible) version until We hear back from the lsquic team.
 * Discussed possible points of comparison:
    - Dash vs QUIC
    - Webrtc over RTP over UDP

* Discussed dissertaion structure:
  - Discuss various scenarios in which media is streamed and how they diff
  - Discuss technqiues for streaming in these scenarios and the suitability of existing techniques
  - Discuss where QUIC fits in.

## Plan for next week:

- Get client element operational
- Look into RTP
