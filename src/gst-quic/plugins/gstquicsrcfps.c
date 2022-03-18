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
 * SECTION:element-gstquicsrcfps
 *
 * The quicsrcfps element receives data over the network using the QUIC protocol.
 * It is currently set up for testing purposes only and so only works with rtp packets.
 * Each rtp packet comes in its own stream.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 quicsrcfps host={addr} port=5000 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96, seqnum-base=(int)0\" ! rtpjitterbuffer latency={buffer_delay} ! rtph264depay ! h264parse ! queue ! decodebin ! videoconvert !  fakesink -v
 * ]|
 * Receive data over the network using quicsrcfps
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstquicsrcfps.h"
#include "gstquicutils.h"

#define QUIC_CLIENT 0
#define QUIC_DEFAULT_PORT 12345
#define QUIC_DEFAULT_HOST "127.0.0.1"
#define QUIC_DEFAULT_LOG_PATH "/home/matt/Documents/lsquic-client-log.txt"
#define READ_BUFFER_SIZE 100000

GST_DEBUG_CATEGORY_STATIC (gst_quicsrcfps_debug_category);
#define GST_CAT_DEFAULT gst_quicsrcfps_debug_category

/* pad templates */

static GstStaticPadTemplate gst_quicsrcfps_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* property templates */

enum
{
  PROP_0,
  PROP_HOST,
  PROP_LOG,
  PROP_PORT,
  PROP_CAPS
};


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstQuicsrcfps, gst_quicsrcfps, GST_TYPE_PUSH_SRC,
    GST_DEBUG_CATEGORY_INIT (gst_quicsrcfps_debug_category, "quicsrcfps", 0,
        "debug category for quicsrcfps element"));

/* prototypes */

static void gst_quicsrcfps_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_quicsrcfps_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static void gst_quicsrcfps_dispose (GObject * object);
static void gst_quicsrcfps_finalize (GObject * object);

static GstCaps *gst_quicsrcfps_get_caps (GstBaseSrc * src, GstCaps * filter);

static gboolean gst_quicsrcfps_start (GstBaseSrc * src);
static gboolean gst_quicsrcfps_stop (GstBaseSrc * src);

static gboolean gst_quicsrcfps_unlock (GstBaseSrc * src);
static gboolean gst_quicsrcfps_unlock_stop (GstBaseSrc * src);
static gboolean gst_quicsrcfps_event (GstBaseSrc * src, GstEvent * event);

static GstFlowReturn gst_quicsrcfps_create (GstPushSrc * src, GstBuffer ** outbuf);

/* Quic functions */
static lsquic_conn_ctx_t *gst_quicsrcfps_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn);
static void gst_quicsrcfps_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status);
static void gst_quicsrcfps_on_conn_closed (struct lsquic_conn *conn);
static lsquic_stream_ctx_t *gst_quicsrcfps_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream);
static size_t gst_quicsrcfps_readf (void *ctx, const unsigned char *data, size_t len, int fin);
static void gst_quicsrcfps_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsrcfps_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);
static void gst_quicsrcfps_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *stream_ctx);

/* QUIC callbacks passed to quic engine */
static struct lsquic_stream_if quicsrcfps_callbacks =
{
    .on_new_conn        = gst_quicsrcfps_on_new_conn,
    .on_hsk_done        = gst_quicsrcfps_on_hsk_done,
    .on_conn_closed     = gst_quicsrcfps_on_conn_closed,
    .on_new_stream      = gst_quicsrcfps_on_new_stream,
    .on_read            = gst_quicsrcfps_on_read,
    .on_write           = gst_quicsrcfps_on_write,
    .on_close           = gst_quicsrcfps_on_close,
};

static void
gst_quicsrcfps_class_init (GstQuicsrcfpsClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *push_src_class = (GstPushSrcClass *) klass;

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_quicsrcfps_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "QUIC packet receiver", "Source/Network", "Receive packets over the network via QUIC",
      "Matthew Walker <mjwalker2299@gmail.com>");

  gobject_class->set_property = gst_quicsrcfps_set_property;
  gobject_class->get_property = gst_quicsrcfps_get_property;
  gobject_class->dispose = gst_quicsrcfps_dispose;
  gobject_class->finalize = gst_quicsrcfps_finalize;

  g_object_class_install_property (gobject_class, PROP_HOST,
      g_param_spec_string ("host", "Host",
          "The server IP address to connect to", QUIC_DEFAULT_HOST,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_LOG,
      g_param_spec_string ("log", "Log",
          "The path to the lsquic log output file", QUIC_DEFAULT_LOG_PATH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "Port", "The port used by the quic server", 0,
          G_MAXUINT16, QUIC_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CAPS,
      g_param_spec_boxed ("caps", "Caps",
          "The caps of the source pad", GST_TYPE_CAPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_quicsrcfps_get_caps);

  base_src_class->start = GST_DEBUG_FUNCPTR (gst_quicsrcfps_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_quicsrcfps_stop);

  base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_quicsrcfps_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_quicsrcfps_unlock_stop);
  base_src_class->event = GST_DEBUG_FUNCPTR (gst_quicsrcfps_event);

  push_src_class->create = GST_DEBUG_FUNCPTR (gst_quicsrcfps_create);
}

static gboolean
tick_connection (gpointer context) 
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (context);

  GST_OBJECT_LOCK(quicsrcfps);
  if (quicsrcfps->engine) {
    gst_quic_read_packets(GST_ELEMENT(quicsrcfps), quicsrcfps->socket, quicsrcfps->engine, quicsrcfps->local_address);
    GST_OBJECT_UNLOCK(quicsrcfps);
    return TRUE;
  } else {
    GST_OBJECT_UNLOCK(quicsrcfps);
    return FALSE;
  }

}

static void
gst_quicsrcfps_init (GstQuicsrcfps * quicsrcfps)
{
  quicsrcfps->port = QUIC_DEFAULT_PORT;
  quicsrcfps->host = g_strdup (QUIC_DEFAULT_HOST);
  quicsrcfps->log_file = g_strdup(QUIC_DEFAULT_LOG_PATH);
  quicsrcfps->socket = -1;
  quicsrcfps->connection_active = FALSE;
  quicsrcfps->engine = NULL;
  quicsrcfps->connection = NULL;
  quicsrcfps->stream_count = 0;

  quicsrcfps->stream_context_queue = NULL;

  /* configure basesrc to be a live source */
  gst_base_src_set_live (GST_BASE_SRC (quicsrcfps), TRUE);
  /* make basesrc output a segment in time */
  gst_base_src_set_format (GST_BASE_SRC (quicsrcfps), GST_FORMAT_TIME);
  /* make basesrc set timestamps on outgoing buffers based on the running_time
   * when they were captured */
  gst_base_src_set_do_timestamp (GST_BASE_SRC (quicsrcfps), TRUE);
}

