# Meeting Conducted at 14:30 on 07/10/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Performed research further research into QUIC, GStreamer and Mininet to create plan/requirements.
* Created requirements for project and a plan
* Project requirements and plan were provided to supervisor.
* Looked into Quant, there is no real documentation of the API or how it works. 

## Minutes:

- Briefly discussed coaching other students
- Discussed requirements document. Requirements are good but more detail could be provided for testing phase. Colin provided links to documents containing recommended testing strategies and values. 
- Colin is happy with the plan for the first semester. However, Colin felt that before the second semester time should be taken to reflect on work done so far and determine which additional features or tests can be developed in the time remaining.
- Colin is happy for the LSQuic implementation to be used, but noted that I should provide justification. 
- I described my justification for LSQuic (Support for all platforms, compatible with GStreamer, substantial documentation + a tutorial for use, clear and simple api with easy configuration of extensions, regularly updated, supports all quic features and extensions which will be need by the project)
- Discussed how media type impacts the transfer of data. Audio and video would make a good point of comparison but individual video encodings will not have much impact.
- Suggestion for additional functionality: Utilise multiple QUIC streams when transferring video files. There are various strategies that could be employed. 


## Plan for next week:

- Experiment further with LSQuic to develop a better understanding of the library.
- Read documents provided by Colin for inspiration testing strategies and realistic values.
- Begin work on GStreamer element plugin for QUIC.
