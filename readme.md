# Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer

This is a 40 credit MSci research project required for the Software Engineering with Year Placement course.

* Student Name: Matthew Walker
* Student Matriculation: 2310528
* Project Supervisor: DR. Colin Perkins

## Project Overview

QUIC is a transport layer network protocol recently standardised by the IETF that provides secure and reliable delivery of packets. QUIC runs over the existing UDP protocol and was designed to solve the issues of Head-of-Line blocking and protocol ossification which plague TCP. Currently, there is no support for QUIC in GStreamer, an open-source framework for creating pipelines for use in media processing and streaming applications. As a result, we do not know how well QUIC performs in conjunction with GStreamer.

The goal of this project is to evaluate the effectiveness of the QUIC protocol when integrated into GStreamer. In order to accomplish this, two new GStreamer source and sink element plugins that utilise an existing implementation of the QUIC protocol will be created. Using these new plugins, the performance of QUIC will be assessed in a range of realistic scenarios and compared to the performance of other network protocols (UPD, TCP) using existing GStreamer plugins. This will show whether or not QUIC-based GStreamer elements can function appropriately and demonstrate in which scenarios, if any, QUIC is the appropriate network protocol to use in a GStreamer pipeline.

## Project Structure

* `timelog.md` The time log for the project (Total time logged is 580.75 hours).
* `data/` Contains data acquired during the project (evaluation).
* `src/` The source code for the project.
* `status_report/` Contains the status report to be submitted in December.
* `meetings/` Records of the meetings conducted during the project.
* `requirements/` Holds the requirements for the project
* `dissertation/` Contains all files needed for the final project dissertation
* `summer/` Contains info on the research conducted before the project began
* `submodules/` Contains submodules necessary to build the GStreamer element plugins and run the test framework.