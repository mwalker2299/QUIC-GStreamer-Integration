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

### 11th FEB 2022

* *5.00 hours* Planned out work for next few days, cleaned up code base, finished work on the stack latency script and added the script to the analysis stage of the test framework.
* *1.0 hours* Started looking into steps for getting test framework setup on stlinux. Unfortunately, I realise that this would not work as sudo permissions would be required.
* *2.00 hours* Installed gstreamer from source and confirmed that changes made were reflected in the logs.
* *1.00* Looked into an issue where QUIC_FPS will run very slowly at 300ms latency. This issue isnt noticeable at the lower param value of 150ms. For some reason lsquic is delaying about 4ms between stream creation and allowing me to write. I'm stumped as to why, but theoretically this shouldnt impact the other QUIC implementations as they create far fewer streams.

### 12th FEB 2022

* *1.50 hours* Discovered that 300ms also impacted the UDP implementation. the first 30 or so packets are dropped at this latency. Additionally for all latencyies above 10ms, there is noticable unintended jitter in the first group of packets. I discovered that both of these issues could be eliminated by running an iperf test before running the actual tests for each run.
* *0.50 hours* Eliminated a bug in the stack latency script that caused the time diff to be always -1 (indicating loss)
* *2.00 hours* Added necessary debug to allow app latency and video quality metrics to be extracted. This required some research on H264 encoding and Nal units which was summarised in `Keyframe_notes.md`.
* *0.50 hours* introduced pandas to analysis scripts to improve readability of output
* *2.25 hours* Began work on app latency and video quality analysis script. See `Keyframe_notes.md` and `Log notes.md` for explanation of what is extracted.


### 13th FEB 2022

* *4.00 hours* Finished possible work on analysis scripts. The test framework will produce csv files for the stack latency and app latency. It will also output the total number of frames, the number of useful frames and the percentage of useful frames

### 14th FEB 2022

* *1.00 hours* Fixed a bug in the app latency script. It previously expected values to occur in a fixed place on relevant log lines. This however can be variable in rare cases. It now searches the string to find the values regardless of their absolute position in the string
* *2.50 hours* Fixed a bug in gstquicsrc which prevented later streams from being processed when they were ready in the event that the earliest stream was not ready. Essentially the bug introduced HOL blocking. Also added code to ensure stream contexts were freed.
* *0.5 hours* Fixed bug in stack latency script. The restriction on what sequence of bytes represented the start of rtp packets we are interested in was to lenient resulting in incorrect recoding of sequence numbers. I have placed tighter restriction in the code now and cannot recreate the issue. 


### 16th FEB 2022

* *1.50 hours* fixed issue where late packets would not be dropped by rtpjitterbuffer
* *2.00 hours* Started to rethink the current evaluation strategy. Right now we have multiple runs using different sized jitterbuffers on the receive side. If a packet is late it is marked as lost and never passed onto the application. Thus the associated nal unit is never complete and we consider it to be a useless frame (For I-frames this status propagates to future p-frames). However, it might make more sense just to have a large jitter buffer (1 seconds) and then calculate which frames would be useful under different latency requirements. (e.g. VOIP starts to become unusable with an apllication latency of over 150ms, IPTV currently operates with a minimum latency of 1s on ultra low latency streams)
* *1.00 hours* Started work on dissertation
* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *1.00 hours* Procured a new test video (BBB) and cut it down to a 60 second clip
* *3.00 hours* Fixed an issue where the new 1 second jitter buffer would hold the first few packets for up to a second as it was not sure it had seen the first packet. This should have been configurable from the commandline but I was seeing no difference when I passed the necessary cap (seqnum-base). I have hardcoded this for now.


### 18th FEB 2022

* *1.00 hours* Fixed a bug in gstquicsrc that would delete stream contexts which had not yet been processed.
* *1.00 hours* Looked into adding TCP to the list of implementations. However, As I had seen before, the tcp gstreamer elements do not seem to work very well. The rtp elements quickly began complaining that all of the buffers were invalid. For now, I have chosen to exclude TCP as the single stream QUIC implementation serves the same purpose.
* *2.00 hours* Investigated to see if the rtpjitterbuffer could be modified such that packets are held based on their frame. I.e. if we are waiting on packets from Frame A to arrive, but currently have packets from Frame B, it would be ideal for Frame B to be pushed. However, this does not seem possible.
* *4.00 hours* Looked into issue where quic_fps can take up to 40ms to retransmit a lost packet. I tried to add the tcp implementation for comparison but the element does not seem to work with the the rtp payloaders. By increasing the rate at which the lsquic engine ticks, and reducing the max ack delay to 5ms we do see improvement. However, there are still outliers. For now I will implement single stream QUIC and GOP per stream quic and see if this behaviour remains (its possible the high number of streams is impacting performance and this is the cause of the delay)

