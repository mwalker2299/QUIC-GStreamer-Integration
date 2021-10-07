# Plan

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* DR. Colin Perkins

Week-by-week plan for the whole project. Update this as you go along.

## Winter semester

* **Week 1 (Start Date: 27/SEP/2021)**

  - Set up source control repository for project.
  - Add project template to source control and insert details about the project.
  - Set up Linux VM for working with GStreamer.

* **Week 2 (Start Date: 04/OCT/21)**

  - Define requirements for the project.
  - Lay out plan for the project.

* **Week 3 (Start Date: 11/OCT/2021)**

  - Select a QUIC implementation for the project.
  - Experiment with QUIC implementation to develop a better understanding of the library.
  - Read documents provided by Colin for inspiration testing strategies and realistic values.

* **Week 4 (Start Date: 18/OCT/2021)**

  - Select the appropriate base class for GStreamer element plugin.
  - Create a GStreamer element plugin that inherits from the chosen class.
  - Add necessary GStreamer properties to store server address and transport parameters.
  - Add an include to the plugin for the QUIC implementation to ensure the plugin can access the implementation.
  - Ensure created plugin can be built and installed successfully.

* **Week 5 (Start Date: 25/OCT/2021)**

  - Add necessary functionality to allow QUIC client src element to initialise (enter NULL state).
  - Add necessary functionality to allow QUIC client src element to establish a connection (enter READY state).
  - Add necessary functionality to allow QUIC client src element to accept a QUIC stream from a connected server and set flow control to prevent data transfer (enter PAUSED state).

* **Week 6 (Start Date: 01/NOV/2021)**

  - Add necessary functionality to allow QUIC client src element to add data from a QUIC stream to GstBuffers and push these downstream (enter PLAYING state).
  - Add necessary functionality to allow QUIC client src element to handle QUIC protocol errors and unexpected stream and connection closures.
  - Add necessary functionality to allow QUIC client src element to close streams and connections early if a NULL transition is requested by the pipeline.

* **Week 7 (Start Date: 08/NOV/2021)**

  - Add extensions to the QUIC client src element (ECN, timestamps, DPLPMTUD)
  - Add ability to migrate to a new address to QUIC client src element.

* **Week 8 (Start Date: 15/NOV/2021)**

  - Add TLS 1.3 support to tcpclientsrc.

* **Week 9 (Start Date: 22/NOV/2021)**

  - Create Python program to instantiate and run pipelines for each protocol.
  - Create server applications for each protocol. (These can use existing example servers as a base to save time).

* **Week 10 (Start Date: 29/NOV/2021)**

  - Test that the pipelines created by the Python program can connect to the server applications.
  - Fix any issues that arise during testing.

* **Week 11 (Start Date: 06/DEC/2021)**

  - Begin development of Python Mininet testing framework.
  - (Time may be taken here to study for exams)

* **Week 12 (Start Date: 13/DEC/2021)**

  - Complete and hand in Status Report.

## Winter break

## Spring Semester

* **Week 13 (Start Date: 03/JAN/2022)**

  - Continue work on Mininet testing framework.

* **Week 14 (Start Date: 10/JAN/2022)**

  - Complete work on Mininet testing framework.

* **Week 15 (Start Date: 17/JAN/2022)**

  - Conduct a small test evaluation of the plugins for each protocol.

* **Week 16 (Start Date: 24/JAN/2022)**

  - Analyse results of initial test run and identify any issues that will require adjustments to the test framework.
  - Fix any issues with the test framework.

* **Week 17 (Start Date: 31/JAN/2022)**

  - Perform full evaluation of QUIC, TCP and TLS 1.3 over TCP plugins.

* **Week 18 (Start Date: 07/FEB/2022)**

  - Analyse results, creating graphs and providing explanations.

* **Week 19 (Start Date: 14/FEB/2022)**

  - Begin writing dissertation.

* **Week 20 (Start Date: 21/FEB/2022)**

  - Continue work on dissertation

* **Week 21 (Start Date: 28/FEB/2022)**

  - Continue work on dissertation

* **Week 22 (Start Date: 07/MAR/2022)**

  - Polish dissertation.

* **Week 23 (Start Date: 14/MAR/2022)**

  - Hand in dissertation and source code.

* **Week 24 (Start Date: 21/MAR/2022)**

  - Create a presentation describing work completed and results.
