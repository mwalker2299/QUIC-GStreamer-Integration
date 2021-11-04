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

### 17 OCT

* *7.0 hours* Continued work on QUIC source element. 
* *1.0 hours* Looked into an issue where client-server handshake fails.


## Week 4 (11.5)

### 21 OCT

* *5.5 hours* Identified cause of handshake failure within client element. Added an issue to the github for lsquic, hoping that they have insight. Using an earlier version of BoringSSL resolves the issue.
* *0.5 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *2.75 hours* Now that the handshake succeeds, continued developing QUIC client element. 


### 24 OCT

* *2.5 hours* QUIC client element can now successfully connect to a server, receive data and pass this data as a buffer down the pipeline.


## Week 5 (16.25)

### 28 OCT

* *2.5 hours* investigated boringssl version issue to provide info on github issue I created
* *1.0 hours* fixed bug causing a segfault in the client element.
* *0.25 hours* Meeting with colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *3.0 hours* Upgraded to a newer version of gstreamer. This simplifies the build process and allows use of a template for gstbasesink which will be used by server element.


### 30 OCT

* *0.5 hours* Created quic sink element from template and added it to the gstquic plugin
* *3.5 hours* Began adding functionality to sink element. Also extracted logic common to client and server into its own file 

### 31 OCT

* *4.5 hours* Continued adding functionality to sink element. Focusing on setting up the lsquic engine to allow the server to accept incoming connections
* *0.75 hours* Fixed build issue where the incorrect openssl header were included. (System headers were used but the headers defined by boringssl should have been used instead.)