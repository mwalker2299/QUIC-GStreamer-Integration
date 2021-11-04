# Meeting Conducted at 15:30 on 04/11/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Waiting on hearing back from the other lsquic user who experienced the same issue.
* Changed the build system to meson, which is easier to debug and is the build system used by newer versions of gstreamer.
* Server element can accept connections from a quic client and respond. Still need to add gstreamer functionality for accepting buffers.

## Questions:
* We do not have a meeting next week. Can I still get in touch via email if I have any queries. - yes

## Minutes:

* Brief conversation about how we are. Discussed hobby of feeding and photographing garden birds.
* Updated Colin on current status
* Discussed slightly longer term plan: once client server are working and can stream video, start work on mininet.
* Colin is happy for me to send him emails next week when he is attending IETF conference.
* Discussed how anyone can attend IETF meetings. Colin recommends missing the one next week to work on project but I can watch the videos afterwards.
* For evaluation: I should look into the space of media transport techniques, pick a area to explore with justification and compare how well quic works compared to the existing method. (low latency transport, single stream (youtube), dash (netflix))


## Plan for next week:

- Add gstreamer functionality to server to allow it to send buffers from upstream as quic packets.
- Once the above step is complete, test that a video stream can successfully be transmitted over the connection.
- If there is time research content delivery methods and select a point of comparison. (I expect this will be carried over to next week)
