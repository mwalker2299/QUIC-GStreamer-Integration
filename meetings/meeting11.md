# Meeting Conducted at 14:00 on 12/01/22

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Completed goals for winter break:
  - Use Mininet's Python API to create a dumbbell network topology which connects server and client nodes via switches. **Deliverable:**
  One or more Python files which can instantiate the above network topology, set parameters on the links between nodes, start the Gstreamer pipelines on the client and server host nodes, and save the output of the Gstreamer and LSQUIC logs.
  - Add support for packet capture to existing Mininet test framework. **Deliverable:**
  As above but with tshark packet capture capabilities.
  - Select possible parameter combinations for the switch node (e.g. drop rate = 0\%,1\%,10\%). **Deliverable:** A list of possible parameter combinations.
  - Add a test runner which will run through each parameter combination for each pair of QUIC elements and the existing pair of UDP elements. **Deliverable:** A test runner written in Python which can setup, run and teardown our Mininet network.


## Minutes:

* Brief catch up about how our breaks were
* Reported the accomplishment of my goals for the break.
* Mentioned a bug I am looking into with the evaluation framework where QUIC runs fail at high loss.
* Ran through brief overview of plans for semester (See status report).
* plan to determine analysis and parameter decisions by next week (what should be extracted, what should we test)



