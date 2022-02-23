#ifndef GST_QUIC_UTILS_H
#define GST_QUIC_UTILS_H


#include <stddef.h>
#include <stdio.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <lsquic.h>

#include <gst/gst.h>

/* We expect the ancillary control message data to contain the original
   destination address and the value of IP_TOS field from which we will 
   retrieve the ecn. 
   
   The TOS field is a byte long so we add the sizeof(char) to the CTL_SZ.

   The original destination address could come within sockaddr_in, 
   in_pktinfo or in6_pktinfo structs depending on the IP version as well
   as whether or not IP_RECVORIGDSTADDR is defined.
*/ 
#if defined(IP_RECVORIGDSTADDR)
#   define DST_MSG_SZ sizeof(struct sockaddr_in)
#else
#   define DST_MSG_SZ sizeof(struct in_pktinfo)
#endif

#define ECN_SZ CMSG_SPACE(sizeof(char))

#define CTL_SZ (CMSG_SPACE(MAX(DST_MSG_SZ, \
                                    sizeof(struct in6_pktinfo))) + ECN_SZ)

typedef union server_addr {
  struct sockaddr     sa;
  struct sockaddr_in  addr4;
  struct sockaddr_in6 addr6;
} server_addr_u;

struct server_stream_ctx
{
    gsize   offset;           /* Number of bytes written to stream */
    gsize   buffer_size;
    GstBuffer* buffer;        /* GstBuffer received from upstream */
};

gint gst_quic_log_buf (void *ctx, const char *buf, size_t len);

gboolean gst_quic_set_addr(gchar* host, guint16 port, server_addr_u *server_addr);

void gst_quic_set_ecn_ancillary_data (struct msghdr *msg, const struct lsquic_out_spec *spec,
   unsigned char *ancillary_buf, size_t ancillary_buf_size);

gint gst_quic_send_packets (void *packets_out_ctx, const struct lsquic_out_spec *packet_specs, unsigned packet_count);

void gst_quic_read_ancillary_data (struct msghdr *msg, struct sockaddr_storage *storage, int *ecn);

void gst_quic_read_packets (GstElement * quic_element, gint socket, lsquic_engine_t *engine, struct sockaddr_storage local_address);

const struct lsquic_logger_if logger_if;

#endif