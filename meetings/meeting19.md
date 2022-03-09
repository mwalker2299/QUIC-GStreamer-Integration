# Meeting Conducted at 14:00 on 09/03/22

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Enforced 24 fps framerate
* Reworked frame analysis scripts so that incomplete frames are not counted as being successfully decoded.
* Added timers to the jitterbuffer so that late HOL stream packets would be noticed.
* Fixed a bug in lsquic where new streams would cause packets to be sent while there was no allowance in the congestion window.
* Created first draft of dissertation

## Minutes:

- Colin feels that the current dissertation focuses far too much on how and not enough on why.
- I should shorten the introduction moving the finer details into the background section.
- Background can then cover the different areas of media streaming (VoD, Live broadcast and interactive (VoIP)) -> Existing protocols and their issues -> new kid on the block and how it could solve these issues.
- Related work can remain mostly as is but can be expanded upon.
- Either at the end of the related work section or start of design, we should comment on clipstream and QUICSilver and our percieved flaws (unintelligent selective retransmission and does not take advantage of streams, instead requires client to have foreknowledge regarding how the )in their approach -> why we have taken our approach (take advantage of streams, dont require client to have foreknowledge of how the video data is structured)(GOP, frames, nal units)
- Then the implementation details should be covered briefly
- Into the results, conclusions and future work.



## Plan for next week

- Rework dissertation to focus on why and not how

