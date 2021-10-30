/* GStreamer
 * Copyright (C) 2021 Matthew Walker <mjwalker2299@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstquicsrc
 *
 * The quicsrc element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! quicsrc ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstquicsrc.h"
#include "gstquicutils.h"

#define QUIC_CLIENT 0
#define QUIC_DEFAULT_PORT 12345
#define QUIC_DEFAULT_HOST "127.0.0.1"

//TODO: understand this
#if defined(IP_RECVORIGDSTADDR)
#   define DST_MSG_SZ sizeof(struct sockaddr_in)
#else
#   define DST_MSG_SZ sizeof(struct in_pktinfo)
#endif

#define ECN_SZ CMSG_SPACE(sizeof(int))

/* Amount of space required for incoming ancillary data */ 
#define CTL_SZ (CMSG_SPACE(MAX(DST_MSG_SZ, \
                                    sizeof(struct in6_pktinfo))) + ECN_SZ)

GST_DEBUG_CATEGORY_STATIC (gst_quicsrc_debug_category);
#define GST_CAT_DEFAULT gst_quicsrc_debug_category

/* pad templates */

static GstStaticPadTemplate gst_quicsrc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* property templates */

enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT
};


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstQuicsrc, gst_quicsrc, GST_TYPE_PUSH_SRC,
    GST_DEBUG_CATEGORY_INIT (gst_quicsrc_debug_category, "quicsrc", 0,
        "debug category for quicsrc element"));

/* prototypes */

static void gst_quicsrc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_quicsrc_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static void gst_quicsrc_dispose (GObject * object);
static void gst_quicsrc_finalize (GObject * object);

static GstCaps *gst_quicsrc_get_caps (GstBaseSrc * src, GstCaps * filter);

static gboolean gst_quicsrc_start (GstBaseSrc * src);
static gboolean gst_quicsrc_stop (GstBaseSrc * src);

static gboolean gst_quicsrc_unlock (GstBaseSrc * src);
static gboolean gst_quicsrc_unlock_stop (GstBaseSrc * src);
static gboolean gst_quicsrc_event (GstBaseSrc * src, GstEvent * event);

static GstFlowReturn gst_quicsrc_create (GstPushSrc * src, GstBuffer ** outbuf);

/* Quic functions */
static lsquic_conn_ctx_t *gst_quicsrc_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn);
static void gst_quicsrc_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status);
static void gst_quicsrc_on_conn_closed (struct lsquic_conn *conn);
static lsquic_stream_ctx_t *gst_quicsrc_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream);
static size_t gst_quicsrc_readf (void *ctx, const unsigned char *data, size_t len, int fin);
static void gst_quicsrc_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsrc_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsrc_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsrc_set_ecn_ancillary_data (struct msghdr *msg, const struct lsquic_out_spec *spec,
                                                unsigned char *ancillary_buf, size_t ancillary_buf_size);
static int gst_quicsrc_send_packets (void *packets_out_ctx, const struct lsquic_out_spec *packet_specs,
                                     unsigned packet_count);

/* QUIC callbacks passed to quic engine */
static struct lsquic_stream_if quicsrc_callbacks =
{
    .on_new_conn        = gst_quicsrc_on_new_conn,
    .on_hsk_done        = gst_quicsrc_on_hsk_done,
    .on_conn_closed     = gst_quicsrc_on_conn_closed,
    .on_new_stream      = gst_quicsrc_on_new_stream,
    .on_read            = gst_quicsrc_on_read,
    .on_write           = gst_quicsrc_on_write,
    .on_close           = gst_quicsrc_on_close,
};

static void
gst_quicsrc_class_init (GstQuicsrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *push_src_class = (GstPushSrcClass *) klass;

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_quicsrc_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "QUIC packet receiver", "Source/Network", "Receive packets over the network via QUIC",
      "Matthew Walker <mjwalker2299@gmail.com>");

  gobject_class->set_property = gst_quicsrc_set_property;
  gobject_class->get_property = gst_quicsrc_get_property;
  gobject_class->dispose = gst_quicsrc_dispose;
  gobject_class->finalize = gst_quicsrc_finalize;

  g_object_class_install_property (gobject_class, PROP_HOST,
      g_param_spec_string ("host", "Host",
          "The server IP address to connect to", QUIC_DEFAULT_HOST,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "Port", "The port used by the quic server", 0,
          G_MAXUINT16, QUIC_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_quicsrc_get_caps);

  base_src_class->start = GST_DEBUG_FUNCPTR (gst_quicsrc_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_quicsrc_stop);

  base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_quicsrc_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_quicsrc_unlock_stop);
  base_src_class->event = GST_DEBUG_FUNCPTR (gst_quicsrc_event);

  push_src_class->create = GST_DEBUG_FUNCPTR (gst_quicsrc_create);
}

static void
gst_quicsrc_init (GstQuicsrc * quicsrc)
{
  quicsrc->port = QUIC_DEFAULT_PORT;
  quicsrc->host = g_strdup (QUIC_DEFAULT_HOST);
  quicsrc->socket = -1;
  quicsrc->connection_active = FALSE;
  quicsrc->engine = NULL;
  quicsrc->connection = NULL;

  quicsrc->offset = 0;
  quicsrc->buffer = malloc(100000);
}

static void
gst_quicsrc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);

  GST_DEBUG_OBJECT (quicsrc, "set_property");

  switch (property_id) {
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (quicsrc->host);
      quicsrc->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_PORT:
      quicsrc->port = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_quicsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);
  GST_DEBUG_OBJECT (quicsrc, "get_property");

  switch (prop_id) {
    case PROP_HOST:
      g_value_set_string (value, quicsrc->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, quicsrc->port);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

void
gst_quicsrc_dispose (GObject * object)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);

  GST_DEBUG_OBJECT (quicsrc, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_quicsrc_parent_class)->dispose (object);
}

void
gst_quicsrc_finalize (GObject * object)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);

  GST_DEBUG_OBJECT (quicsrc, "finalize");

  lsquic_global_cleanup();

  /* clean up object here */

  G_OBJECT_CLASS (gst_quicsrc_parent_class)->finalize (object);
}

/* Provides the basesrc with possible pad capabilities
   to allow it to respond to queries from downstream elements.
   Since the data transmitted over QUIC could be of any type, we 
   return either ANY caps or, if provided, the caps of the
   downstream element */
static GstCaps *
gst_quicsrc_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstQuicsrc *quicsrc;
  GstCaps *caps = NULL;

  quicsrc = GST_QUICSRC (src);

  caps = (filter ? gst_caps_ref (filter) : gst_caps_new_any ());

  GST_DEBUG_OBJECT (quicsrc, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (caps));
  return caps;
}

static lsquic_conn_ctx_t *
gst_quicsrc_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (stream_if_ctx);
  GST_DEBUG_OBJECT(quicsrc,"MW: Connection created");
  quicsrc->connection_active = TRUE;
  lsquic_conn_make_stream(conn);
  return (void *) quicsrc;
}

static void
gst_quicsrc_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status) 
{
  GstQuicsrc *quicsrc = GST_QUICSRC ((void *) lsquic_conn_get_ctx(conn));

    switch (status)
    {
    case LSQ_HSK_OK:
    case LSQ_HSK_RESUMED_OK:
        GST_DEBUG_OBJECT(quicsrc, "MW: Handshake completed successfully");
        break;
    default:
        GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
          ("Handshake with server failed"));
        break;
    }
}

static void
gst_quicsrc_on_conn_closed (struct lsquic_conn *conn)
{
    GstQuicsrc *quicsrc = GST_QUICSRC ((void *) lsquic_conn_get_ctx(conn));

    GST_DEBUG_OBJECT(quicsrc, "MW: Connection closed, send EOS on next create call");
    quicsrc->connection_active = FALSE;
}

static lsquic_stream_ctx_t *
gst_quicsrc_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream)
{
    GstQuicsrc *quicsrc = GST_QUICSRC (stream_if_ctx);
    GST_DEBUG_OBJECT(quicsrc, "MW: created new stream");
    //FIXME: For now, as a test, we want to write a string initially and receive our text back
    lsquic_stream_wantwrite(stream, 1);
    return (void *) quicsrc;
}

static size_t
gst_quicsrc_readf (void *ctx, const unsigned char *data, size_t len, int fin)
{
    struct lsquic_stream *stream = (struct lsquic_stream *) ctx;
    GstQuicsrc *quicsrc = GST_QUICSRC ((void *) lsquic_stream_get_ctx(stream));

    if (len) 
    {
        quicsrc->offset = len;
        memcpy(quicsrc->buffer, data, len);
        GST_DEBUG_OBJECT(quicsrc, "MW: Read from stream");
    }
    if (fin)
    {
        fflush(stdout);
        GST_DEBUG_OBJECT(quicsrc, "MW: Read end of stream, expect connection close soon");
        lsquic_stream_shutdown(stream, 2);
    }
    return len;
}

static void
gst_quicsrc_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
    GstQuicsrc *quicsrc = GST_QUICSRC (stream_ctx);
    ssize_t bytes_read;

    bytes_read = lsquic_stream_readf(stream, gst_quicsrc_readf, stream);
    if (bytes_read < 0)
    {
        GST_ELEMENT_ERROR (quicsrc, RESOURCE, READ, (NULL),
        ("Error while reading from a QUIC stream"));
    }
}

static void
gst_quicsrc_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
    ssize_t bytes_written;

    lsquic_conn_t *conn = lsquic_stream_conn(stream);
    GstQuicsrc *quicsrc = GST_QUICSRC (stream_ctx);

    GST_DEBUG_OBJECT(quicsrc, "MW: writing stream");

    char test_string[13] = "tattarrattat";

    bytes_written = lsquic_stream_write(stream, test_string, sizeof(test_string));
    if (bytes_written > 0)
    {
        if (bytes_written == sizeof(test_string))
        {
            GST_DEBUG_OBJECT(quicsrc, "MW: wrote full test string to stream");
            lsquic_stream_shutdown(stream, 1);  /* This flushes as well */
            lsquic_stream_wantread(stream, 1);
        }
        else
        {
            GST_ELEMENT_ERROR (quicsrc, RESOURCE, WRITE,
              (NULL),
              ("Couldn't write all of the test string"));
        }
    }
    else
    {
        GST_DEBUG_OBJECT(quicsrc, "stream_write() wrote zero bytes. This should not be possible and indicates a serious error. Aborting conn");
        lsquic_conn_abort(conn);
    }
}

static void
gst_quicsrc_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
    GstQuicsrc *quicsrc = GST_QUICSRC (stream_ctx);
    GST_DEBUG_OBJECT(quicsrc, "MW: stream closed");
    lsquic_conn_close(lsquic_stream_conn(stream));
}

static void
gst_quicsrc_set_ecn_ancillary_data (struct msghdr *msg, const struct lsquic_out_spec *spec,
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
static int
gst_quicsrc_send_packets (void *packets_out_ctx, const struct lsquic_out_spec *packet_specs,
                                                                unsigned packet_count)
{
    GstQuicsrc *quicsrc = GST_QUICSRC (packets_out_ctx);
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
            gst_quicsrc_set_ecn_ancillary_data(&msg, &packet_specs[current_packet], ancillary_data.buf,
                                                    sizeof(ancillary_data.buf));
        else
        {
            msg.msg_control    = NULL;
            msg.msg_controllen = 0;
        }

        sendmsg_result = sendmsg(socket, &msg, 0);
        if (sendmsg_result < 0)
        {
            GST_WARNING_OBJECT(quicsrc, "sendmsg failed: %s", strerror(errno));
            break;
        }
    }

    if (current_packet < packet_count)
        GST_WARNING_OBJECT(quicsrc, "Not all packets could be sent");

    if (current_packet > 0)
        return current_packet;
    else
    {
        return -1;
    }
}

//TODO: understand and rewrite the following two functions
static void
gst_quicsrc_read_ancillary_data (struct msghdr *msg, struct sockaddr_storage *storage,
                                                                    int *ecn)
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
static void
gst_quicsrc_read_socket (GstQuicsrc * quicsrc)
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
    nread = recvmsg(quicsrc->socket, &msg, 0);
    if (-1 == nread) {
        if (!(EAGAIN == errno || EWOULDBLOCK == errno))
            GST_WARNING_OBJECT(quicsrc, "recvmsg error: %s", strerror(errno));
        lsquic_engine_process_conns(quicsrc->engine);
        return;
    }

    local_sas = quicsrc->local_address;
    ecn = 0;
    gst_quicsrc_read_ancillary_data(&msg, &local_sas, &ecn);

    (void) lsquic_engine_packet_in(quicsrc->engine, buf, nread,
        (struct sockaddr *) &local_sas,
        (struct sockaddr *) &peer_sas,
        (void *) (uintptr_t) quicsrc->socket, ecn);

    lsquic_engine_process_conns(quicsrc->engine);
}

/* Open a connection with the QUIC server */
static gboolean
gst_quicsrc_start (GstBaseSrc * src)
{
  socklen_t socklen;
  struct lsquic_engine_api engine_api;
  struct lsquic_engine_settings engine_settings;
  server_addr_u server_addr;

  FILE *s_log_fh = stderr;

  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "start");

  if (0 != lsquic_global_init(LSQUIC_GLOBAL_CLIENT))
  {
    GST_ELEMENT_ERROR (quicsrc, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  // /* Initialize logging */
  // if (0 != lsquic_set_log_level("debug"))
  // {
  //   GST_ELEMENT_ERROR (quicsrc, LIBRARY, INIT,
  //       (NULL),
  //       ("Failed to initialise lsquic"));
  //   return FALSE;
  // }

  // setvbuf(s_log_fh, NULL, _IOLBF, 0);
  // lsquic_logger_init(&logger_if, s_log_fh, LLTS_HHMMSSUS);

  // Initialize engine settings to default values
  lsquic_engine_init_settings(&engine_settings, QUIC_CLIENT);

  // Parse IP address and set port number
  if (!gst_quic_set_addr(quicsrc->host, quicsrc->port, &server_addr))
  {
      GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to resolve host: %s", quicsrc->host));
      return FALSE;
  }

  // Create socket
  quicsrc->socket = socket(server_addr.sa.sa_family, SOCK_DGRAM, 0);

  if (quicsrc->socket < 0)
  {
    GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
      ("Failed to open socket"));
    return FALSE;
  }

  // set socket to be non-blocking
  int socket_flags = fcntl(quicsrc->socket, F_GETFL);
  if (socket_flags == -1) 
  {
    GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to retrieve socket_flags using fcntl"));
    return FALSE;
  }

  socket_flags |= O_NONBLOCK;
  if (0 != fcntl(quicsrc->socket, F_SETFL, socket_flags)) 
  {
    GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to set socket_flags using fcntl"));
    return FALSE;
  }

  // Activate IP_TOS field manipulation to allow ecn to be set
  int activate, socket_opt_result;

  activate = 1;
  if (AF_INET == server_addr.sa.sa_family)
  {
      socket_opt_result = setsockopt(quicsrc->socket, IPPROTO_IP, IP_RECVTOS, &activate, sizeof(activate));
  }
  else
  {
      socket_opt_result = setsockopt(quicsrc->socket, IPPROTO_IPV6, IPV6_RECVTCLASS, &activate, sizeof(activate));
  }

  if (0 != socket_opt_result) {
    GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add ecn support using setsockopt"));
    return FALSE;
  }

  // Bind local address to socket.
  quicsrc->local_address.ss_family = server_addr.sa.sa_family;
  socklen = sizeof(quicsrc->local_address);
  if (0 != bind(quicsrc->socket,
                  (struct sockaddr *) &(quicsrc->local_address), socklen))
  {
      GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to bind socket"));
      return FALSE;
  }

  // Initialize engine callbacks
  memset(&engine_api, 0, sizeof(engine_api));
  engine_api.ea_packets_out = gst_quicsrc_send_packets;
  engine_api.ea_packets_out_ctx = quicsrc;
  engine_api.ea_stream_if   = &quicsrc_callbacks;
  engine_api.ea_stream_if_ctx = quicsrc;
  engine_api.ea_settings = &engine_settings;

  // Instantiate engine in client mode
  quicsrc->engine = lsquic_engine_new(QUIC_CLIENT, &engine_api);
  if (!quicsrc->engine)
  {
      GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to create lsquic engine"));
      return FALSE;
  }


  GST_DEBUG_OBJECT(quicsrc, "Creating connection to server at %s:%d", quicsrc->host, quicsrc->port);
  // Create connection
  quicsrc->connection = lsquic_engine_connect(
      quicsrc->engine, LSQVER_I001,
      (struct sockaddr *) &(quicsrc->local_address), &server_addr.sa,
      (void *) (uintptr_t) quicsrc->socket,  /* Peer ctx */
      NULL, NULL, 0, NULL, 0, NULL, 0);
  if (!quicsrc->connection)
  {
    GST_ELEMENT_ERROR (quicsrc, RESOURCE, OPEN_READ, (NULL),
      ("Failed to create QUIC connection to server"));
      return FALSE;
  }

  lsquic_engine_process_conns(quicsrc->engine);
  

  return TRUE;
}

static gboolean
gst_quicsrc_stop (GstBaseSrc * src)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "stop");

  return TRUE;
}

// /* given a buffer, return start and stop time when it should be pushed
//  * out. The base class will sync on the clock using these times. */
// static void
// gst_quicsrc_get_times (GstBaseSrc * src, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end)
// {
//   GstQuicsrc *quicsrc = GST_QUICSRC (src);

//   GST_DEBUG_OBJECT (quicsrc, "get_times");

// }

// /* get the total size of the resource in bytes */
// static gboolean
// gst_quicsrc_get_size (GstBaseSrc * src, guint64 * size)
// {
//   GstQuicsrc *quicsrc = GST_QUICSRC (src);

//   GST_DEBUG_OBJECT (quicsrc, "get_size");

//   return TRUE;
// }

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_quicsrc_unlock (GstBaseSrc * src)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "unlock");

  return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean
gst_quicsrc_unlock_stop (GstBaseSrc * src)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "unlock_stop");

  return TRUE;
}

/* notify subclasses of an event */
static gboolean
gst_quicsrc_event (GstBaseSrc * src, GstEvent * event)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "event");

  return TRUE;
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
static GstFlowReturn
gst_quicsrc_create (GstPushSrc * src, GstBuffer ** outbuf)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);
  GstMapInfo map;

  if (!quicsrc->connection_active) {
    return GST_FLOW_EOS;
  }

  gst_quicsrc_read_socket(quicsrc);

  *outbuf = gst_buffer_new_and_alloc(quicsrc->offset);
  gst_buffer_map (*outbuf, &map, GST_MAP_READWRITE);

  memcpy(map.data, quicsrc->buffer, quicsrc->offset);

  gst_buffer_unmap(*outbuf, &map);
  gst_buffer_resize(*outbuf, 0, quicsrc->offset);

  GST_LOG_OBJECT (quicsrc, "create");

  return GST_FLOW_OK;
}