### 21st FEB 2022

* *8.00 hours* Contrary to the previous entry, I next decided to debug the issues with the tcp elements when using rtp over tcp. At first I believed that the issue lay in my pipeline construction as well as the commands I was giving to tshark. Strangely however, rtp data just seems to go missing when it is passed to the existing tcp element. The element appears to be set up for multiple clients with bidirectional data transfer and is incredibly complicated. As it would take several hours atleast to comprehend how this element works, I instead opted to create my own as I felt this would be quicker. This ended up working quite well so we now have a working tcp implementation for use in testing.


### 22nd FEB 2022

* *7.50 hours* After overnight testing completed, An issue in the tcp test was revealed. tshark will fail to dissect rtp packets if any loss or reordering occurs in transit. I spent some time looking through forums and the tshark source code but I could not find a solution. As an alternative, I reworked the analysis scripts so that they could handle tcp payload data and parse the rtp packets that way. This turned out to be quite complicated to do effieciently. I wasnt sure if I should measure the latency from when the full rtp packet arrived or when the kernel made it available to the application (tcpsrc). So in the end I did both.
* *0.5 hours* When testing the new latency analysis for tcp, I noticed that some packets took more time than was reasonable despite the fact that they were never lost. This turned out to be caused by a delay when `rtph264pay` pushes a list of buffers. I was unable to reduce this delay.
* *0.50 hours* Renamed existing `gstquicsrc`/`gstquicsink` elements to `gstquicsrcpps`/`gstquicsinkpps` (both in terms of filename and within the code itself) to reflect that this element sends a single rtp packet per stream. Tested to make sure I hadn't inadvertantly broken anything.
* *4.50 hours* Added single stream implementation of QUIC and performed testing. Ran into a bug that appeared to arise from lsquic not updating its flow control window but actually was caused by a null reference. Rather than crash, the pipeline stalled and lsquic seemed to lose some functionality. The odd presentation made it difficult to identify the cause but after some debugging I identified the issue. After this I kicked off a test using all the existing implementations.


### 23rd FEB 2022

* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file


### 24th FEB 2022

* *2.00 hours* While reviewing the results from the dry run, I noticed greater delays than expected. I spent some time trying to identify the cause of delays and improve performance. 
  - The first step was identifying why tcp had a greater app latency than the other implementations. This was due to the smaller rtp packets being delayed while tcp waited for more data to send. Enabling TCP_NODELAY and TCP_QUICACK solved this.
  - The app latency for I-frames in the big buck bunny test file is much larger than for the zoom recording. This is due to the I-frames being significantly larger. I considered replacing the BBB video with a zoom recording. However, as i would prefer to use a common test video, I instead opted to increase the bandwidth.

* *8.00 hours* Looked into reworking the pipeline so that it encoded data before sending it to more accurately model a real streaming application. This took some finetuning before it would work correctly, however the pipelines are now updated and I have identified the necessary changes to the gstreamer analysis scripts.


### 25th FEB 2022

* *1.50 hours* During testing, a new issue was revealed The first was that the pcaps for the quic streams used up far too much space. It appears that the ssl secrets are logged in time to decrypt the first packet, so we no longer need to store the full packet capture. I added some extra details about the stream (ID, length, offset fin) just in case these are necessary for future analysis. I also went to PC world to pick up an external ssd. By moving the test results to this SSD, we should have no risk of running out of diskspace.
* *2.50 hours* Despite working fine previously, the Single stream quic implementation is now failing frequently tests. There was no obvious cause for this in the logs but the pipeline appeared to stall. It was difficult to determine the cause immediately as neither gdb nor the logs were providing much info. In the end, I looked into the possibility of a multithreading issue between gstreamer and lsquic. This was the problem and I have now mended this.
* *5.00 hours* I noticed a delay in the higher latency runs for the quic implementations. 
  - On the single stream implementation, this was caused by a small maximum stream flow control window. On higher latencies, the frequent stream_blocked messages took a significant amount of time to arrive causing a delay when writing at the server. Setting the max stream flow control window to a higher value eliminated this issue.
  - On the packet per stream implementation, there were frequent delays caused by the maximum stream count being reach repeatedly. Setting a higher initial stream maximum and increasing the rate at which the stream maximum could grow fixed this issue.

