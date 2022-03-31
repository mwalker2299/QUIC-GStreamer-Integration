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
 * SECTION:element-gstquicsinkfps
 *
 * The quicsinkfps element acts as a QUIC server. This element is solely for 
 * experimental purposes. 
 * The element utilises one stream per frame sent.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v filesrc location=\"test.mp4\" ! qtdemux name=demux  demux.video_0  ! rtph264pay seqnum-offset=0 mtu=1398 ! rtpstreampay ! quicsinkfps host=127.0.0.1 port=5000 keylog=SSL.keys sync=false"
 * ]|
 * Send packets over the network via QUIC.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "gstquicsinkfps.h"
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

GST_DEBUG_CATEGORY_STATIC (gst_quicsinkfps_debug_category);
#define GST_CAT_DEFAULT gst_quicsinkfps_debug_category

/* gstreamer method prototypes */

static void gst_quicsinkfps_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_quicsinkfps_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_quicsinkfps_dispose (GObject * object);
static void gst_quicsinkfps_finalize (GObject * object);

static GstCaps *gst_quicsinkfps_get_caps (GstBaseSink * sink, GstCaps * filter);
static gboolean gst_quicsinkfps_start (GstBaseSink * sink);
static gboolean gst_quicsinkfps_stop (GstBaseSink * sink);
static gboolean gst_quicsinkfps_unlock (GstBaseSink * sink);
static gboolean gst_quicsinkfps_unlock_stop (GstBaseSink * sink);
static gboolean gst_quicsinkfps_event (GstBaseSink * sink, GstEvent * event);
static GstFlowReturn gst_quicsinkfps_preroll (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_quicsinkfps_render (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_quicsinkfps_render_list (GstBaseSink * sink,
    GstBufferList * buffer_list);


/* Quic functions */
static void gst_quicsinkfps_load_cert_and_key (GstQuicsinkfps * quicsinkfps);
static SSL_CTX *gst_quicsinkfps_get_ssl_ctx (void *peer_ctx);
static lsquic_conn_ctx_t *gst_quicsinkfps_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn);
static void gst_quicsinkfps_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status);
static void gst_quicsinkfps_on_conn_closed (struct lsquic_conn *conn);
static lsquic_stream_ctx_t *gst_quicsinkfps_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream);
static size_t gst_quicsinkfps_readf (void *ctx, const unsigned char *data, size_t len, int fin);
static void gst_quicsinkfps_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsinkfps_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsinkfps_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);

/* QUIC callbacks passed to quic engine */
static struct lsquic_stream_if quicsinkfps_callbacks =
{
    .on_new_conn        = gst_quicsinkfps_on_new_conn,
    .on_hsk_done        = gst_quicsinkfps_on_hsk_done,
    .on_conn_closed     = gst_quicsinkfps_on_conn_closed,
    .on_new_stream      = gst_quicsinkfps_on_new_stream,
    .on_read            = gst_quicsinkfps_on_read,
    .on_write           = gst_quicsinkfps_on_write,
    .on_close           = gst_quicsinkfps_on_close,
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

static GstStaticPadTemplate gst_quicsinkfps_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstQuicsinkfps, gst_quicsinkfps, GST_TYPE_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT (gst_quicsinkfps_debug_category, "quicsinkfps", 0,
        "debug category for quicsinkfps element"));

static void
gst_quicsinkfps_class_init (GstQuicsinkfpsClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_quicsinkfps_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "QUIC packet sender", "Sink/Network", "Send packets over the network via QUIC (frame per stream)",
      "Matthew Walker <mjwalker2299@gmail.com>");

  gobject_class->set_property = gst_quicsinkfps_set_property;
  gobject_class->get_property = gst_quicsinkfps_get_property;
  gobject_class->dispose = gst_quicsinkfps_dispose;
  gobject_class->finalize = gst_quicsinkfps_finalize;

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

  base_sink_class->get_caps = GST_DEBUG_FUNCPTR (gst_quicsinkfps_get_caps);
  base_sink_class->start = GST_DEBUG_FUNCPTR (gst_quicsinkfps_start);
  base_sink_class->stop = GST_DEBUG_FUNCPTR (gst_quicsinkfps_stop);
  base_sink_class->unlock = GST_DEBUG_FUNCPTR (gst_quicsinkfps_unlock);
  base_sink_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_quicsinkfps_unlock_stop);
  base_sink_class->event = GST_DEBUG_FUNCPTR (gst_quicsinkfps_event);
  base_sink_class->preroll = GST_DEBUG_FUNCPTR (gst_quicsinkfps_preroll);
  base_sink_class->render = GST_DEBUG_FUNCPTR (gst_quicsinkfps_render);
  base_sink_class->render_list = GST_DEBUG_FUNCPTR (gst_quicsinkfps_render_list);

}

static SSL_CTX *static_ssl_ctx;
static gchar *keylog_file;

static gboolean
tick_connection (gpointer context) 
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (context);
  int diff = 0;
  gboolean tickable = FALSE;

  GST_OBJECT_LOCK(quicsinkfps);

  if (quicsinkfps->engine) {
    tickable = lsquic_engine_earliest_adv_tick(quicsinkfps->engine, &diff);
    gst_quic_read_packets(GST_ELEMENT(quicsinkfps), quicsinkfps->socket, quicsinkfps->engine, quicsinkfps->local_address);

    GST_OBJECT_UNLOCK(quicsinkfps);
    return TRUE;
  } else {
    GST_OBJECT_UNLOCK(quicsinkfps);
    return FALSE;
  }

}

static void *
gst_quicsinkfps_open_keylog_file (const SSL *ssl)
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
gst_quicsinkfps_log_ssl_key (const SSL *ssl, const char *line)
{
  FILE *keylog_file;
  keylog_file = gst_quicsinkfps_open_keylog_file(ssl);
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
gst_quicsinkfps_load_cert_and_key (GstQuicsinkfps * quicsinkfps)
{
    quicsinkfps->ssl_ctx = SSL_CTX_new(TLS_method());
    if (!quicsinkfps->ssl_ctx)
    {
        GST_ELEMENT_ERROR (quicsinkfps, LIBRARY, FAILED,
          (NULL),
          ("Could not create an SSL context"));
    }
    SSL_CTX_set_min_proto_version(quicsinkfps->ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(quicsinkfps->ssl_ctx, TLS1_3_VERSION);
    if (!SSL_CTX_use_certificate_chain_file(quicsinkfps->ssl_ctx, quicsinkfps->cert_file))
    {
        SSL_CTX_free(quicsinkfps->ssl_ctx);
        GST_ELEMENT_ERROR (quicsinkfps, LIBRARY, FAILED,
          (NULL),
          ("SSL_CTX_use_certificate_chain_file failed, is the path to the cert file correct? path = %sIf not provide it via the launch line argument: cert", quicsinkfps->cert_file));
    }
    if (!SSL_CTX_use_PrivateKey_file(quicsinkfps->ssl_ctx, quicsinkfps->key_file,
                                                            SSL_FILETYPE_PEM))
    {
        SSL_CTX_free(quicsinkfps->ssl_ctx);
        GST_ELEMENT_ERROR (quicsinkfps, LIBRARY, FAILED,
          (NULL),
          ("SSL_CTX_use_PrivateKey_file failed, is the path to the key file correct?\nIf not provide it via the launch line argument: key path = %s", quicsinkfps->key_file));
    }
    // Set callback for writing SSL secrets to keylog file
    SSL_CTX_set_keylog_callback(quicsinkfps->ssl_ctx, gst_quicsinkfps_log_ssl_key);
    static_ssl_ctx = quicsinkfps->ssl_ctx;
}

static SSL_CTX *
gst_quicsinkfps_get_ssl_ctx (void *peer_ctx)
{
    return static_ssl_ctx;
}

static lsquic_conn_ctx_t *gst_quicsinkfps_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (stream_if_ctx);
  GST_DEBUG_OBJECT(quicsinkfps,"MW: Connection created");
  quicsinkfps->connection_active = TRUE;
  quicsinkfps->connection = conn;
  return (void *) quicsinkfps;
}

static void gst_quicsinkfps_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS ((void *) lsquic_conn_get_ctx(conn));

  switch (status)
  {
  case LSQ_HSK_OK:
  case LSQ_HSK_RESUMED_OK:
      GST_DEBUG_OBJECT(quicsinkfps, "MW: Handshake completed successfully");
      break;
  default:
      GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("MW: Handshake with client failed"));
      break;
  }
}

static void gst_quicsinkfps_on_conn_closed (struct lsquic_conn *conn)
{
    GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS ((void *) lsquic_conn_get_ctx(conn));

    // Current set-up assumes that there will be a single client and that they,
    // will not reconnect. This may change in the future.
    GST_DEBUG_OBJECT(quicsinkfps, "MW: Connection closed, send EOS to pipeline");
    quicsinkfps->connection_active = FALSE;
}

