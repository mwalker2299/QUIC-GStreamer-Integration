# Meeting Conducted at 15:30 on 30/09/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins

## Status Report

- Source control has been created. 
- Template project structure has been added and updated.
- Research has been performed on gstreamer element creation.
- Currently setting up a linux VM for work on the project. 

Current impl knowledge:

ngtcp2:
- possibly not compatible with Windows 
- TLS impl agnostic. Supports Boring ssl, OpenSSl, GNUTls
- passes all quic interop tests 
- Great documentation
- Need to perform more research on using this lib within a gstreamer plugin

msquic:
- Support for all platforms but building is complicated and use within gstreamer plugin will likely be as well
- Good documentation
- Seems to support most, if not all, quic features
- best crosstraffic transfer speed for interopmatrix

quiche:
- Support for all platforms
- builds a c library and header file for easy use in project
- documentation is not great
- aspects of QUIC supported are unclear
       
lsquic:
- Support for all platforms 
- Builds a c library and header file for easy use in project (very few dependencies)
- Great documentation
- Seems to support most, if not all, quic features



## Minutes:

* Year placement was discussed briefly.
* Discussed issues getting GStreamer to work on MacOS as well as solution of Linux VM
* Discussed options for quic implementation (lsquic, ngtcp2, msquic and quiche are currently being considered. Supervisor suggested looking into Quant aswell
* Decided that a good way to identify the projects quic implementation was to create an interop matrix using each considered implementations. This should also aid to improve the students understanding of QUIC in general.
* Supervisor mentioned that he would be unable to provide assistance with GStreamer but could provide resources to help with Mininet.
* Agreed that the next best step was to come up with specific requirements and plan for carrying out the project. 


## Plan for next week:

* Determine requirements of the project (e.g. for GStreamer, QUIC, evaluation with mininet).
* If time allows, begin work on quic implementation interop tests.