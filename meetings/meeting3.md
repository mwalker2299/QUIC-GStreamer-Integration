# Meeting Conducted at 15:30 on 14/10/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Read RFC Documents provided by Colin. They provide realistic values for testing.
* Created GStreamer QUIC source element plugin and confirmed it could be installed.
* Installed LSQuic on the VM and sucessfully built the plugin while utilising this library.
* Worked through the LSQuic tutorial
* Began adding functionality to the QUIC source element

## Minutes:

* Gave an update on current status
* Discussed using QUIC for VOIP (place each frame in a single quic stream)
* Some confusion over whether a GStreamer server element would be created. Colin suggests I should.
* To accomplish this, I should look into what GStreamer is doing behind the scenes and how video can be split up and sent. 
* The QUIC GStreamer sources and sink should be able to handle multiple streams.


## Plan for next week:

* Continue working on elements.
* Look into how GStreamer handles media data.
* Look into techniques for splitting up video before transmission

