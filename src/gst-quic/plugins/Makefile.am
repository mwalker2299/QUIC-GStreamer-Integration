plugin_LTLIBRARIES = libgstquic.la

# sources used to compile this plug-in
libgstquic_la_SOURCES = gstquic.c gstquicsrc.c gstquicsrc.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstquic_la_CFLAGS = $(GST_CFLAGS) \
-I${LSQUIC}/include  -D_GNU_SOURCE
libgstquic_la_LIBADD = $(GST_LIBS) \
-L${LSQUIC}/src/liblsquic -llsquic
libgstquic_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