static lsquic_stream_ctx_t *gst_quicsinkfps_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream)
{
    GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (stream_if_ctx);
    GST_DEBUG_OBJECT(quicsinkfps, "MW: created new stream");
    quicsinkfps->stream = stream;
    return (void *) quicsinkfps;
}

//These functions are unused, but need to be defined for lsquic to function
static gsize gst_quicsinkfps_readf (void *ctx, const unsigned char *data, size_t len, int fin)
{
    struct lsquic_stream *stream = (struct lsquic_stream *) ctx;
    GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS ((void *) lsquic_stream_get_ctx(stream));

    return len;
}

static void gst_quicsinkfps_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
    GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (stream_ctx);
    ssize_t bytes_read;

    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, READ, (NULL),
        ("WE SHOULD NOT BE READING"));

    raise(SIGSEGV);

    bytes_read = lsquic_stream_readf(stream, gst_quicsinkfps_readf, stream);
    if (bytes_read < 0)
    {
        GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, READ, (NULL),
        ("Error while reading from a QUIC stream"));
    }
}

static gsize gst_quicsinkfps_read_buffer (void *ctx, void *buf, size_t count)
{
    GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (ctx);
    GstMapInfo map;

    gst_buffer_map (quicsinkfps->stream_ctx.buffer, &map, GST_MAP_READ);

    memcpy(buf, map.data+quicsinkfps->stream_ctx.offset, count);
    quicsinkfps->stream_ctx.offset += count;
    return count;
}


static gsize gst_quicsinkfps_get_remaining_buffer_size (void *ctx)
{
    GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (ctx);
    return quicsinkfps->stream_ctx.buffer_size - quicsinkfps->stream_ctx.offset;
}


/* 
Write GstBuffer to stream
*/
static void gst_quicsinkfps_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
    lsquic_conn_t *conn = lsquic_stream_conn(stream);
    GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (stream_ctx);
    struct lsquic_reader buffer_reader = {gst_quicsinkfps_read_buffer, gst_quicsinkfps_get_remaining_buffer_size, quicsinkfps, };
    const gsize buffer_size = quicsinkfps->stream_ctx.buffer_size;
    gssize bytes_written;

    // GST_DEBUG_OBJECT(quicsinkfps, "MW: writing to stream, total buffer size = (%lu bytes)", buffer_size);

    bytes_written = lsquic_stream_writef(stream, &buffer_reader);
    if (bytes_written > 0)
    {
      GST_DEBUG_OBJECT(quicsinkfps, "MW: wrote %ld bytes to stream", bytes_written);
      if (quicsinkfps->stream_ctx.offset == buffer_size)
      {
          GST_DEBUG_OBJECT(quicsinkfps, "MW: wrote full buffer to stream (%lu bytes)", buffer_size);
          lsquic_stream_wantwrite(quicsinkfps->stream, 0);
      }
    }
    else if (bytes_written < 0)
    {
        GST_DEBUG_OBJECT(quicsinkfps, "stream_write() failed to write any bytes. This should not be possible and indicates a serious error. Aborting conn");
        lsquic_conn_abort(conn);
    }
}

static void gst_quicsinkfps_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (stream_ctx);
}

static void
gst_quicsinkfps_init (GstQuicsinkfps * quicsinkfps)
{
  quicsinkfps->port = QUIC_DEFAULT_PORT;
  quicsinkfps->host = g_strdup (QUIC_DEFAULT_HOST);
  quicsinkfps->cert_file = g_strdup (QUIC_DEFAULT_CERTIFICATE_PATH);
  quicsinkfps->key_file = g_strdup (QUIC_DEFAULT_KEY_PATH);
  quicsinkfps->log_file = g_strdup (QUIC_DEFAULT_LOG_PATH);
  keylog_file = g_strdup (QUIC_DEFAULT_KEYLOG_PATH);

  quicsinkfps->socket = -1;
  quicsinkfps->connection_active = FALSE;
  quicsinkfps->engine = NULL;
  quicsinkfps->connection = NULL;
  quicsinkfps->stream = NULL;
  quicsinkfps->ssl_ctx = NULL;
  quicsinkfps->prev_buffer_pts = -1;
  
  memset(&quicsinkfps->stream_ctx, 0, sizeof(quicsinkfps->stream_ctx));
}