### 26th FEB 2022

* *1.50 hours* Mininet appeared to crash during an overnight dry run. The error message has been talked about on forums before and it doesnt seem to be caused by anything ive done. This error has never occured before, so I expect it is rare. In case this error occurs again, I have modified the test_runner to allow it to start again from whatever point it got to before stopping.
* *0.50 hours* The mininet bug appeared again, So i have added a bash script which will restart the tests if necessary.
* *0.25 hours* When sending rtp packets over a stream with no message boundaries to delineate the start and end of packets, we add a 2 byte length indicator to the beginning of packets. The rtppayloader does not take this into account, and so we end up with rtp packets larger than the path mtu. I have set the maximum path mtu to 1400-2=1398 for implementations which utilise rfc 4751.
* *2.00 hours* Added GOP quic implementation.
* *3.00 hours* Looked into an issue on single stream QUIC where the connection would fail due to a flow control violation on high jitter. This was caused by what appears to be an lsquic bug, the server's initial max send offset was much higher than the client's initial max recv offset. By explicitly setting the max send and recv offsets at engine creation, this issue can be avoided.

### 27th FEB 2022

* *1.50 hours* I was seeing some delays still present in the QUIC implementations as well as some failures on the PPS QUIC runs. Experimenting with the parameter values for max streams and max stream data has fixed the issue, further testing shows no problems.
* *1.00 hours* Previously, we would monitor the server gst-launch process and kill the other threads when this process finished or after a timeout has passed. However, it makes more sense to have the client inidicate end of execution, since the server side pipeline will always finish before the client. I swapped the controlling threads and confirmed that this functioned as desired.
* *2.50 hours* The QUIC client elements have issues ending execution when the server closes the connection due. I spent time unifying the server elements tear down process and fixed some multithreading issues on the client side. There was also an issue where lsquic would not send a connection close frame, I made a tweak to the lsquic src code to ensure that the connection close frame was always sent, resolving this issue. I also utilised the delay_onclose setting to ensure that the server would not close the connection while there were outstanding retransmissions.
* *1.00 hours* Looked into an issue where tcp would slow its sending rate on high loss tests but lsquic would not. This turned out to be due to a difference in congestion control protocols. TCP was using cubic and so every packet loss registered as congestion on the network, resulting in a lowered sending rate. LSQUIC on the other hand uses BBR by default, and so the sending rate is tied to the RTT. I am still considering which I should use for the final run.
* *2.50 hours* Began making necessary changes to stack_latency_analysis scripts to work with QUIC streams. While testing I noticed that the tcp analysis was crashing due to lack of memory, so I improved the memory effieciency by leaving the payloads as a python list. The tcp analysis was also taking a long time to run, but this was due to extra data added by the pre-test iperf command. Switching this back to UDP solved this issue.


### 28th FEB 2022

* *4.00 hours* Finished making additions to quic stream stack latency analysis code.
* *8.00 hours* Worked on pipelines used in tests to ensure the following:
  - frames do not arrive at encoder at a faster rate than 24 frames per second
  - Packets are not considered lost by jitterbuffer before they should be
  - There are no unnessecary reading delays in the tcpsrc and quicsrc (ss) elements


### 1st MAR 2022

* *12.50 hours* Another long day of debuggin before the final test:
  - The previous changes to the jitter buffer were flawed, leading to packets being declared lost far too late. This was fixed. 
  - Due to the changes in pipeline, many rtp packets are smaller than the limit of 1400 bytes. This lead to parts of multiple streams being sent in the same datagram. This behaviour is undesired as we do not want a single datagram loss to impact multiple rtp packets for QUIC PPS. I made some modifications to the lsquic source code to prevent this.
  - The initial mtu used by QUIC is 1427 which does not leave enough room for a full 1400 byte rtp packet. This values increases via probing, but this is delayed on high loss, high latency networks. Since we know the MTU will be 1500, we can set this value initially to avoid this.
  - Investigated an issue where lsquic would not retransmit packets for up to several seconds! Lsquicuses BBR for congestion control, once it exits the start up phase and enters RTT probe mode, the congestion window decreases in size for a substantial period of time on high latency, high loss links. This should prevent any packets from being sent, but new packets are sent regardless. However, retransmissions are delayed. Since I am unable to force the new packets to experience the same delay (and im not sure if this behaviour is even correct) I have opted to rmove the delay from retransmissions. 
  - Investigated app latency delays on high latency tcp during beginning of each run. This appears to be due to congestion control.

