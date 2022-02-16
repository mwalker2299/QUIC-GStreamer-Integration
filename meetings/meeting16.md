# Meeting Conducted at 14:00 on 12/01/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Finished work on scripts to extract stack and app latency and calculate useful frames
* Got a reply from support, no sudo access can be granted so I have reduced the parameters to allow the tests to complete within a week. Bandwidth, router queue length and cross traffic have been removed.
* Fixed some bugs that were causing HOL blocking and late packets to not be dropped by the jitter buffer
* Started on dissertation

## Minutes:

- Discussed the superbowl 
- Discussed latency bounds required by different applications: VOIP~=150ms, Live streams current lower bound is 1000ms, for synchronised ocherstra lower than 150 may be required.
- Colin was happy for me to use the application latency as a method for determining which frames are useful, 1000ms jitter buffer can be used for all tests.
- Colin provided a link to the TCP goes hollywood implementation: https://csperkins.org/publications/2016/05/mcquistin2016goes.html
- Discussed different implmentations to test: TCP, packet per stream, Gop per stream, single stream, UDP and potentially selective retransmission if possible (may not be due to lsquic)
- Colin was happy with 60 - 120 seconds of video for tests, he pointed me towards the following test video servers:https://media.xiph.org/video/derf/, https://pi4.informatik.uni-mannheim.de/~kiess/test_sequences/download/

## Questions

- Does VOIP use retransmissions? I can't seem to find an answer to this online.
- Does partial reliability actually have a chance to reduce latency?
- Rather than multiple runs with different jitterbuffer max latency values, would it make more sense to have a large jitter buffer (1seconds) and use the application latency to determine under which circumstances certain implementations perform well.


## Plan for next week

- Add missing QUIC implementations and look into possibility of selective retransmission
- Begin full test run
- Start fleshing out dissertation


