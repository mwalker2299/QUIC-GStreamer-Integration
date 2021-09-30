# Meeting Conducted at 15:30 on 30/09/21

## Questions
What implementation would you recommend from the following:
* ngtcp2 
       - possibly not compatible with Windows (X)
       - TLS impl agnostic supports (Boring ssl, OpenSSl, GNUTls)
       - build didnt seem to work properly when I tried (X)
       - Supports all quic interop tests and passes
       - Better handshake results than tcp but variable performance in different scenarios


* quiche 
       - Support for all platforms
       - builds a c library and header file for easy use in project
       - Good documentation
       - best transfer speed for interopmatrix

* lsquic 
       - Support for all platforms (tick)
       - Builds a c library and header file for easy use in project (very few dependencies)
       - Great documentation
       - Supports all or almost all quic features
       - In a study, performed better than tcp once the connection was established, however it was a about the same for handshake time


## Minutes:


## Plan for next week: