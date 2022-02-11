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

## Week 12 (26.75)

### 13th DEC 2021

* *4.00 hours* Spent some time cleaning up commits and removing unnecesarry changes to code. A lot of changes were committed together then moving to the dual boot, this work separated the commits to make it clear what changed.


### 14th Dec 2021

* *6.00 hours* set up mininet, worked through walkthrough and began reading example code.


### 15th Dec 2021
* *4.00 hours* Performed further background research on UDP, RTP, TCP and IPTV to ensure I fully understood the purpose of the project before writing the status report.
* *4.00 hours* Wrote and submitted status report

### 16th Dec 2021

* *8.00 hours* Read through mininet's Python API documentation and guide. Read through more mininet python script examples. Performed further experiments with CLI. Created some basic mininet scripts using the python API.
* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file


## Winter Break

* *5.00 hours* Learned how to use threads within python for use within mininet scripts. also brushed up on modules, the main method, handling cmdline arguments and manipulating json files.
* *4.00 hours* Planned out mininet test framework in more detail and created pseudocode for the framework
* *1.00 hours* Created config files providing server and client gstreamer commands for each impl as well as parameter values for testing.
* *12.00 hours* Created all necessary mininet framework scripts and added skeleton for each based on pseudocode. Completed code for the test_runner.py and the network.py scripts. Also tested and debugged a few issues with results creation. Next, I began work on the test_loop.py script. All that remains is finish work on the test_loop.py script and add result analysis capabilities. (This work is slow going at first due to being rusty with python and inexperienced with mininet, but getting faster as work continues.)
* *10.00 hours* Completed work on test_loop.py script, added tcpdump support and added the ability to set lsquic log destinations to gstquic cmdline options. Once this was done, the framework was tested to confirm that is produced reasonable results. This work involved some unexpected obstacles such as partitioning more space for linux so that the test results didn't use up all availiable space as well as identifying the best way to substitute the correct ip address into the gst-launch commands which start the pipelines. Another obstacle was that the client pipelines do not shut themselves down when transmission completes, so the threads were reworked such that the client thread will run its process in the background and kill it when the server thread completes.
* *6.00 hours* After the initial tests of the framework, I ran a longer test using a set of possible delay and loss parameters that I felt were reasonable. The iterations were increased to 10. This worked well until it stalled on one of the final QUIC test runs. I was initially unable to reproduce this error but eventually found that it occurs rarely on QUIC runs with a high loss percentage. Unfortunately, the mininet api does not return the output of commands which don't complete, making it difficult to idnetify the cause. I have been trying to reproduce the error using the mininet cli as this should allow the output to be observed.


## Semester 2

## Week 1 

### 12th JAN 2022


* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *2.5 hours* I successfully reproduced the stall that occurs on high loss links within the cli, however, stdout of gstreamer did not provide any hints as to the cause of the problem. I then switched to the gstreamer logs where I discovered that the quic elements were not outputting debug to the log. Additionally, gst-inspect was throwing a segfault when inspecting the quic elements. After some debugging, this was due to an issue where the element was disposed of before start was called. When the element is disposed of, it will destroy its lsquic engine instance. However, in the case of the issue, the engine instance had never been created, leading to a null dereference.
* *1.5 hours* With access to the logs, I discovered that if a connection was closed due to major packet loss, the server would stall on the connection close callback. GDB shows that the last call is to recv_msg but this should be non-blocking. This requires further investigation, but as this error only occurs on loss levels which would never occur in reality, I have chosen to prioritize finishing work on the test framework first (Identify real world parameters, complete analysis script)


### 16th JAN 2022

* *3.00 hours* Researched Network parameters for use in test framework. Decided on Delay and Loss parameters given by RFC 8868. Altered test framework to utilise these params.


### 19th JAN 2022

* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *2.00* After discussion with Colin, performed some preliminary research on new param considerations.

### 21st JAN 2022

* *8.00 hours* Researched new parameters for the test framework. I checked ofcom reports, RFC documents and any other reputable resources I could find to select appropriate values for testing.

### 23rd JAN 2022 

* *2.00 hours* Finished parameter research which began on Friday and wrote up results in a markdown file for Colin.

* *3.5 hours* Modified the test bed to allow it to accept the new parameters that were chosen.

* *2.00 hours* Fixed test bed stall. When running gdb the only thread active is the recvmsg thread which seems to stall. I could not determine why this stall happened. However, the stall could also occur at the start if a connection could not be established. Since this was correct behaviour, I chose to implement an autotimeout to fix both issues. 
* *1.00 hours* Tested the autotimeout. I ran into an issue where first, QUIC couldnt send packets. This seemed to resolve itself after a reboot. Then while testing I noticed that the loss was not impacting the tests. This appeared to be an issue with tclinks bandwidth param and on closer inspection the first test set this value to 0 rather than 1 Mbit/s. After amending this the issue went away.


### 26th JAN 2022

* *3.00 hours* Studied data gprovded by gstreamer logs and packet capture output, and researched potential analysis metrics before meeting with Colin
* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file


### 30th JAN 2022 

* *1.50 hours* Looked into queue length. Mininet only accepts queue size in packets. I have come up with a formula that uses the MTU (1500 for ethernet) to convert the queue size in bytes to the queue size in packets. However, the value seems too low so I need to discuss it with colin before implementing this formula.
* *1.50 hours* Added a second tcpdump process at the server-side switch. This will allow me to monitor the stack latency of packets. Also changed the synchronisation mechanism of the threads from a lock to an event as this was more appropriate.
* *1.00 hours* While testing the second tcpdump, I noticed that the timeout no longer seemed to work properly. I was unsure why this was happening at first, but realised the issue was not the timeout, but that the playback time was taking longer than before. This turned out to be due to the low bandwidth setting but interestingly, this did not result in warnings because the server was slowed down due to the bandwidth being low.
* *2.50 hours* While the packet capture for udp worked well, I realised that it would not be as simple for QUIC. QUIC is encrypted and tcpdump does not appear to have the capability to decrypt it. Due to a lack of good tutorials and poor documentation, it took some time to determine that tshark should be capable of decrypting the QUIC packet captures. However, after reading a number of forum posts and watching a couple of youtube tutorials I uncovered the necessary steps needed to accomplish this.


### 31st JAN 2022

* *1.50 hours* Added ability for quicsink to write SSL keys to a file for use by tshark in decrypting quic packets.
* *3.00 hours* Attempted to decrypt quic packets with tsharks. quicsink correctly logs the keys but unfortunately I was seeing no difference when providing these keys to tshark. After reading some forum posts I came to the conclusion that my version of tshark was too old to decrypt IETF QUIC v1. Installing the latest version from source took some time due to missing dependencies.
* *4.00 hours* While tshark is able to decrypt IETF QUIC v1, it oddly does not have the capability to decode the stream data as rtp packets. I spent several hours trying to get this to work as well as trying some rtp pcap decoder tools but nothing was working. However, my understanding of tshark did deepen and I realised I could extract a frame timestamp + a hex representation of the stream data. From this, the analysis script will be able to extract the rtp packet number. As a bonus, the frame timestamps will be much easier to compare than the default timestamp. (Took a lot longer than expected but we got there in the end!)


### 2nd FEB 2022

* *4.25 hours* Wrote script which extracts timestamps rtp and packet numbers from the frame time and QUIC stream data. The script then calculates the time between sending each packet and that packet arriving. Lost packets are marked with a -1 time difference for easy identification.
* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file


### 9th FEB 2022

* *2.00 hours* Continued work on analysis scripts.
* *1.00 hours* Confirmed I could connect to the stlinux boxes. This required an update to my cisco anyconnect app which had apparently become too outdated to function. I then struggled to remember the procedure for connecting but after asking some fellow students I was set on the right track.
* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file

