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
 * SECTION:element-gstquicsinkpps
 *
 * The quicsinkpps element acts as a QUIC server. This element is solely for 
 * experimental purposes. 
 * The element utilises one stream per rtp packet sent.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
   * gst-launch-1.0 -v filesrc location=\"test.mp4\" ! qtdemux name=demux  demux.video_0  ! rtph264pay seqnum-offset=0 mtu=1400 ! quicsinkpps host=127.0.0.1 port=5000 keylog=SSL.keys sync=false"
 * ]|
 * Send rtp packets over the network.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "gstquicsinkpps.h"
#include "gstquicutils.h"

#define QUIC_SERVER 1
#define QUIC_DEFAULT_PORT 12345
#define QUIC_DEFAULT_HOST "127.0.0.1"
#define QUIC_DEFAULT_CERTIFICATE_PATH ""
#define QUIC_DEFAULT_KEY_PATH ""
#define QUIC_DEFAULT_LOG_PATH ""
#define QUIC_DEFAULT_KEYLOG_PATH ""

/* We are interested in the original destination address of received packets.
  The IP_RECVORIGDSTADDR flag can be set on sockets to allow this. However,
  if this flag is not defined, we shall use the more geneal IP_PKTINFO flag.
*/
#if defined(IP_RECVORIGDSTADDR)
#define IP_RECVDESTADDR_FLAG IP_RECVORIGDSTADDR
#else
#define IP_RECVDESTADDR_FLAG IP_PKTINFO
#endif

GST_DEBUG_CATEGORY_STATIC (gst_quicsinkpps_debug_category);
#define GST_CAT_DEFAULT gst_quicsinkpps_debug_category

/* gstreamer method prototypes */

static void gst_quicsinkpps_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_quicsinkpps_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_quicsinkpps_dispose (GObject * object);
static void gst_quicsinkpps_finalize (GObject * object);

static GstCaps *gst_quicsinkpps_get_caps (GstBaseSink * sink, GstCaps * filter);
static gboolean gst_quicsinkpps_start (GstBaseSink * sink);
static gboolean gst_quicsinkpps_stop (GstBaseSink * sink);
static gboolean gst_quicsinkpps_unlock (GstBaseSink * sink);
static gboolean gst_quicsinkpps_unlock_stop (GstBaseSink * sink);
static GstFlowReturn gst_quicsinkpps_preroll (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_quicsinkpps_render (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_quicsinkpps_render_list (GstBaseSink * sink,
    GstBufferList * buffer_list);


/* Quic functions */
static void gst_quicsinkpps_load_cert_and_key (GstQuicsinkpps * quicsinkpps);
static SSL_CTX *gst_quicsinkpps_get_ssl_ctx (void *peer_ctx);
static lsquic_conn_ctx_t *gst_quicsinkpps_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn);
static void gst_quicsinkpps_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status);
static void gst_quicsinkpps_on_conn_closed (struct lsquic_conn *conn);
static lsquic_stream_ctx_t *gst_quicsinkpps_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream);
static size_t gst_quicsinkpps_readf (void *ctx, const unsigned char *data, size_t len, int fin);
static void gst_quicsinkpps_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsinkpps_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsinkpps_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);

/* QUIC callbacks passed to quic engine */
static struct lsquic_stream_if quicsinkpps_callbacks =
{
    .on_new_conn        = gst_quicsinkpps_on_new_conn,
    .on_hsk_done        = gst_quicsinkpps_on_hsk_done,
    .on_conn_closed     = gst_quicsinkpps_on_conn_closed,
    .on_new_stream      = gst_quicsinkpps_on_new_stream,
    .on_read            = gst_quicsinkpps_on_read,
    .on_write           = gst_quicsinkpps_on_write,
    .on_close           = gst_quicsinkpps_on_close,
};

enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT,
  PROP_CERT,
  PROP_LOG,
  PROP_KEY,
  PROP_KEYLOG
};

/* pad templates */

static GstStaticPadTemplate gst_quicsinkpps_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstQuicsinkpps, gst_quicsinkpps, GST_TYPE_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT (gst_quicsinkpps_debug_category, "quicsinkpps", 0,
        "debug category for quicsinkpps element"));

static void
gst_quicsinkpps_class_init (GstQuicsinkppsClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_quicsinkpps_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "QUIC packet sender", "Sink/Network", "Send packets over the network via QUIC",
      "Matthew Walker <mjwalker2299@gmail.com>");

  gobject_class->set_property = gst_quicsinkpps_set_property;
  gobject_class->get_property = gst_quicsinkpps_get_property;
  gobject_class->dispose = gst_quicsinkpps_dispose;
  gobject_class->finalize = gst_quicsinkpps_finalize;

  g_object_class_install_property (gobject_class, PROP_HOST,
      g_param_spec_string ("host", "Host",
          "The IP address on which the server receives connections", QUIC_DEFAULT_HOST,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CERT,
      g_param_spec_string ("cert", "Cert",
          "The path to the SSL certificate file", QUIC_DEFAULT_CERTIFICATE_PATH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_KEY,
      g_param_spec_string ("key", "Key",
          "The path to the SSL private key file", QUIC_DEFAULT_KEY_PATH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_LOG,
      g_param_spec_string ("log", "Log",
          "The path to the lsquic log output file", QUIC_DEFAULT_LOG_PATH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_KEYLOG,
      g_param_spec_string ("keylog", "Keylog",
          "The path to the SSL keylog output file", QUIC_DEFAULT_KEYLOG_PATH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "Port", "The port used by the quic server", 0,
          G_MAXUINT16, QUIC_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_sink_class->get_caps = GST_DEBUG_FUNCPTR (gst_quicsinkpps_get_caps);
  // base_sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_quicsinkpps_set_caps);
  // base_sink_class->fixate = GST_DEBUG_FUNCPTR (gst_quicsinkpps_fixate);
  // base_sink_class->activate_pull =
  //     GST_DEBUG_FUNCPTR (gst_quicsinkpps_activate_pull);
  // base_sink_class->get_times = GST_DEBUG_FUNCPTR (gst_quicsinkpps_get_times);
  // base_sink_class->propose_allocation =
  //     GST_DEBUG_FUNCPTR (gst_quicsinkpps_propose_allocation);
  base_sink_class->start = GST_DEBUG_FUNCPTR (gst_quicsinkpps_start);
  base_sink_class->stop = GST_DEBUG_FUNCPTR (gst_quicsinkpps_stop);
  base_sink_class->unlock = GST_DEBUG_FUNCPTR (gst_quicsinkpps_unlock);
  base_sink_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_quicsinkpps_unlock_stop);
  // base_sink_class->query = GST_DEBUG_FUNCPTR (gst_quicsinkpps_query);
  // base_sink_class->event = GST_DEBUG_FUNCPTR (gst_quicsinkpps_event);
  // base_sink_class->wait_event = GST_DEBUG_FUNCPTR (gst_quicsinkpps_wait_event);
  // base_sink_class->prepare = GST_DEBUG_FUNCPTR (gst_quicsinkpps_prepare);
  // base_sink_class->prepare_list = GST_DEBUG_FUNCPTR (gst_quicsinkpps_prepare_list);
  base_sink_class->preroll = GST_DEBUG_FUNCPTR (gst_quicsinkpps_preroll);
  base_sink_class->render = GST_DEBUG_FUNCPTR (gst_quicsinkpps_render);
  base_sink_class->render_list = GST_DEBUG_FUNCPTR (gst_quicsinkpps_render_list);

}

static SSL_CTX *static_ssl_ctx;
static gchar *keylog_file;

static gboolean
tick_connection (gpointer context) 
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (context);
  int diff = 0;
  gboolean tickable = FALSE;

  GST_OBJECT_LOCK(quicsinkpps);

  if (quicsinkpps->engine) {
    tickable = lsquic_engine_earliest_adv_tick(quicsinkpps->engine, &diff);
    gst_quic_read_packets(GST_ELEMENT(quicsinkpps), quicsinkpps->socket, quicsinkpps->engine, quicsinkpps->local_address);

    GST_OBJECT_UNLOCK(quicsinkpps);
    return TRUE;
  } else {
    GST_OBJECT_UNLOCK(quicsinkpps);
    return FALSE;
  }

}

static void *
gst_quicsinkpps_open_keylog_file (const SSL *ssl)
{
  const lsquic_conn_t *conn;
  FILE *fh;

  GST_DEBUG_OBJECT(NULL, "Opening keylog file: %s", keylog_file);

  fh = fopen(keylog_file, "ab");
  if (!fh)
      GST_ERROR_OBJECT(NULL,"Could not open SSL key logging file \"%s\" for appending: %s", keylog_file, strerror(errno));
  return fh;
}

static void
gst_quicsinkpps_log_ssl_key (const SSL *ssl, const char *line)
{
  FILE *keylog_file;
  keylog_file = gst_quicsinkpps_open_keylog_file(ssl);
  if (keylog_file)
  {
      fputs(line, keylog_file);
      fputs("\n", keylog_file);
      fclose(keylog_file);
  }
}

/* This creates a new ssl context. TLS_method indicates that the highest 
 * mutually supported version will be negotiated. Since quic requires TLS 1.3
 * at minimum, we later restrict the possible protocol versions to TLS 1.3.
 * If there are any SSL error, a Gstreamer error will be thrown, causing the 
 * pipeline to be reset.
*/
static void
gst_quicsinkpps_load_cert_and_key (GstQuicsinkpps * quicsinkpps)
{
    quicsinkpps->ssl_ctx = SSL_CTX_new(TLS_method());
    if (!quicsinkpps->ssl_ctx)
    {
        GST_ELEMENT_ERROR (quicsinkpps, LIBRARY, FAILED,
          (NULL),
          ("Could not create an SSL context"));
    }
    SSL_CTX_set_min_proto_version(quicsinkpps->ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(quicsinkpps->ssl_ctx, TLS1_3_VERSION);
    if (!SSL_CTX_use_certificate_chain_file(quicsinkpps->ssl_ctx, quicsinkpps->cert_file))
    {
        SSL_CTX_free(quicsinkpps->ssl_ctx);
        GST_ELEMENT_ERROR (quicsinkpps, LIBRARY, FAILED,
          (NULL),
          ("SSL_CTX_use_certificate_chain_file failed, is the path to the cert file correct? path = %s\nIf not provide it via the launch line argument: cert", quicsinkpps->cert_file));
    }
    if (!SSL_CTX_use_PrivateKey_file(quicsinkpps->ssl_ctx, quicsinkpps->key_file,
                                                            SSL_FILETYPE_PEM))
    {
        SSL_CTX_free(quicsinkpps->ssl_ctx);
        GST_ELEMENT_ERROR (quicsinkpps, LIBRARY, FAILED,
          (NULL),
          ("SSL_CTX_use_PrivateKey_file failed, is the path to the key file correct? path = %s\nIf not provide it via the launch line argument: key", quicsinkpps->key_file));
    }
    // Set callback for writing SSL secrets to keylog file
    SSL_CTX_set_keylog_callback(quicsinkpps->ssl_ctx, gst_quicsinkpps_log_ssl_key);
    static_ssl_ctx = quicsinkpps->ssl_ctx;
}

static SSL_CTX *
gst_quicsinkpps_get_ssl_ctx (void *peer_ctx)
{
    return static_ssl_ctx;
}

static lsquic_conn_ctx_t *gst_quicsinkpps_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (stream_if_ctx);
  GST_DEBUG_OBJECT(quicsinkpps,"MW: Connection created");
  quicsinkpps->connection_active = TRUE;
  quicsinkpps->connection = conn;
  return (void *) quicsinkpps;
}

static void gst_quicsinkpps_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS ((void *) lsquic_conn_get_ctx(conn));

  switch (status)
  {
  case LSQ_HSK_OK:
  case LSQ_HSK_RESUMED_OK:
      GST_DEBUG_OBJECT(quicsinkpps, "MW: Handshake completed successfully");
      break;
  default:
      GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("MW: Handshake with client failed"));
      break;
  }
}

static void gst_quicsinkpps_on_conn_closed (struct lsquic_conn *conn)
{
    GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS ((void *) lsquic_conn_get_ctx(conn));

    // Current set-up assumes that there will be a single client and that they,
    // will not reconnect. This may change in the future.
    GST_DEBUG_OBJECT(quicsinkpps, "MW: Connection closed, send EOS to pipeline");
    quicsinkpps->connection_active = FALSE;
}

static lsquic_stream_ctx_t *gst_quicsinkpps_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream)
{
    GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (stream_if_ctx);
    GST_DEBUG_OBJECT(quicsinkpps, "MW: created new stream");
    lsquic_stream_wantwrite(stream, 1);
    return (void *) quicsinkpps;
}

//These functions are not used, but must be defined for lsquic to work
static gsize gst_quicsinkpps_readf (void *ctx, const unsigned char *data, size_t len, int fin)
{
    struct lsquic_stream *stream = (struct lsquic_stream *) ctx;
    GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS ((void *) lsquic_stream_get_ctx(stream));

    return len;
}

static void gst_quicsinkpps_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
    GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (stream_ctx);
    ssize_t bytes_read;

    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, READ, (NULL),
        ("WE SHOULD NOT BE READING"));

    raise(SIGSEGV);

    bytes_read = lsquic_stream_readf(stream, gst_quicsinkpps_readf, stream);
    if (bytes_read < 0)
    {
        GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, READ, (NULL),
        ("Error while reading from a QUIC stream"));
    }
}

static gsize gst_quicsinkpps_read_buffer (void *ctx, void *buf, size_t count)
{
    GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (ctx);
    GstMapInfo map;

    gst_buffer_map (quicsinkpps->stream_ctx.buffer, &map, GST_MAP_READ);

    memcpy(buf, map.data+quicsinkpps->stream_ctx.offset, count);
    quicsinkpps->stream_ctx.offset += count;
    return count;
}


static gsize gst_quicsinkpps_get_remaining_buffer_size (void *ctx)
{
    GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (ctx);
    return quicsinkpps->stream_ctx.buffer_size - quicsinkpps->stream_ctx.offset;
}


/* 
Write GstBuffer to stream
*/
static void gst_quicsinkpps_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
    lsquic_conn_t *conn = lsquic_stream_conn(stream);
    GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (stream_ctx);
    struct lsquic_reader buffer_reader = {gst_quicsinkpps_read_buffer, gst_quicsinkpps_get_remaining_buffer_size, quicsinkpps, };
    const gsize buffer_size = quicsinkpps->stream_ctx.buffer_size;
    gssize bytes_written;

    bytes_written = lsquic_stream_writef(stream, &buffer_reader);
    if (bytes_written > 0)
    {
      GST_DEBUG_OBJECT(quicsinkpps, "MW: wrote %ld bytes to stream", bytes_written);
      if (quicsinkpps->stream_ctx.offset == buffer_size) 
      {
          GST_DEBUG_OBJECT(quicsinkpps, "MW: wrote full buffer to stream (%lu bytes), closing stream", buffer_size);
          lsquic_stream_close(stream);
      }
    }
    else if (bytes_written < 0)
    {
        GST_DEBUG_OBJECT(quicsinkpps, "stream_write() failed to write any bytes. This should not be possible and indicates a serious error. Aborting conn");
        lsquic_conn_abort(conn);
    }
}

static void gst_quicsinkpps_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (stream_ctx);
}

static void
gst_quicsinkpps_init (GstQuicsinkpps * quicsinkpps)
{
  quicsinkpps->port = QUIC_DEFAULT_PORT;
  quicsinkpps->host = g_strdup (QUIC_DEFAULT_HOST);
  quicsinkpps->cert_file = g_strdup (QUIC_DEFAULT_CERTIFICATE_PATH);
  quicsinkpps->key_file = g_strdup (QUIC_DEFAULT_KEY_PATH);
  quicsinkpps->log_file = g_strdup (QUIC_DEFAULT_LOG_PATH);
  keylog_file = g_strdup (QUIC_DEFAULT_KEYLOG_PATH);

  quicsinkpps->socket = -1;
  quicsinkpps->connection_active = FALSE;
  quicsinkpps->engine = NULL;
  quicsinkpps->connection = NULL;
  quicsinkpps->ssl_ctx = NULL;
  
  memset(&quicsinkpps->stream_ctx, 0, sizeof(quicsinkpps->stream_ctx));
}

void
gst_quicsinkpps_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (object);

  GST_DEBUG_OBJECT (quicsinkpps, "set_property");

  switch (property_id) {
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (quicsinkpps->host);
      quicsinkpps->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_PORT:
      quicsinkpps->port = g_value_get_int (value);
      break;
    case PROP_CERT:
      if (!g_value_get_string (value)) {
        g_warning ("SSL certificate path property cannot be NULL");
        break;
      }
      g_free (quicsinkpps->cert_file);
      quicsinkpps->cert_file = g_strdup (g_value_get_string (value));
      break;
    case PROP_LOG:
      if (!g_value_get_string (value)) {
        g_warning ("Log path property cannot be NULL");
        break;
      }
      g_free (quicsinkpps->log_file);
      quicsinkpps->log_file = g_strdup (g_value_get_string (value));
      break;
    case PROP_KEY:
      if (!g_value_get_string (value)) {
        g_warning ("SSL key path property cannot be NULL");
        break;
      }
      g_free (quicsinkpps->key_file);
      quicsinkpps->key_file = g_strdup (g_value_get_string (value));
    case PROP_KEYLOG:
      if (keylog_file[0] != '\0') {
        break;
      }
      if (!g_value_get_string (value)) {
        g_warning ("SSL keylog path property cannot be NULL");
        break;
      }
      g_free (keylog_file);
      keylog_file = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_quicsinkpps_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (object);
  GST_DEBUG_OBJECT (quicsinkpps, "get_property");

  switch (property_id) {
    case PROP_HOST:
      g_value_set_string (value, quicsinkpps->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, quicsinkpps->port);
      break;
    case PROP_CERT:
      g_value_set_string (value, quicsinkpps->cert_file);
      break;
    case PROP_LOG:
      g_value_set_string (value, quicsinkpps->log_file);
      break;
    case PROP_KEY:
      g_value_set_string (value, quicsinkpps->key_file);
      break;
    case PROP_KEYLOG:
      g_value_set_string (value, keylog_file);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_quicsinkpps_dispose (GObject * object)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (object);

  GST_DEBUG_OBJECT (quicsinkpps, "dispose");

  GST_OBJECT_LOCK (quicsinkpps);
  if (quicsinkpps->connection) {
    GST_DEBUG_OBJECT (quicsinkpps, "dispose called, closing connection");
    lsquic_conn_close(quicsinkpps->connection);
    while (quicsinkpps->connection_active) {
      gst_quic_read_packets(GST_ELEMENT(quicsinkpps), quicsinkpps->socket, quicsinkpps->engine, quicsinkpps->local_address);
    }
    quicsinkpps->connection = NULL;
  } else {
    GST_DEBUG_OBJECT (quicsinkpps, "dispose called, but connection has already been closed");
  }
  GST_OBJECT_UNLOCK (quicsinkpps);

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_quicsinkpps_parent_class)->dispose (object);
}

void
gst_quicsinkpps_finalize (GObject * object)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (object);

  GST_DEBUG_OBJECT (quicsinkpps, "finalize");

  GST_OBJECT_LOCK (quicsinkpps);
  /* clean up lsquic */
  if (quicsinkpps->engine) {
    GST_DEBUG_OBJECT(quicsinkpps, "Destroying lsquic engine");
    gst_quic_read_packets(GST_ELEMENT(quicsinkpps), quicsinkpps->socket, quicsinkpps->engine, quicsinkpps->local_address);
    lsquic_engine_destroy(quicsinkpps->engine);
    quicsinkpps->engine = NULL;
  }
  lsquic_global_cleanup();
  GST_OBJECT_UNLOCK (quicsinkpps);

  G_OBJECT_CLASS (gst_quicsinkpps_parent_class)->finalize (object);
}

/* Provides the basesink with possible pad capabilities
   to allow it to respond to queries from upstream elements.
   Since the data transmitted over QUIC could be of any type, we 
   return either ANY caps or, if provided, the caps of the
   upstream element */
static GstCaps *
gst_quicsinkpps_get_caps (GstBaseSink * sink, GstCaps * filter)
{
  GstQuicsinkpps *quicsinkpps;
  GstCaps *caps = NULL;

  quicsinkpps = GST_QUICSINKPPS (sink);

  caps = (filter ? gst_caps_ref (filter) : gst_caps_new_any ());

  GST_DEBUG_OBJECT (quicsinkpps, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (caps));
  return caps;
}

/* Set up lsquic and establish a connection to the client */
static gboolean
gst_quicsinkpps_start (GstBaseSink * sink)
{
  socklen_t socklen;
  struct lsquic_engine_api engine_api;
  struct lsquic_engine_settings engine_settings;
  server_addr_u server_addr;
  int activate, socket_opt_result;

  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (sink);

  GST_DEBUG_OBJECT (quicsinkpps, "start");

  if (0 != lsquic_global_init(LSQUIC_GLOBAL_SERVER))
  {
    GST_ELEMENT_ERROR (quicsinkpps, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  GST_DEBUG_OBJECT(quicsinkpps, "Host is: %s, port is: %d, log is: %s", quicsinkpps->host, quicsinkpps->port, quicsinkpps->log_file);

  /* Initialize logging */
  if (quicsinkpps->log_file[0] != '\0') {
    FILE *s_log_fh = fopen(quicsinkpps->log_file, "wb");

    if (s_log_fh != NULL)
    {    
      if (0 != lsquic_set_log_level("debug"))
      {
        GST_ELEMENT_ERROR (quicsinkpps, LIBRARY, INIT,
            (NULL),
            ("Failed to initialise lsquic logging"));
        return FALSE;
      }

      setvbuf(s_log_fh, NULL, _IOLBF, 0);
      lsquic_logger_init(&logger_if, s_log_fh, LLTS_HHMMSSUS);
    } else {
      GST_WARNING_OBJECT (quicsinkpps, "Could not open logfile, errno: %d", errno);
    }
  } else {
    GST_WARNING_OBJECT (quicsinkpps, "No logfile provided, lsquic logs will not be created");
  }

  // Initialize engine settings to default values
  lsquic_engine_init_settings(&engine_settings, QUIC_SERVER);

  // Disable delayed acks to improve response to loss
  engine_settings.es_delayed_acks = 0;

  // The max stream flow control window seems to default to 16384.
  // This ends up causing streams to become blocked frequently, leading
  // to delays. After experimentation, a value of 524288 was found to be 
  // large enough that blocks do not occur.
  engine_settings.es_max_sfcw = 524288;

  engine_settings.es_base_plpmtu = 1472;

	engine_settings.es_pace_packets = FALSE;

  // Using the default values (es_max_streams_in = 50, es_init_max_streams_bidi=100), the max number of streams grows at too little a rate
  // when we are creating a new packet per stream. This results in significant delays
  // By setting es_max_streams_in and es_init_max_streams_bidi to a higher value, we can avoid this.
  // Setting es_max_streams_in above 200 seems to cause an error, but setting a higher starting value
  // using es_init_max_streams_bidi allows lsquic to run without delays due to stream count.
  engine_settings.es_max_streams_in = 200;
  engine_settings.es_init_max_streams_bidi = 20000;

  // We don't want to close the connection until all data has been acknowledged.
  // So we set es_delay_onclose to true
  engine_settings.es_delay_onclose = TRUE;

  // Parse IP address and set port number
  if (!gst_quic_set_addr(quicsinkpps->host, quicsinkpps->port, &server_addr))
  {
      GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to resolve host: %s", quicsinkpps->host));
      return FALSE;
  }

  // Set up SSL context
  if (quicsinkpps->cert_file [0] == '\0') {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, FAILED,
            (NULL),
            ("You have not provided an SSL certificate. Please provide one with the `cert` cmdline option"));
    return FALSE;
  }

  if (quicsinkpps->key_file [0] == '\0') {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, FAILED,
            (NULL),
            ("You have not provided an SSL key. Please provide one with the `key` cmdline option"));
    return FALSE;
  }
  gst_quicsinkpps_load_cert_and_key(quicsinkpps);

  // Create socket
  quicsinkpps->socket = socket(server_addr.sa.sa_family, SOCK_DGRAM, 0);

  GST_DEBUG_OBJECT(quicsinkpps, "Socket fd = %d", quicsinkpps->socket);

  if (quicsinkpps->socket < 0)
  {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
      ("Failed to open socket"));
    return FALSE;
  }

  // set socket to be non-blocking
  int socket_flags = fcntl(quicsinkpps->socket, F_GETFL);
  if (socket_flags == -1) 
  {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to retrieve socket_flags using fcntl"));
    return FALSE;
  }

  socket_flags |= O_NONBLOCK;
  if (0 != fcntl(quicsinkpps->socket, F_SETFL, socket_flags)) 
  {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to set socket_flags using fcntl"));
    return FALSE;
  }

  //set ip flags
  //Set flags to allow the original destination address to be retrieved.
  activate = 1;
  if (AF_INET == server_addr.sa.sa_family)
  {
      socket_opt_result = setsockopt(quicsinkpps->socket, IPPROTO_IP, IP_RECVDESTADDR_FLAG, &activate, sizeof(activate));
  }
  else 
  {
      socket_opt_result = setsockopt(quicsinkpps->socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &activate, sizeof(activate));
  }

  if (0 != socket_opt_result) 
  {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to set option for original destination address support using setsockopt"));
    return FALSE;
  }

  // Activate IP_TOS field manipulation to allow ecn to be set
  if (AF_INET == server_addr.sa.sa_family)
  {
      socket_opt_result = setsockopt(quicsinkpps->socket, IPPROTO_IP, IP_RECVTOS, &activate, sizeof(activate));
  }
  else
  {
      socket_opt_result = setsockopt(quicsinkpps->socket, IPPROTO_IPV6, IPV6_RECVTCLASS, &activate, sizeof(activate));
  }

  if (0 != socket_opt_result) 
  {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add ecn support using setsockopt"));
    return FALSE;
  }

  // Bind socket to address of our server and save local address in a sockaddr_storage struct.
  // sockaddr_storage is used as it is big enough to allow IPV6 addresses to be stored.
  socklen = sizeof(server_addr);
  if (0 != bind(quicsinkpps->socket, &server_addr.sa, socklen))
  {
      GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to bind socket %d", errno));
      return FALSE;
  }
  memcpy(&(quicsinkpps->local_address), &server_addr, sizeof(server_addr));

  // Initialize engine callbacks
  memset(&engine_api, 0, sizeof(engine_api));
  engine_api.ea_packets_out     = gst_quic_send_packets;
  engine_api.ea_packets_out_ctx = quicsinkpps;
  engine_api.ea_stream_if       = &quicsinkpps_callbacks;
  engine_api.ea_stream_if_ctx   = quicsinkpps;
  engine_api.ea_settings        = &engine_settings;
  engine_api.ea_get_ssl_ctx     = gst_quicsinkpps_get_ssl_ctx;

  // Instantiate engine in server mode
  quicsinkpps->engine = lsquic_engine_new(QUIC_SERVER, &engine_api);
  if (!quicsinkpps->engine)
  {
      GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to create lsquic engine"));
      return FALSE;
  }

  GST_DEBUG_OBJECT(quicsinkpps, "Initialised engine, ready to accept connections");

  while (!quicsinkpps->connection_active) {
    gst_quic_read_packets(GST_ELEMENT(quicsinkpps), quicsinkpps->socket, quicsinkpps->engine, quicsinkpps->local_address);
  }

  g_timeout_add(1, tick_connection, quicsinkpps);

  return TRUE;
}

static gboolean
gst_quicsinkpps_stop (GstBaseSink * sink)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (sink);

  GST_OBJECT_LOCK (quicsinkpps);
  GST_DEBUG_OBJECT (quicsinkpps, "stop called, closing connection");
  if (quicsinkpps->connection) {
    lsquic_conn_close(quicsinkpps->connection);
    while (quicsinkpps->connection_active) {
      gst_quic_read_packets(GST_ELEMENT(quicsinkpps), quicsinkpps->socket, quicsinkpps->engine, quicsinkpps->local_address);
    }
    quicsinkpps->connection = NULL;
  }
  GST_OBJECT_UNLOCK (quicsinkpps);

  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_quicsinkpps_unlock (GstBaseSink * sink)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (sink);

  GST_DEBUG_OBJECT (quicsinkpps, "unlock");

  return TRUE;
}

/* Clear a previously indicated unlock request not that unlocking is
 * complete. Sub-classes should clear any command queue or indicator they
 * set during unlock */
static gboolean
gst_quicsinkpps_unlock_stop (GstBaseSink * sink)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (sink);

  GST_DEBUG_OBJECT (quicsinkpps, "unlock_stop");

  return TRUE;
}

/* notify subclass of preroll buffer or real buffer */
static GstFlowReturn
gst_quicsinkpps_preroll (GstBaseSink * sink, GstBuffer * buffer)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (sink);

  GST_DEBUG_OBJECT (quicsinkpps, "preroll");

  return GST_FLOW_OK;
}

/* Write data from upstream buffers to a stream.
   every buffer will contain 1 complete RTP packet,
   so a new stream is created per buffer */
static GstFlowReturn
gst_quicsinkpps_render (GstBaseSink * sink, GstBuffer * buffer)
{
  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (sink);

  if (!quicsinkpps->connection_active) 
  {
    GST_ELEMENT_ERROR (quicsinkpps, RESOURCE, READ, (NULL),
        ("There is no active connection to send data to"));
    return GST_FLOW_ERROR;
  } 

  GST_DEBUG_OBJECT (quicsinkpps, "render -- duration: %" GST_TIME_FORMAT ",  dts: %" GST_TIME_FORMAT ",  pts: %" GST_TIME_FORMAT", GREP_MARKER pts: %lu", GST_TIME_ARGS(buffer->duration), GST_TIME_ARGS(buffer->dts), GST_TIME_ARGS(buffer->pts), buffer->pts);
  if (!(gst_buffer_get_flags(buffer) & GST_BUFFER_FLAG_DELTA_UNIT)) {
    GST_DEBUG_OBJECT (quicsinkpps, "GST_BUFFER_FLAG_DELTA_UNIT is not set, this frame is an I-frame");
  } 

  quicsinkpps->stream_ctx.buffer = buffer;
  quicsinkpps->stream_ctx.offset = 0;
  quicsinkpps->stream_ctx.buffer_size = gst_buffer_get_size(quicsinkpps->stream_ctx.buffer);

  // Simultaneous attempts to process the lsquic connection via read packets will cause 
  // An assertion error within lsquic. So we take the lock to prevent this from happening
  GST_OBJECT_LOCK(quicsinkpps);
  lsquic_conn_make_stream(quicsinkpps->connection);

  while (quicsinkpps->stream_ctx.offset != quicsinkpps->stream_ctx.buffer_size) 
  {
    gst_quic_read_packets(GST_ELEMENT(quicsinkpps), quicsinkpps->socket, quicsinkpps->engine, quicsinkpps->local_address);
  }

  // If we do not tell lsquic to explicitly send packets after writing, 
  // it may delay until the packet can be filled with more data, this is not desired.
  lsquic_engine_send_unsent_packets (quicsinkpps->engine);
  GST_OBJECT_UNLOCK(quicsinkpps);

  return GST_FLOW_OK;
}

/* Render a BufferList */
static GstFlowReturn
gst_quicsinkpps_render_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  GstFlowReturn flow_return;
  guint i, num_buffers;

  GstQuicsinkpps *quicsinkpps = GST_QUICSINKPPS (sink);

  GST_DEBUG_OBJECT (quicsinkpps, "render_list");

  num_buffers = gst_buffer_list_length (buffer_list);
  if (num_buffers == 0) {
    GST_DEBUG_OBJECT (quicsinkpps, "Empty buffer list provided, returning immediately");
    return GST_FLOW_OK;
  }
    
  for (i = 0; i < num_buffers; i++) {

    flow_return = gst_quicsinkpps_render (sink, gst_buffer_list_get (buffer_list, i));

    if (flow_return != GST_FLOW_OK) {
      GST_WARNING_OBJECT(quicsinkpps, "flow_return was not GST_FLOW_OK, returning after sending %u/%u buffers", i+1, num_buffers);
      return flow_return;
    }

  }

  return GST_FLOW_OK;
}
