# Timelog

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* DR Colin Perkins

## Week 1

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

## Week 2

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
* *1.0 hours* wrote up minutes for meeting and plan for next steps. Also updated project plan.

### 9 Oct 2021

* *1.0 hours* Updated to ubuntu 20.04 VM to allow GStreamer 1.16 to be utilised
* *1.5 hours* Created GStreamer QUIC source element from template. Confirmed build and installation worked
* *1.5 hours* Installed LSQuic on ubuntu VM. Added this use of this library to quicsrc plugin.


### 10 Oct 2021

* *1.0 hours* Had a preliminary read of the RTP test metrics Colin provided. Curious if these are applicable to reliable streams. Added questions to Q file for next meeting.
* *1.0 hours* Added QUIC gstreamer project to source control. The makefile provided had no full clean target so it took time to determine which files should be added to src control.
* *1.0 hours* Modified the QUIC src plugin such that it inherits from GstPushSrc.

## Week 4

### 14 Oct 2021

* *2.0 hours* Worked through lsquic tutorial
* *3.5 hours* Began adding functionality to QUIC source GStreamer element
* *0.5 hours* Meeting with Colin
* *0.5 hours* Added minutes and plan to appropriate meeting file
* *4.0 hours* Researched media streaming over various protocols and looked into how gstreamer split up data

### 16 OCT 2021

* *3.0 hours* Continued work on QUIC source element.

### 17 OCT

* *7.0 hours* Continued work on QUIC source element. 
* *1.0 hours* Looked into an issue where client-server handshake fails.

