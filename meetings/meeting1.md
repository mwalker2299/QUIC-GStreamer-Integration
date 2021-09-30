# Meeting Conducted at 15:30 on 30/09/21

## Status Report

- Source control created
- GStreamer installed and template elements have been created

## Questions

Platform restrictions?

Does Colin have much experience with GStreamer? 

What implementation would you recommend from the following:
* ngtcp2 
       - possibly not compatible with Windows 
       - TLS impl agnostic supports (Boring ssl, OpenSSl, GNUTls)
       - Supports all quic interop tests and passes
       - Better handshake results than tcp but variable performance in different scenarios

* msquic 
       - Support for all platforms but building is complicated 
       - Good documentation
       - Supports all or almost all quic features
       - best transfer speed for interopmatrix

* quiche 
       - Support for all platforms
       - builds a c library and header file for easy use in project
       - ok documentation
       

* lsquic 
       - Support for all platforms 
       - Builds a c library and header file for easy use in project (very few dependencies)
       - Great documentation
       - Supports all or almost all quic features
       - In a study, performed better than tcp once the connection was established, however it was a about the same for handshake time


## Minutes:


## Plan for next week: