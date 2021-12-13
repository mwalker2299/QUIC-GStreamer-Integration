# Meeting Conducted at 13:00 on 26/11/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* While testing video streaming using the quic elements in combinations with gstreamers rtppayloader, I ran into an issue with playback. Initially this seemed to be an issue with the pipeline I had constructed, but I eventually tracked it down to a couple of bugs in the quicsrc code.
* There is still a much rarer issues with dropped rtp packets that I am looking into



## Minutes:

* Updated Colin on Current status (See above)
* Discussed plan for next week (See below)

## Plan for next week:

- Continue debugging current issues with gstreamer playback over a network
- Improve gstreamer elements so that they can identify frame types