void
gst_quicsinkfps_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (object);

  GST_DEBUG_OBJECT (quicsinkfps, "set_property");

  switch (property_id) {
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (quicsinkfps->host);
      quicsinkfps->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_PORT:
      quicsinkfps->port = g_value_get_int (value);
      break;
    case PROP_CERT:
      if (!g_value_get_string (value)) {
        g_warning ("SSL certificate path property cannot be NULL");
        break;
      }
      g_free (quicsinkfps->cert_file);
      quicsinkfps->cert_file = g_strdup (g_value_get_string (value));
      break;
    case PROP_LOG:
      if (!g_value_get_string (value)) {
        g_warning ("Log path property cannot be NULL");
        break;
      }
      g_free (quicsinkfps->log_file);
      quicsinkfps->log_file = g_strdup (g_value_get_string (value));
      break;
    case PROP_KEY:
      if (!g_value_get_string (value)) {
        g_warning ("SSL key path property cannot be NULL");
        break;
      }
      g_free (quicsinkfps->key_file);
      quicsinkfps->key_file = g_strdup (g_value_get_string (value));
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
gst_quicsinkfps_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (object);
  GST_DEBUG_OBJECT (quicsinkfps, "get_property");

  switch (property_id) {
    case PROP_HOST:
      g_value_set_string (value, quicsinkfps->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, quicsinkfps->port);
      break;
    case PROP_CERT:
      g_value_set_string (value, quicsinkfps->cert_file);
      break;
    case PROP_LOG:
      g_value_set_string (value, quicsinkfps->log_file);
      break;
    case PROP_KEY:
      g_value_set_string (value, quicsinkfps->key_file);
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
gst_quicsinkfps_dispose (GObject * object)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (object);

  GST_DEBUG_OBJECT (quicsinkfps, "dispose");

  GST_OBJECT_LOCK (quicsinkfps);
  if (quicsinkfps->stream) {
    GST_DEBUG_OBJECT (quicsinkfps, "dispose called, closing stream");
    lsquic_stream_close(quicsinkfps->stream);
    quicsinkfps->stream = NULL;
  } else {
    GST_DEBUG_OBJECT (quicsinkfps, "dispose called, but stream has already been closed");
  }

  if (quicsinkfps->connection) {
    GST_DEBUG_OBJECT (quicsinkfps, "dispose called, closing connection");
    lsquic_conn_close(quicsinkfps->connection);
    while (quicsinkfps->connection_active) {
      gst_quic_read_packets(GST_ELEMENT(quicsinkfps), quicsinkfps->socket, quicsinkfps->engine, quicsinkfps->local_address);
    }
    quicsinkfps->connection = NULL;
  } else {
    GST_DEBUG_OBJECT (quicsinkfps, "dispose called, but connection has already been closed");
  }
  GST_OBJECT_UNLOCK (quicsinkfps);

  G_OBJECT_CLASS (gst_quicsinkfps_parent_class)->dispose (object);
}

void
gst_quicsinkfps_finalize (GObject * object)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (object);

  GST_DEBUG_OBJECT (quicsinkfps, "finalize");

  GST_OBJECT_LOCK (quicsinkfps);
  /* clean up lsquic */
  if (quicsinkfps->engine) {
    GST_DEBUG_OBJECT(quicsinkfps, "Destroying lsquic engine");
    gst_quic_read_packets(GST_ELEMENT(quicsinkfps), quicsinkfps->socket, quicsinkfps->engine, quicsinkfps->local_address);
    lsquic_engine_destroy(quicsinkfps->engine);
    quicsinkfps->engine = NULL;
  }
  lsquic_global_cleanup();
  GST_OBJECT_UNLOCK (quicsinkfps);

  G_OBJECT_CLASS (gst_quicsinkfps_parent_class)->finalize (object);
}

/* Provides the basesink with possible pad capabilities
   to allow it to respond to queries from upstream elements.
   Since the data transmitted over QUIC could be of any type, we 
   return either ANY caps or, if provided, the caps of the
   upstream element */
static GstCaps *
gst_quicsinkfps_get_caps (GstBaseSink * sink, GstCaps * filter)
{
  GstQuicsinkfps *quicsinkfps;
  GstCaps *caps = NULL;

  quicsinkfps = GST_QUICSINKFPS (sink);

  caps = (filter ? gst_caps_ref (filter) : gst_caps_new_any ());

  GST_DEBUG_OBJECT (quicsinkfps, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (caps));
  return caps;
}


/* Set up lsquic and establish a connection to the client */
static gboolean
gst_quicsinkfps_start (GstBaseSink * sink)
{
  socklen_t socklen;
  struct lsquic_engine_api engine_api;
  struct lsquic_engine_settings engine_settings;
  server_addr_u server_addr;
  int activate, socket_opt_result;

  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);

  GST_DEBUG_OBJECT (quicsinkfps, "start");

  if (0 != lsquic_global_init(LSQUIC_GLOBAL_SERVER))
  {
    GST_ELEMENT_ERROR (quicsinkfps, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  GST_DEBUG_OBJECT(quicsinkfps, "Host is: %s, port is: %d, log is: %s", quicsinkfps->host, quicsinkfps->port, quicsinkfps->log_file);

  /* Initialize logging */
  if (quicsinkfps->log_file[0] != '\0') {
    FILE *s_log_fh = fopen(quicsinkfps->log_file, "wb");

    if (s_log_fh != NULL)
    {    
      if (0 != lsquic_set_log_level("debug"))
      {
        GST_ELEMENT_ERROR (quicsinkfps, LIBRARY, INIT,
            (NULL),
            ("Failed to initialise lsquic logging"));
        return FALSE;
      }

      setvbuf(s_log_fh, NULL, _IOLBF, 0);
      lsquic_logger_init(&logger_if, s_log_fh, LLTS_HHMMSSUS);
    } else {
      GST_WARNING_OBJECT (quicsinkfps, "Could not open logfile, errno: %d", errno);
    }
  } else {
    GST_WARNING_OBJECT (quicsinkfps, "No logfile provided, lsquic logs will not be created");
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

  // The initial stream flow control offset on the client side is 16384.
  // However, the server appears to begin with a much higher max send offset
  // It should be zero, but instead it's 6291456. We can force lsquic to behave
  // by setting the following to parameters. Initially, I experiemented with
  // setting these values to 16384, but, as lsquic waits until we have read up
  // to half the stream flow control offset, this causes the window to grow too slowly.
  // UPDATE: After changing to the BBB video, the previously discovered values were no longer 
  // suitable, As time was short and I did not want to risk tests being impacted by stream flow 
  // control, I have instead opted to raise the initial max stream data to 85000, which is the maximum 
  // stream flow allowance used by a stream during FPS runs.
  engine_settings.es_init_max_stream_data_bidi_local = 85000;
  engine_settings.es_init_max_stream_data_bidi_remote = 85000;

  // We don't want to close the connection until all data has been acknowledged.
  // So we set es_delay_onclose to true
  engine_settings.es_delay_onclose = TRUE;

  // Parse IP address and set port number
  if (!gst_quic_set_addr(quicsinkfps->host, quicsinkfps->port, &server_addr))
  {
      GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to resolve host: %s", quicsinkfps->host));
      return FALSE;
  }

  // Set up SSL context
  if (quicsinkfps->cert_file [0] == '\0') {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, FAILED,
            (NULL),
            ("You have not provided an SSL certificate. Please provide one with the `cert` cmdline option"));
    return FALSE;
  }

  if (quicsinkfps->key_file [0] == '\0') {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, FAILED,
            (NULL),
            ("You have not provided an SSL key. Please provide one with the `key` cmdline option"));
    return FALSE;
  }
  gst_quicsinkfps_load_cert_and_key(quicsinkfps);

  // Create socket
  quicsinkfps->socket = socket(server_addr.sa.sa_family, SOCK_DGRAM, 0);

  GST_DEBUG_OBJECT(quicsinkfps, "Socket fd = %d", quicsinkfps->socket);

  if (quicsinkfps->socket < 0)
  {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
      ("Failed to open socket"));
    return FALSE;
  }

  // set socket to be non-blocking
  int socket_flags = fcntl(quicsinkfps->socket, F_GETFL);
  if (socket_flags == -1) 
  {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to retrieve socket_flags using fcntl"));
    return FALSE;
  }

  socket_flags |= O_NONBLOCK;
  if (0 != fcntl(quicsinkfps->socket, F_SETFL, socket_flags)) 
  {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to set socket_flags using fcntl"));
    return FALSE;
  }

  //set ip flags
  //Set flags to allow the original destination address to be retrieved.
  activate = 1;
  if (AF_INET == server_addr.sa.sa_family)
  {
      socket_opt_result = setsockopt(quicsinkfps->socket, IPPROTO_IP, IP_RECVDESTADDR_FLAG, &activate, sizeof(activate));
  }
  else 
  {
      socket_opt_result = setsockopt(quicsinkfps->socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &activate, sizeof(activate));
  }

  if (0 != socket_opt_result) 
  {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to set option for original destination address support using setsockopt"));
    return FALSE;
  }

  // Activate IP_TOS field manipulation to allow ecn to be set
  if (AF_INET == server_addr.sa.sa_family)
  {
      socket_opt_result = setsockopt(quicsinkfps->socket, IPPROTO_IP, IP_RECVTOS, &activate, sizeof(activate));
  }
  else
  {
      socket_opt_result = setsockopt(quicsinkfps->socket, IPPROTO_IPV6, IPV6_RECVTCLASS, &activate, sizeof(activate));
  }

  if (0 != socket_opt_result) 
  {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add ecn support using setsockopt"));
    return FALSE;
  }

  // Bind socket to address of our server and save local address in a sockaddr_storage struct.
  // sockaddr_storage is used as it is big enough to allow IPV6 addresses to be stored.
  socklen = sizeof(server_addr);
  if (0 != bind(quicsinkfps->socket, &server_addr.sa, socklen))
  {
      GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to bind socket %d", errno));
      return FALSE;
  }
  memcpy(&(quicsinkfps->local_address), &server_addr, sizeof(server_addr));

  // Initialize engine callbacks
  memset(&engine_api, 0, sizeof(engine_api));
  engine_api.ea_packets_out     = gst_quic_send_packets;
  engine_api.ea_packets_out_ctx = quicsinkfps;
  engine_api.ea_stream_if       = &quicsinkfps_callbacks;
  engine_api.ea_stream_if_ctx   = quicsinkfps;
  engine_api.ea_settings        = &engine_settings;
  engine_api.ea_get_ssl_ctx     = gst_quicsinkfps_get_ssl_ctx;

  // Instantiate engine in server mode
  quicsinkfps->engine = lsquic_engine_new(QUIC_SERVER, &engine_api);
  if (!quicsinkfps->engine)
  {
      GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to create lsquic engine"));
      return FALSE;
  }

  GST_DEBUG_OBJECT(quicsinkfps, "Initialised engine, ready to accept connections");

  while (!quicsinkfps->connection_active) {
    gst_quic_read_packets(GST_ELEMENT(quicsinkfps), quicsinkfps->socket, quicsinkfps->engine, quicsinkfps->local_address);
  }

  g_timeout_add(1, tick_connection, quicsinkfps);

  return TRUE;
}

static gboolean
gst_quicsinkfps_stop (GstBaseSink * sink)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);

  GST_OBJECT_LOCK (quicsinkfps);
  GST_DEBUG_OBJECT (quicsinkfps, "stop called, closing stream");
  if (quicsinkfps->stream) {
    lsquic_stream_close(quicsinkfps->stream);
    quicsinkfps->stream = NULL;
  }

  GST_DEBUG_OBJECT (quicsinkfps, "stop called, closing connection");
  if (quicsinkfps->connection) {
    lsquic_conn_close(quicsinkfps->connection);
    while (quicsinkfps->connection_active) {
      gst_quic_read_packets(GST_ELEMENT(quicsinkfps), quicsinkfps->socket, quicsinkfps->engine, quicsinkfps->local_address);
    }
    quicsinkfps->connection = NULL;
  }
  GST_OBJECT_UNLOCK (quicsinkfps);

  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_quicsinkfps_unlock (GstBaseSink * sink)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);

  GST_DEBUG_OBJECT (quicsinkfps, "unlock");

  return TRUE;
}

/* Clear a previously indicated unlock request not that unlocking is
 * complete. Sub-classes should clear any command queue or indicator they
 * set during unlock */
static gboolean
gst_quicsinkfps_unlock_stop (GstBaseSink * sink)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);

  GST_DEBUG_OBJECT (quicsinkfps, "unlock_stop");

  return TRUE;
}