static void
gst_quicsrcfps_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (object);

  GST_DEBUG_OBJECT (quicsrcfps, "set_property");

  switch (property_id) {
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (quicsrcfps->host);
      quicsrcfps->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_LOG:
      if (!g_value_get_string (value)) {
        g_warning ("log file path property cannot be NULL");
        break;
      }
      g_free (quicsrcfps->log_file);
      quicsrcfps->log_file = g_strdup (g_value_get_string (value));
      break;
    //FIXME: Should we protect against this being null?
    case PROP_PORT:
      quicsrcfps->port = g_value_get_int (value);
      break;
    case PROP_CAPS:
    {
      const GstCaps *new_caps_val = gst_value_get_caps (value);
      GstCaps *new_caps;
      GstCaps *old_caps;

      if (new_caps_val == NULL) {
        new_caps = gst_caps_new_any ();
      } else {
        new_caps = gst_caps_copy (new_caps_val);
      }

      GST_OBJECT_LOCK (quicsrcfps);
      old_caps = quicsrcfps->caps;
      quicsrcfps->caps = new_caps;
      GST_OBJECT_UNLOCK (quicsrcfps);
      if (old_caps)
        gst_caps_unref (old_caps);

      gst_pad_mark_reconfigure (GST_BASE_SRC_PAD (quicsrcfps));
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_quicsrcfps_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (object);
  GST_DEBUG_OBJECT (quicsrcfps, "get_property");

  switch (property_id) {
    case PROP_HOST:
      g_value_set_string (value, quicsrcfps->host);
      break;
    case PROP_LOG:
      g_value_set_string (value, quicsrcfps->log_file);
      break;
    case PROP_PORT:
      g_value_set_int (value, quicsrcfps->port);
      break;
    case PROP_CAPS:
      GST_OBJECT_LOCK (quicsrcfps);
      gst_value_set_caps (value, quicsrcfps->caps);
      GST_OBJECT_UNLOCK (quicsrcfps);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_quicsrcfps_dispose (GObject * object)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (object);

  GST_DEBUG_OBJECT (quicsrcfps, "dispose");

  if (quicsrcfps->connection) {
    GST_DEBUG_OBJECT (quicsrcfps, "dispose called, closing connection");
    lsquic_conn_close(quicsrcfps->connection);
    while (quicsrcfps->connection_active) {
      gst_quic_read_packets(GST_ELEMENT(quicsrcfps), quicsrcfps->socket, quicsrcfps->engine, quicsrcfps->local_address);
    }
    quicsrcfps->connection = NULL;
  } else {
    GST_DEBUG_OBJECT (quicsrcfps, "dispose called, but connection has already been closed connection");
  }

  if (quicsrcfps->engine) {
    lsquic_engine_destroy(quicsrcfps->engine);
  }
  lsquic_global_cleanup();

  G_OBJECT_CLASS (gst_quicsrcfps_parent_class)->dispose (object);
}

void
gst_quicsrcfps_finalize (GObject * object)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (object);

  GST_DEBUG_OBJECT (quicsrcfps, "finalize");

  lsquic_global_cleanup();

  /* clean up object here */

  G_OBJECT_CLASS (gst_quicsrcfps_parent_class)->finalize (object);
}

/* Provides the basesrc with possible pad capabilities
   to allow it to respond to queries from downstream elements.
   Since the data transmitted over QUIC could be of any type, we 
   return either ANY caps or, if provided, the caps of the
   downstream element */
static GstCaps *
gst_quicsrcfps_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstQuicsrcfps *quicsrcfps;
  GstCaps *caps, *result;

  quicsrcfps = GST_QUICSRCFPS (src);

  GST_OBJECT_LOCK (src);
  if ((caps = quicsrcfps->caps))
    gst_caps_ref (caps);
  GST_OBJECT_UNLOCK (src);

  if (caps) {
    if (filter) {
      result = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
      gst_caps_unref (caps);
    } else {
      result = caps;
    }
  } else {
    result = (filter) ? gst_caps_ref (filter) : gst_caps_new_any ();
  }

  GST_DEBUG_OBJECT (quicsrcfps, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (result));
  return result;
}

static lsquic_conn_ctx_t *
gst_quicsrcfps_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (stream_if_ctx);
  GST_DEBUG_OBJECT(quicsrcfps,"MW: Connection created");
  return (void *) quicsrcfps;
}

static void
gst_quicsrcfps_on_hsk_done(lsquic_conn_t *conn, enum lsquic_hsk_status status) 
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS ((void *) lsquic_conn_get_ctx(conn));

    switch (status)
    {
    case LSQ_HSK_OK:
    case LSQ_HSK_RESUMED_OK:
        GST_DEBUG_OBJECT(quicsrcfps, "MW: Handshake completed successfully");
        quicsrcfps->connection_active = TRUE;
        break;
    default:
        GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
          ("Handshake with server failed"));
        break;
    }
}