### 2nd MAR 2022

* *3.00 hours* The rtp jitter buffer was still not behaving properly, occassionally packets would be considered lost too early and overall it was waiting 0.042ms too long. This turned out to be an issue with the timers and the delay parameter. I have fixed this now.
* *1.00 hours* Inspected final dry run before kicking off full tests.
* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *1.00 hours* while examining test logs I noticed that frames were not arriving at the enocder at the desired 24 fps rate. I made some modifications to the identity element:
  - Record time at which first frame passes through identity element (FBRT)
  - For each subsequent frame add the PTS to FBRT and requests a wait until this time. This ensures that the 24 fps rate is acheived.
* *4.00 hours* During testing, I noticed that the jitter buffer would only start a timer for the first expected packet. When later packets arrives, other timers are added for the gap packets. This works fine for udp, but for transmissions with any form of HOL blocking (TCP, QUIC_SS) we will not see subsequent packets until the first missing packet arrives. This leads packets which should be considered late being accepted. I began modfifying the jitter buffer such that, for implementations marked HOL, a list of expected timers would be created. There are still some kinks to work out though.


### 3rd MAR 2022

* *0.5 hours* Fixed an issue with the stack latency analysis for TCP. It now ignores retransmission lines on the server side, as the rtp dissector does not work for retransmissions. We are only interested in the timestamp of the original transmission anyway, so this is not an issue. I have also added the payload data to the server shark output for tcp. This will be ignored by the analysis script. But if anything goes wrong with the RTP dissector, we will still have the data available.
* *0.25 hours* Looked into an issue with the QUIC PPS stack latency analysis. Some packets were showing as never arriving which shouldnt have been possible. However, the cause was actually an experiment I had been doing to see the affect of consecutive missed packets. The sink element was programmed such that it would not send packets 40 and 41 and this had not been recompiled.
* *8.00 hours* Finished work on HOL blocking jitter buffer modifications. This required a bit more work than expected as the jitterbuffer has no way of what the spacing is between packets, and desync can occur quickly. This could be hard coded but, unfortunately, the number of packets generated is not consistent between runs. To work around this, I implemented a function which would estimate the packet spacing based on the pts of previous packets (this is updated dynamically as the pipeline recieves more packets, This took a bit of finetuning but works well for the most part). I also had to work around the issue of the h264 rtp payloader not spacing out its packet delivery to the sink element. I modified so that, rather than sending all nal units immediately, it would space them evenly. This made it possible for the HOL jitterbuffer to calculate the packet spacing based on the PTS of received packets.
* *4.00 hours*  While testing the HOL jitterbuffer, I noticed that the QUIC SS was not sending data immediately, leading to packets being delayed. I modified lsquic to allow packets to be sent immediately, similar to the TCP_NODELAY option. This lead to some test failures as the final STREAM frame and its associated FIN frame were sent in seperate packets, which gstquicsrc was not prepared for. After fixing this issue, great improvement could be seen in the SS QUIC runs.


### 4th MAR 2022

* *8.00 hours* Two issues arose in the overnight tests for the new HOL jitterbuffer implementation. 
  - 1) The inconsistent packets spacing leads to a desync in the deadline expected by the jitterbuffer and the time that packets would actually be late. I decided to hard code the packet spacing so that there would be no desyncronisation. Unfortunately, the current encoder settings were leading to a different number of packets being output each time, so it wasn't possible to predict the correct deadlines ahead of time. I experimented with different decoder settings and found that constant quantification would produce consistent results. I then recorded the pts of each rtp packet generated and used this to provide the correct packet spacing to the jitter buffer ahead of time. 
  - 2) The jitterbuffer was not always using the correct base time for its timers. I am could not determine the cause and so opted to record the arrival time of the first packet, and use this as a base for all future timers. In the case that the first packet has not arrived, the original logic is used.


### 5th MAR 2022

