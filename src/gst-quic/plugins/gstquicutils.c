#include "gstquicutils.h"

const struct lsquic_logger_if logger_if = { gst_quic_log_buf, };


gint gst_quic_log_buf (void *ctx, const char *buf, size_t len)
{
    FILE *out = ctx;
    fwrite(buf, 1, len, out);
    fflush(out);
    return 0;
}

gboolean gst_quic_set_addr(gchar* host, guint16 port, server_addr_u *server_addr) 
{
  if (inet_pton(AF_INET, host, &(server_addr->addr4.sin_addr)))
  {
      server_addr->addr4.sin_family = AF_INET;
      server_addr->addr4.sin_port   = htons(port);
      return TRUE;
  }
  else if (memset(&(server_addr->addr6), 0, sizeof(server_addr->addr6)),
      inet_pton(AF_INET6, host, &(server_addr->addr6.sin6_addr)))
  {
      server_addr->addr6.sin6_family = AF_INET6;
      server_addr->addr6.sin6_port   = htons(port);
      return TRUE;
  }
  else
  {
    return FALSE;
  }
} 

void gst_quic_set_ecn_ancillary_data (struct msghdr *msg, const struct lsquic_out_spec *spec,
   unsigned char *ancillary_buf, size_t ancillary_buf_size)
{
    struct cmsghdr *cmsg;

    msg->msg_control = ancillary_buf;
    msg->msg_controllen = ancillary_buf_size;

    memset(ancillary_buf, 0, ancillary_buf_size);

    cmsg = CMSG_FIRSTHDR(msg);

    if (cmsg) {
      // Select appropriate address family and enable ecn
      if (AF_INET == spec->dest_sa->sa_family)
      {
          const int tos = spec->ecn;
          cmsg->cmsg_level = IPPROTO_IP;
          cmsg->cmsg_type  = IP_TOS;
          cmsg->cmsg_len   = CMSG_LEN(sizeof(tos));
          memcpy(CMSG_DATA(cmsg), &tos, sizeof(tos));
          msg->msg_controllen += CMSG_SPACE(sizeof(tos));
      }
      else
      {
          const int tos = spec->ecn;
          cmsg->cmsg_level = IPPROTO_IPV6;
          cmsg->cmsg_type  = IPV6_TCLASS;
          cmsg->cmsg_len   = CMSG_LEN(sizeof(tos));
          memcpy(CMSG_DATA(cmsg), &tos, sizeof(tos));
          msg->msg_controllen += CMSG_SPACE(sizeof(tos));
      }
    } else {
      GST_WARNING("Could not retrive cmsg header from msghdr");
    }
}

/* Method used by lsquic engine to send quic packets via udp datagrams. Returns number of packets sent */
gint gst_quic_send_packets (void *packets_out_ctx, const struct lsquic_out_spec *packet_specs,
                                                                unsigned packet_count)
{
    GstElement *quic_element = GST_ELEMENT (packets_out_ctx);
    unsigned current_packet;
    int socket, sendmsg_result = 0;
    struct msghdr msg;
    union { 
        unsigned char buf[
            CMSG_SPACE(MAX(sizeof(struct in_pktinfo),
                sizeof(struct in6_pktinfo))) + CMSG_SPACE(sizeof(int))
        ];
        struct cmsghdr align;
    } ancillary_data;

    if (0 == packet_count)
        return 0;

    current_packet = 0;
    msg.msg_flags = 0; //TODO: Does this need to be set to zero?

    for (current_packet = 0; current_packet < packet_count; ++current_packet) 
    {
        socket                 = (int) (uint64_t) packet_specs[current_packet].peer_ctx;
        msg.msg_name       = (void *) packet_specs[current_packet].dest_sa;
        msg.msg_namelen    = (AF_INET == packet_specs[current_packet].dest_sa->sa_family ?
                                            sizeof(struct sockaddr_in) :
                                            sizeof(struct sockaddr_in6)),
        msg.msg_iov        = packet_specs[current_packet].iov;
        msg.msg_iovlen     = packet_specs[current_packet].iovlen;

        // Set up ancillary message
        if (packet_specs[current_packet].ecn)
            gst_quic_set_ecn_ancillary_data(&msg, &packet_specs[current_packet], ancillary_data.buf,
                                                    sizeof(ancillary_data.buf));
        else
        {
            msg.msg_control    = NULL;
            msg.msg_controllen = 0;
        }

        sendmsg_result = sendmsg(socket, &msg, 0);
        if (sendmsg_result < 0)
        {
            GST_WARNING_OBJECT(quic_element, "sendmsg failed: %s", strerror(errno));
            break;
        }
    }

    if (current_packet < packet_count)
        GST_WARNING_OBJECT(quic_element, "Not all packets could be sent");

    if (current_packet > 0)
        return current_packet;
    else
    {
        return -1;
    }
}

void gst_quic_read_ancillary_data (struct msghdr *msg, struct sockaddr_storage *storage, int *ecn)
{
    const struct in6_pktinfo *in6_pkt;
    struct cmsghdr *cmsg;

    for (cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg))
    {
        if (cmsg->cmsg_level == IPPROTO_IP &&
            cmsg->cmsg_type  ==
#if defined(IP_RECVORIGDSTADDR)
                                IP_ORIGDSTADDR
#else
                                IP_PKTINFO
#endif
                                              )
        {
#if defined(IP_RECVORIGDSTADDR)
            memcpy(storage, CMSG_DATA(cmsg), sizeof(struct sockaddr_in));
#else
            const struct in_pktinfo *in_pkt;
            in_pkt = (void *) CMSG_DATA(cmsg);
            ((struct sockaddr_in *) storage)->sin_addr = in_pkt->ipi_addr;
#endif
        }
        else if (cmsg->cmsg_level == IPPROTO_IPV6 &&
                 cmsg->cmsg_type  == IPV6_PKTINFO)
        {
            in6_pkt = (void *) CMSG_DATA(cmsg);
            ((struct sockaddr_in6 *) storage)->sin6_addr =
                                                    in6_pkt->ipi6_addr;
        }
        else if ((cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TOS)
                 || (cmsg->cmsg_level == IPPROTO_IPV6
                                            && cmsg->cmsg_type == IPV6_TCLASS))
        {
            memcpy(ecn, CMSG_DATA(cmsg), sizeof(*ecn));
            *ecn &= IPTOS_ECN_MASK;
        }
    }
}

/* Method which reads UDP datagrams from the socket associated with the QUIC connections. These packets are then passed 
   to the quic engine for processing. */
void gst_quic_read_packets (GstElement * quic_element, gint socket, lsquic_engine_t *engine, struct sockaddr_storage local_address)
{
    gssize nread;
    int ecn;
    struct sockaddr_storage peer_sas, local_sas;
    unsigned char buf[0x1000];
    struct iovec vec[1] = {{ buf, sizeof(buf) }};
    unsigned char ctl_buf[CTL_SZ];

    struct msghdr msg = {
        .msg_name       = &peer_sas,
        .msg_namelen    = sizeof(peer_sas),
        .msg_iov        = vec,
        .msg_iovlen     = 1,
        .msg_control    = ctl_buf,
        .msg_controllen = sizeof(ctl_buf),
    };

    nread = recvmsg(socket, &msg, 0);
    if (-1 == nread) {
        if (!(EAGAIN == errno || EWOULDBLOCK == errno))
            GST_WARNING_OBJECT(quic_element, "recvmsg error: %s", strerror(errno));
        lsquic_engine_process_conns(engine);
        return;
    }

    local_sas = local_address;
    ecn = 0;
    gst_quic_read_ancillary_data(&msg, &local_sas, &ecn);

    (void) lsquic_engine_packet_in(engine, buf, nread,
        (struct sockaddr *) &local_sas,
        (struct sockaddr *) &peer_sas,
        (void *) (uintptr_t) socket, ecn);

    lsquic_engine_process_conns(engine);
}