static void
gst_quicsrcfps_on_conn_closed (struct lsquic_conn *conn)
{
    GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS ((void *) lsquic_conn_get_ctx(conn));

    GST_DEBUG_OBJECT(quicsrcfps, "MW: Connection closed, send EOS on next create call");
    quicsrcfps->connection_active = FALSE;
}

static lsquic_stream_ctx_t *
gst_quicsrcfps_on_new_stream (void *stream_if_ctx, struct lsquic_stream *stream)
{
    GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (stream_if_ctx);
    lsquic_stream_wantread(stream, 1);

    struct stream_ctx* stream_ctx = malloc(sizeof(struct stream_ctx));
    stream_ctx->buffer = malloc(READ_BUFFER_SIZE);
    stream_ctx->offset = 0;
    stream_ctx->processed = 0;
    stream_ctx->ready = FALSE;
    stream_ctx->data_available = FALSE;
    stream_ctx->streamID = quicsrcfps->stream_count;
    quicsrcfps->stream_count++;

    GST_DEBUG_OBJECT(quicsrcfps, "MW: Accepted new stream for reading, ID: %u", stream_ctx->streamID);

    quicsrcfps->stream_context_queue = g_list_append(quicsrcfps->stream_context_queue, (void *) stream_ctx);
    return (void *) stream_ctx;
}

static size_t
gst_quicsrcfps_readf (void *ctx, const unsigned char *data, size_t len, int fin)
{
    struct lsquic_stream *stream = (struct lsquic_stream *) ctx;
    struct stream_ctx *stream_ctx = (struct stream_ctx*) ((void *) lsquic_stream_get_ctx(stream));

    if (len) 
    {
        memcpy(stream_ctx->buffer+stream_ctx->offset, data, len);
        stream_ctx->offset += len;
        GST_DEBUG("MW: Read %lu bytes from stream, ID: %u", len, stream_ctx->streamID);
        stream_ctx->data_available = TRUE;
    }
    if (fin)
    {
        GST_DEBUG("MW: Read end of stream, ID: %u", stream_ctx->streamID);
        stream_ctx->ready = TRUE;
        lsquic_stream_shutdown(stream, 2);
    }
    return len;
}

static void
gst_quicsrcfps_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *lsquic_stream_ctx)
{
    ssize_t bytes_read;

    bytes_read = lsquic_stream_readf(stream, gst_quicsrcfps_readf, stream);
    if (bytes_read < 0)
    {
        if (errno != EWOULDBLOCK) {
          GST_WARNING("Error while reading from quic stream, closing stream");
          lsquic_stream_close(stream);
        }
    }
}

//FIXME: This function is set up for test purposes and should probably be disbaled as we have no need to write.
static void
gst_quicsrcfps_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *lsquic_stream_ctx)
{
    ssize_t bytes_written;

    lsquic_conn_t *conn = lsquic_stream_conn(stream);
    struct stream_ctx *stream_ctx = (struct stream_ctx*) (lsquic_stream_ctx);

    GST_DEBUG("writing stream");


    char test_string[100] = "Jack and Jill Went up the hill";

    bytes_written = lsquic_stream_write(stream, test_string, sizeof(test_string));
    if (bytes_written > 0)
    {
        if (bytes_written == sizeof(test_string))
        {
            GST_DEBUG("MW: wrote full test string to stream");
            lsquic_stream_shutdown(stream, 1);  /* This flushes as well */
            lsquic_stream_wantread(stream, 1);
        }
    }
    else
    {
        GST_DEBUG("stream_write() wrote zero bytes. This should not be possible and indicates a serious error. Aborting conn");
        lsquic_conn_abort(conn);
    }
}

static void
gst_quicsrcfps_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *lsquic_stream_ctx)
{
    struct stream_ctx *stream_ctx = (struct stream_ctx*) (stream_ctx);
    GST_DEBUG("stream closed");
}