* *2.00 hours* Finished work on frame latency analysis scripts.
* *1.00 hours* Fixed a bug where the jitterbuffer would double the expected arrival time of a missing packet, leading to large delays.
* *3.00 hours* Fixed an issue where lsquic would continue to send new streams during the BBR RTT ptobing phase, extending the phase greatly. This is actually what caused a previous the retx time to be much higher than it should originally, so that fix has been removed in favour of preventing lsquic from sending data when new streams are created if we are in PROBE RTT mode. I have also turned off lsquic's pacer as we do our own pacing.
* *4.00 hours* During PROBE RTT periods, QUIC PPS will stop sending, this introduces a delay adn the jitterbuffer will not notice late packets as subsequent packets will not arrive. I attempted to modify the HOL code to work with QUIC PPS to prevent this issue. However, I could not get this to work and decided that the impact of the PROBE RTT periods on jitterbuffers would be an interesting talking point, so i reverted to the old method.
* *1.00 hours* An issue began appearing where lsquic would not send packets before it recieived an acknowledgement of the last handshake packet. This was a problem as the jitterbuffer relies on the arrival of the first packet for timing info. I have added extra code to the identity such that it delays the first buffer by 200ms, enough time to recieved the handshake. This required the additon of a second identity element, to keep nal units in sync with the inital 200ms delay.

### 6th MAR 2022

* *3.00 hours* Sketched out structure for the dissertation and wrote introduction.

### 7th MAR 2022

* *9.00 hours* Wrote first draft of dissertation for review by Colin.

### 13th MAR 2022

* *8.00 hours* Re wrote dissertation so that the "why" is made clear before tackling the "how".

### 16th MAR 2022

* *3.00 hours* Created QUIC_FPS elements. In the process I noticed a flaw in the QUIC_GOP elements, they would not pass data to the app until the full stream has closed. Reworked both elements such that data would be passed immediately.
* *0.50 hours* Meeting with Colin
* *0.25 hours* Added minutes and plan to appropriate meeting file
* *3.00 hours* Began adapting the jitterbuffer so that it would work with the new QUIC_GOP and QUIC_FPS elements. Previously, for elements that can have the potential for HOL blocking, the jitter buffer assumed that all packets would arrive in order.

### 17th MAR 2022

* *6.00 hours* Finished adapting jitter buffer, greatly improving the handling of delayed or lost packets.
* *2.00 hours* Fixed bug where frames would be delayed by 1/24 of a second before transmission.

### 18th MAR 2022

* *4.00 hours* Completed final checks before launching another test run. the jitterbuffer was struggling with creating timers for a full frames worth of packets in the scenario that all PTS values were the same. I have updated the jitterbuffer such that it will wait atleast 2/3 of a millisecond before starting the next timer.

## 22nd MAR 2022

* *12.00 hours* Reviewed results from final test runs. I noticed unexpected delays on TCP, QUIC_SS, QUIC_GOP and QUIC_FPS. QUIC_GOP, QUIC_FPS, QUIC_SS issues were infrequent and difficult to reproduce but I eventually found that the SFCW limit was being reached repeatedly. The values that I had uncovered previously were no longer appropriate for the new BBB video so I updated these values such that no flow control issues would occur. Additionally, the rtpstreamdepay element was introducing unexplained delays. I modified TCP and QUIC_SS elements such that they would handle the stream payloading (RFC 4571) manually, similar to how QUIC_GOP and QUIC_FPS operate. This removed the delays and after confirming everything was working as expected, I reran the tests for the affected implementations.

## 26th MAR 2022

* *1.50 hours* Fixed bug in stack latency analysis for TCP. The tshark rtp packet subdissector appears to fail even on the server side in the event of loss. Our `extract_rtp_data_from_tcp_stream_available` method used on the client side works without issue, so this is now used instead.
* *2.00 hours* Fixed two bugs in the stack latency available analysis for QUIC: 
  - Packets which arrived at the client side twice were causing issues, so I added a list which tracks the offsets of every stream frame recieved. If the stream frame has already been receieved, it is ignored. 
  - The code was not properly handling the scenario where two consecutive packets were lost, and the second of the two arrived first. The code previosly only attempted to add new stream frames to the end of existing chunks, and did not consider the scenario where a new frame could not be added to the beginning of any existing chunks but could be added to the beginning. Code to handle this case has now been added.

* *4.00 hours* Added code necessary to produce graphs of sending rate and stack latency in terms of availability. 


## 27th MAR 2022

* *12.00 hours* Worked on dissertation

## 28th MAR 2022

* *14.00 hours* Worked on dissertation

## 29th MAR 2022

* *14.00 hours* Worked on dissertation

## 30th MAR 2022

* *2.00 hours* Worked on dissertation
* *1.00 hours* Final meeting with Colin
* *9.00 hours* Cleaned up code upon learning this was to be submitted as well.


## 31st MAR 2022

* *16.00 hours* Worked on dissertation

## 1st MAR 2022 

* *18.00 hours* Finished work on presentation and submitted project