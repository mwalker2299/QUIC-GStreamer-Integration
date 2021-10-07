# Questions Prepared for Meeting Conducted at 15:30 on 30/09/21

## Questions

Platform restrictions? - Needs to work on linux for mininet testing

Does Colin have much experience with GStreamer? - No


What implementation would you recommend from the following:

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

