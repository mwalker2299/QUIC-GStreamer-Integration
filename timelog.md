# Timelog

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* DR Colin Perkins

## Week 1 (16.25)

### 30 Sep 2021

* *1 hour* Read the project guidance notes
* *0.5 hours* Set up Source Control repository on GitHub and added template project structure
* *1 hour* Added project specifics to project template files
* *1.5 hours* Researched QUIC implementations
* *2.25 hours* Researched GStreamer + setup vm to work on GStreamer
* *0.5 hours* Meeting with Supervisor
* *0.5 hours* set up ssh server on VM and confirmed connectivity

### 1 Oct 2021

* *3.0 hours* performed research on GStreamer and QUIC. 
* *1.0 hour* worked on project requirements/plan.


### 3 Oct 2021

* *3.0 hours* performed further research on GStreamer and QUIC. 
* *2.0 hours* worked on project requirements/plan.

## Week 2 (23.5)

### 4 Oct 2021

* *2.0 hours* Identified and analysed an issue with project plan requiring reconsideration (tcp elements do not use encryption).
* *3.0 hours* performed further research on GStreamer and TCP to create solution for issue.
* *1.0 hours* reworked project plan.


### 5 Oct 2021

* *6.0 hours* Looked into mininet for test purposes. Expanded requirements and plan with new info


### 6 Oct 2021

* *3.0 hours* Added final touches to requirements/plan document and added deliverables.

### 7 Oct 2021

* *0.5 hours* meeting with supervisor
* *0.25 hours* wrote up minutes for meeting and plan for next steps. 
* *0.75 hours* updated project plan.

### 9 Oct 2021

* *1.0 hours* Updated to ubuntu 20.04 VM to allow GStreamer 1.16 to be utilised
* *1.5 hours* Created GStreamer QUIC source element from template. Confirmed build and installation worked
* *1.5 hours* Installed LSQuic on ubuntu VM. Added this use of this library to quicsrc plugin.


### 10 Oct 2021

* *1.0 hours* Had a preliminary read of the RTP test metrics Colin provided. Curious if these are applicable to reliable streams. Added questions to Q file for next meeting.
* *1.0 hours* Added QUIC gstreamer project to source control. The makefile provided had no full clean target so it took time to determine which files should be added to src control.
* *1.0 hours* Modified the QUIC src plugin such that it inherits from GstPushSrc.

## Week 3 (21.5)

### 14 Oct 2021

* *2.0 hours* Worked through lsquic tutorial
* *3.5 hours* Began adding functionality to QUIC source GStreamer element
* *0.5 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *4.25 hours* Researched media streaming over various protocols and looked into how gstreamer split up data

### 16 OCT 2021

* *3.0 hours* Continued work on QUIC source element.

### 17 OCT 2021

* *7.0 hours* Continued work on QUIC source element. 
* *1.0 hours* Looked into an issue where client-server handshake fails.


## Week 4 (11.5)

### 21 OCT 2021

* *5.5 hours* Identified cause of handshake failure within client element. Added an issue to the github for lsquic, hoping that they have insight. Using an earlier version of BoringSSL resolves the issue.
* *0.5 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *2.75 hours* Now that the handshake succeeds, continued developing QUIC client element. 


### 24 OCT 2021

* *2.5 hours* QUIC client element can now successfully connect to a server, receive data and pass this data as a buffer down the pipeline.


## Week 5 (16.25)

### 28 OCT 2021

* *2.5 hours* investigated boringssl version issue to provide info on github issue I created
* *1.0 hours* fixed bug causing a segfault in the client element.
* *0.25 hours* Meeting with colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *3.0 hours* Upgraded to a newer version of gstreamer. This simplifies the build process and allows use of a template for gstbasesink which will be used by server element.


### 30 OCT 2021

* *0.5 hours* Created quic sink element from template and added it to the gstquic plugin
* *3.5 hours* Began adding functionality to sink element. Also extracted logic common to client and server into its own file 

### 31 OCT 2021

* *4.5 hours* Continued adding functionality to sink element. Focusing on setting up the lsquic engine to allow the server to accept incoming connections
* *0.75 hours* Fixed build issue where the incorrect openssl header were included. (System headers were used but the headers defined by boringssl should have been used instead.)


## Week 6 (12.5)

### 4 NOV

* *0.5 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *7.75 hours* Added remaining functions neceesary for the quicserver element to communicate with a quic client.


### 5 NOV 
* *4.0 hours* Fixed issue with Linux VM after a crash. GUI was not starting which prevents testing gstreamer videosinks. After exausting all solutions found online, I had to reinstall ubuntu.

## Week 7 (22.0)

### 11 NOV 2021

* *10.00 hours* Added remaining Gstreamer functionality to quic sink. Data within GstBuffers received from upstream can now be written to a stream. The quicsrc then retrieves the data and sends it downstream in a new GstBuffer.

### 12 Nov 2021

* *1.00 hours* Fixed a bug where quicsink would not transmit the full buffer.
* *1.00 hours* Fixed a bug where quicsink would wait forever.
* *1.00 hours* Fixed a bug where quicsrc would loop forever while trying to read a close streamed.
* *9.00 hours* The VM was too slow to propely process and play video using gstreamer. So I moved to a dual boot instead. This failed the first time and took longer than expected. I managed to break the bootloader for windows in the process which took some time to debug and fix.


## Week 8 (17.0)

### 19 Nov 2021

* *3.00 hours* Researched potential points of comprison for dissertation. Decided on RTP over semi-reliable quic vs RTP over UDP 
* *3.00 hours* Looked into Gstreamers RTP framework and experimented with sending video over RTP via quic and UDP.
* *2.00 hours* Added ability for quicsink to detect I-frames.
* *0.25 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file


### 20 Nov 2021

* *7.00* While testing that the quic elements could send video, I noticed an issue. playback would not start and the h264parser element was dropping all frames. This affected the existing udpsrc and sink elements as well so I initially believed the issue to be in my pipeline setup. However, I tracked the cause down to two bugs in gstquicsrc. These have now been fixed.
* *1.50 hours* Added bufferlist support to quicsink. This will allow it to work with the RTP payloader which exists within gstreamer.

## Week 9 (0.5 hours)

### 25 Nov 2021

* *0.25 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file

No work was done during week 9, 10 and part of 11 as I was focused on coursework + exams and then fell ill.

## Week 11 (12.75)

### 8th Dec 2021

* *4.00 hours* Looked into bug which was causing distortion on video transported via quic elements. It looks like an I frame is being dropped due to an rtp error.

### 9th Dec 2021 

* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file

### 11th DEC 2021

* *8.00 hours* After a lot of debugging, Identified source of video issues. Rarely, the quic src element was having an issue handling parallel streams. This lead to buffer corruption, causing the rtp depayloader to drop the buffers. As a result the first I-frame could not be decoded. Added some changes to the way quic src operates to mend this.