/* Open a connection with the QUIC server */
static gboolean
gst_quicsrcfps_start (GstBaseSrc * src)
{
  socklen_t socklen;
  struct lsquic_engine_api engine_api;
  struct lsquic_engine_settings engine_settings;
  server_addr_u server_addr;

  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);

  GST_DEBUG_OBJECT (quicsrcfps, "start");
  GST_DEBUG_OBJECT (quicsrcfps, "Host is: %s, port is: %d, log is: %s", quicsrcfps->host, quicsrcfps->port, quicsrcfps->log_file);

  if (0 != lsquic_global_init(LSQUIC_GLOBAL_CLIENT))
  {
    GST_ELEMENT_ERROR (quicsrcfps, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }
  
  /* Initialize logging */
  FILE *s_log_fh = fopen(quicsrcfps->log_file, "wb");

  if (0 != lsquic_set_log_level("debug"))
  {
    GST_ELEMENT_ERROR (quicsrcfps, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  setvbuf(s_log_fh, NULL, _IOLBF, 0);
  lsquic_logger_init(&logger_if, s_log_fh, LLTS_HHMMSSUS);

  // Initialize engine settings to default values
  lsquic_engine_init_settings(&engine_settings, QUIC_CLIENT);

  // Disable delayed acks to improve response to loss
  engine_settings.es_delayed_acks = 0;

  // The max stream flow control window seems to default to 16384.
  // This ends up causing streams to become blocked frequently, leading
  // to delays. After experimentation, a value of 524288 was found to be 
  // large enough that blocks do not occur.
  engine_settings.es_max_sfcw = 524288;

  engine_settings.es_base_plpmtu = 1472;

	engine_settings.es_pace_packets = FALSE;

  /// The initial stream flow control offset on the client side is 16384.
  // However, the server appears to begin with a much higher max send offset
  // It should be zero, but instead it's 6291456. We can force lsquic to behave
  // by setting the following to parameters. Initially, I experiemented with
  // setting these values to 16384, but, as lsquic waits until we have read up
  // to half the stream flow control offset, this causes the window to grow too slowly.
  engine_settings.es_init_max_stream_data_bidi_local = 16384*4;
  engine_settings.es_init_max_stream_data_bidi_remote = 16384*4;

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
  if (!gst_quic_set_addr(quicsrcfps->host, quicsrcfps->port, &server_addr))
  {
      GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to resolve host: %s", quicsrcfps->host));
      return FALSE;
  }

  // Create socket
  quicsrcfps->socket = socket(server_addr.sa.sa_family, SOCK_DGRAM, 0);

  if (quicsrcfps->socket < 0)
  {
    GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
      ("Failed to open socket"));
    return FALSE;
  }

  // set socket to be non-blocking
  int socket_flags = fcntl(quicsrcfps->socket, F_GETFL);
  if (socket_flags == -1) 
  {
    GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to retrieve socket_flags using fcntl"));
    return FALSE;
  }

  socket_flags |= O_NONBLOCK;
  if (0 != fcntl(quicsrcfps->socket, F_SETFL, socket_flags)) 
  {
    GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to set socket_flags using fcntl"));
    return FALSE;
  }

  // Activate IP_TOS field manipulation to allow ecn to be set
  int activate, socket_opt_result;

  activate = 1;
  if (AF_INET == server_addr.sa.sa_family)
  {
      socket_opt_result = setsockopt(quicsrcfps->socket, IPPROTO_IP, IP_RECVTOS, &activate, sizeof(activate));
  }
  else
  {
      socket_opt_result = setsockopt(quicsrcfps->socket, IPPROTO_IPV6, IPV6_RECVTCLASS, &activate, sizeof(activate));
  }

  if (0 != socket_opt_result) {
    GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add ecn support using setsockopt"));
    return FALSE;
  }

  // Bind local address to socket.
  quicsrcfps->local_address.ss_family = server_addr.sa.sa_family;
  socklen = sizeof(quicsrcfps->local_address);
  if (0 != bind(quicsrcfps->socket,
                  (struct sockaddr *) &(quicsrcfps->local_address), socklen))
  {
      GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to bind socket"));
      return FALSE;
  }

  // Initialize engine callbacks
  memset(&engine_api, 0, sizeof(engine_api));
  engine_api.ea_packets_out = gst_quic_send_packets;
  engine_api.ea_packets_out_ctx = quicsrcfps;
  engine_api.ea_stream_if   = &quicsrcfps_callbacks;
  engine_api.ea_stream_if_ctx = quicsrcfps;
  engine_api.ea_settings = &engine_settings;

  // Instantiate engine in client mode
  quicsrcfps->engine = lsquic_engine_new(QUIC_CLIENT, &engine_api);
  if (!quicsrcfps->engine)
  {
      GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
        ("Failed to create lsquic engine"));
      return FALSE;
  }


  GST_DEBUG_OBJECT(quicsrcfps, "Creating connection to server at %s:%d", quicsrcfps->host, quicsrcfps->port);
  // Create connection
  quicsrcfps->connection = lsquic_engine_connect(
      quicsrcfps->engine, LSQVER_I001,
      (struct sockaddr *) &(quicsrcfps->local_address), &server_addr.sa,
      (void *) (uintptr_t) quicsrcfps->socket,  /* Peer ctx */
      NULL, NULL, 0, NULL, 0, NULL, 0);
  if (!quicsrcfps->connection)
  {
    GST_ELEMENT_ERROR (quicsrcfps, RESOURCE, OPEN_READ, (NULL),
      ("Failed to create QUIC connection to server"));
      return FALSE;
  }

  lsquic_engine_process_conns(quicsrcfps->engine);

  while (!quicsrcfps->connection_active) {
    gst_quic_read_packets(GST_ELEMENT(quicsrcfps), quicsrcfps->socket, quicsrcfps->engine, quicsrcfps->local_address);
  }

  g_timeout_add(1, tick_connection, quicsrcfps);
  

  return TRUE;
}

