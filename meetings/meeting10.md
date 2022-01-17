# Meeting Conducted at 15:30 on 16/12/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Fixed issues with playback that I had mentioned previosuly. Everything looks good during tests with no RTP warnings or errors in the GStreamer logs.
* Completed and submitted status report
* Began learning about Mininet. Have installed, completed walktrhough and I am working through the guide for getting started with the python API. I experimented with the CLI and have created some basic scripts which can instatiate the gstreamer pipeline and confirmed that tshark can be attached to a node.
* Had some issues with tshark when using CLI but it seems to work fine when using the python API.



## Minutes:

* Updated Colin on Current status (See above)
* Discussed plan for winter break (See below)
* Discussed timetable variability and set a preliminary meeting time for next semester.
* Discussed style of writing for dissertation. Colin has a github repo for project templates with advice on writing style.
* Discussed replacing tcpdump with tshark.
* Discussed contacting Colin's other mininet utilising student's. See below for contact info.
* Colin advised that I should script everything
* Send availability (early Jan)


https://www.gla.ac.uk/schools/computing/researchstudents/mihailyanev/
2292523 Ivan Nikitin

## Updated plan for winter break:

  - Use Mininet's Python API to create a dumbbell network topology which connects server and client nodes via switches. **Deliverable:**
  One or more Python files which can instantiate the above network topology, set parameters on the links between nodes, start the Gstreamer pipelines on the client and server host nodes, and save the output of the Gstreamer and LSQUIC logs.
  - Add support for packet capture to existing Mininet test framework. **Deliverable:**
  As above but with tshark packet capture capabilities.
  - Select possible parameter combinations for the switch node (e.g. drop rate = 0\%,1\%,10\%). **Deliverable:** A list of possible parameter combinations.
  - Add a test runner which will run through each parameter combination for each pair of QUIC elements and the existing pair of UDP elements. **Deliverable:** A test runner written in Python which can setup, run and teardown our Mininet network.