/* notify subclass of event */
static gboolean
gst_quicsinkfps_event (GstBaseSink * sink, GstEvent * event)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);

  if (GST_EVENT_EOS == event->type) {
    GST_DEBUG_OBJECT(quicsinkfps, "GOT EOS EVENT");
    if (quicsinkfps->stream) {
      lsquic_stream_close(quicsinkfps->stream);
      quicsinkfps->stream = NULL;
    }
  }

  GST_DEBUG_OBJECT(quicsinkfps, "Chaining up");

  gboolean ret = GST_BASE_SINK_CLASS (gst_quicsinkfps_parent_class)->event(sink,event);


  return ret;
}

/* notify subclass of preroll buffer or real buffer */
static GstFlowReturn
gst_quicsinkfps_preroll (GstBaseSink * sink, GstBuffer * buffer)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);

  GST_DEBUG_OBJECT (quicsinkfps, "preroll");

  return GST_FLOW_OK;
}

/* Write data from upstream buffers to a stream.
   if the buffer marks the start of a frame, create a new stream */
static GstFlowReturn
gst_quicsinkfps_render (GstBaseSink * sink, GstBuffer * buffer)
{
  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);
  gboolean start_frame = FALSE;

  if (!quicsinkfps->connection_active) 
  {
    GST_ELEMENT_ERROR (quicsinkfps, RESOURCE, READ, (NULL),
        ("There is no active connection to send data to"));
    return GST_FLOW_ERROR;
  } 

  GST_DEBUG_OBJECT (quicsinkfps, "render -- duration: %" GST_TIME_FORMAT ",  dts: %" GST_TIME_FORMAT ",  pts: %" GST_TIME_FORMAT", GREP_MARKER pts: %lu", GST_TIME_ARGS(buffer->duration), GST_TIME_ARGS(buffer->dts), GST_TIME_ARGS(buffer->pts), buffer->pts-3600000000000000);
  if ((((buffer->pts-3600000000000000) % 41666666) == 0) && (buffer->pts != quicsinkfps->prev_buffer_pts)) {
    GST_DEBUG_OBJECT (quicsinkfps, "We have received a new frame");
    start_frame = TRUE;
  }
  quicsinkfps->prev_buffer_pts = buffer->pts;

  

  quicsinkfps->stream_ctx.buffer = buffer;
  quicsinkfps->stream_ctx.offset = 0;
  quicsinkfps->stream_ctx.buffer_size = gst_buffer_get_size(quicsinkfps->stream_ctx.buffer);

  // Simultaneous attempts to process the lsquic connection via read packets will cause 
  // An assertion error within lsquic. So we take the lock to prevent this from happening
  GST_OBJECT_LOCK(quicsinkfps);

  //At the start of a new gop, we close the old stream and create our new gop stream
  if (start_frame) {
    if (quicsinkfps->stream) {
      lsquic_stream_close(quicsinkfps->stream);
    }
    lsquic_conn_make_stream(quicsinkfps->connection);
    
  }
  lsquic_stream_wantwrite(quicsinkfps->stream, 1);

  while (quicsinkfps->stream_ctx.offset != quicsinkfps->stream_ctx.buffer_size) 
  {
    gst_quic_read_packets(GST_ELEMENT(quicsinkfps), quicsinkfps->socket, quicsinkfps->engine, quicsinkfps->local_address);
  }
  // If we do not tell lsquic to explicitly send packets after writing, 
  // it may delay until the packet can be filled with more data, this is not desired.
  lsquic_engine_send_unsent_packets (quicsinkfps->engine);
  GST_OBJECT_UNLOCK(quicsinkfps);

  return GST_FLOW_OK;
}

/* Render a BufferList */
static GstFlowReturn
gst_quicsinkfps_render_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  GstFlowReturn flow_return;
  guint i, num_buffers;

  GstQuicsinkfps *quicsinkfps = GST_QUICSINKFPS (sink);

  GST_DEBUG_OBJECT (quicsinkfps, "render_list");

  num_buffers = gst_buffer_list_length (buffer_list);
  if (num_buffers == 0) {
    GST_DEBUG_OBJECT (quicsinkfps, "Empty buffer list provided, returning immediately");
    return GST_FLOW_OK;
  }
    
  for (i = 0; i < num_buffers; i++) {

    flow_return = gst_quicsinkfps_render (sink, gst_buffer_list_get (buffer_list, i));

    if (flow_return != GST_FLOW_OK) {
      GST_WARNING_OBJECT(quicsinkfps, "flow_return was not GST_FLOW_OK, returning after sending %u/%u buffers", i+1, num_buffers);
      return flow_return;
    }

  }

  return GST_FLOW_OK;
}