static gboolean
gst_quicsrcfps_stop (GstBaseSrc * src)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);

  GST_DEBUG_OBJECT (quicsrcfps, "stop called, closing connection");
  if (quicsrcfps->connection && quicsrcfps->connection_active) 
  {
    lsquic_conn_close(quicsrcfps->connection);
  }

  return TRUE;
}

// /* given a buffer, return start and stop time when it should be pushed
//  * out. The base class will sync on the clock using these times. */
// static void
// gst_quicsrcfps_get_times (GstBaseSrc * src, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end)
// {
//   GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);

//   GST_DEBUG_OBJECT (quicsrcfps, "get_times");

// }

// /* get the total size of the resource in bytes */
// static gboolean
// gst_quicsrcfps_get_size (GstBaseSrc * src, guint64 * size)
// {
//   GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);

//   GST_DEBUG_OBJECT (quicsrcfps, "get_size");

//   return TRUE;
// }

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_quicsrcfps_unlock (GstBaseSrc * src)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);

  GST_DEBUG_OBJECT (quicsrcfps, "unlock");

  return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean
gst_quicsrcfps_unlock_stop (GstBaseSrc * src)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);

  GST_DEBUG_OBJECT (quicsrcfps, "unlock_stop");

  return TRUE;
}

/* notify subclasses of an event */
static gboolean
gst_quicsrcfps_event (GstBaseSrc * src, GstEvent * event)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);

  // GST_DEBUG_OBJECT (quicsrcfps, "event");

  return TRUE;
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
static GstFlowReturn
gst_quicsrcfps_create (GstPushSrc * src, GstBuffer ** outbuf)
{
  GstQuicsrcfps *quicsrcfps = GST_QUICSRCFPS (src);
  GstMapInfo map;
  gboolean new_data = FALSE;
  GList * stream_context_queue = NULL;
  GList * list_element_to_be_processed = NULL;
  struct stream_ctx* stream_to_be_processed = NULL;
  struct stream_ctx* current_stream = NULL;
  guint16 next_packet_length = 0;

  if (!quicsrcfps->connection_active) {
    return GST_FLOW_EOS;
  }

  // Iterate through all of our stream conetexts to find complete streams.
  // The first completed stream found is converted to a buffer and pushed downstream
  // If no completed streams are available, we will read packets until one is
  stream_context_queue = quicsrcfps->stream_context_queue;
  while (stream_context_queue != NULL) {
    current_stream =  ((struct stream_ctx*) (stream_context_queue->data));
    if (current_stream->ready && current_stream->processed == current_stream->offset) {
      // Free resources from stream context if we have reached end of stream
      quicsrcfps->stream_context_queue = g_list_remove_link (quicsrcfps->stream_context_queue, stream_context_queue);
      GList * temp = stream_context_queue->next;
      free(current_stream->buffer);
      free(current_stream);
      g_list_free (stream_context_queue);
      stream_context_queue = temp;
    } else if (current_stream->data_available) {
      guint8 octet1 = current_stream->buffer[current_stream->processed+0];
      guint8 octet2 = current_stream->buffer[current_stream->processed+1];
      next_packet_length = (octet1 << 8) + octet2;
      if ((current_stream->offset - current_stream->processed) >= next_packet_length + 2) {
          new_data = TRUE;
          list_element_to_be_processed = stream_context_queue;
          break;
      }
    } else {
      stream_context_queue = stream_context_queue->next;
    }
  }

  GST_OBJECT_LOCK(quicsrcfps);
  while (!new_data && quicsrcfps->connection_active)
  {
    gst_quic_read_packets(GST_ELEMENT(quicsrcfps), quicsrcfps->socket, quicsrcfps->engine, quicsrcfps->local_address);
    stream_context_queue = quicsrcfps->stream_context_queue;
    while (stream_context_queue != NULL) {
      current_stream =  ((struct stream_ctx*) (stream_context_queue->data));
      if (current_stream->ready && current_stream->processed == current_stream->offset) {
        // Free resources from stream context if we have reached end of stream
        quicsrcfps->stream_context_queue = g_list_remove_link (quicsrcfps->stream_context_queue, stream_context_queue);
        GList * temp = stream_context_queue->next;
        free(current_stream->buffer);
        free(current_stream);
        g_list_free (stream_context_queue);
        stream_context_queue = temp;
      } else if (current_stream->data_available) {
        guint8 octet1 = current_stream->buffer[current_stream->processed+0];
        guint8 octet2 = current_stream->buffer[current_stream->processed+1];
        next_packet_length = (octet1 << 8) + octet2;
        if ((current_stream->offset - current_stream->processed) >= next_packet_length + 2) {
          new_data = TRUE;
          list_element_to_be_processed = stream_context_queue;
          break;
        } else {
          current_stream->data_available = FALSE;
          stream_context_queue = stream_context_queue->next;
        }
      } else {
        stream_context_queue = stream_context_queue->next;
      }
    }
  }
  GST_OBJECT_UNLOCK(quicsrcfps);

  // Connection may close while we are trying to read.
  if (!quicsrcfps->connection_active) {
    return GST_FLOW_EOS;
  }


  // Create buffer from stream context
  stream_to_be_processed = list_element_to_be_processed->data;

  *outbuf = gst_buffer_new_and_alloc(next_packet_length);

  gst_buffer_map (*outbuf, &map, GST_MAP_READWRITE);

  memcpy(map.data, stream_to_be_processed->buffer+stream_to_be_processed->processed+2, next_packet_length);
  stream_to_be_processed->processed += next_packet_length+2;
  if (stream_to_be_processed->processed == stream_to_be_processed->offset) {
    stream_to_be_processed->data_available = FALSE;
  }
  
  gst_buffer_unmap(*outbuf, &map);
  gst_buffer_resize(*outbuf, 0, next_packet_length);



  GST_DEBUG_OBJECT (quicsrcfps, "Read data from stream %u. Pushing buffer of size %lu", stream_to_be_processed->streamID, gst_buffer_get_size(*outbuf));

  return GST_FLOW_OK;
}
