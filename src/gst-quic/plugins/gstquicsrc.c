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
#define QUIC_DEFAULT_LOG_PATH "/home/matt/Documents/lsquic-client-log.txt"

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
  PROP_LOG,
  PROP_PORT,
  PROP_CAPS
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
  quicsrc->log_file = g_strdup(QUIC_DEFAULT_LOG_PATH);
  quicsrc->socket = -1;
  quicsrc->connection_active = FALSE;
  quicsrc->engine = NULL;
  quicsrc->connection = NULL;

  quicsrc->stream_context_queue = NULL;

  /* configure basesrc to be a live source */
  gst_base_src_set_live (GST_BASE_SRC (quicsrc), TRUE);
  /* make basesrc output a segment in time */
  gst_base_src_set_format (GST_BASE_SRC (quicsrc), GST_FORMAT_TIME);
  /* make basesrc set timestamps on outgoing buffers based on the running_time
   * when they were captured */
  gst_base_src_set_do_timestamp (GST_BASE_SRC (quicsrc), TRUE);
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
    case PROP_LOG:
      if (!g_value_get_string (value)) {
        g_warning ("log file path property cannot be NULL");
        break;
      }
      g_free (quicsrc->log_file);
      quicsrc->log_file = g_strdup (g_value_get_string (value));
      break;
    //FIXME: Should we protect against this being null?
    case PROP_PORT:
      quicsrc->port = g_value_get_int (value);
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

      GST_OBJECT_LOCK (quicsrc);
      old_caps = quicsrc->caps;
      quicsrc->caps = new_caps;
      GST_OBJECT_UNLOCK (quicsrc);
      if (old_caps)
        gst_caps_unref (old_caps);

      gst_pad_mark_reconfigure (GST_BASE_SRC_PAD (quicsrc));
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_quicsrc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);
  GST_DEBUG_OBJECT (quicsrc, "get_property");

  switch (property_id) {
    case PROP_HOST:
      g_value_set_string (value, quicsrc->host);
      break;
    case PROP_LOG:
      g_value_set_string (value, quicsrc->log_file);
      break;
    case PROP_PORT:
      g_value_set_int (value, quicsrc->port);
      break;
    case PROP_CAPS:
      GST_OBJECT_LOCK (quicsrc);
      gst_value_set_caps (value, quicsrc->caps);
      GST_OBJECT_UNLOCK (quicsrc);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_quicsrc_dispose (GObject * object)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);

  GST_DEBUG_OBJECT (quicsrc, "dispose");

  lsquic_engine_destroy(quicsrc->engine);
  lsquic_global_cleanup();

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
  GstCaps *caps, *result;

  quicsrc = GST_QUICSRC (src);

  GST_OBJECT_LOCK (src);
  if ((caps = quicsrc->caps))
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

  GST_DEBUG_OBJECT (quicsrc, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (result));
  return result;
}

static lsquic_conn_ctx_t *
gst_quicsrc_on_new_conn (void *stream_if_ctx, struct lsquic_conn *conn)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (stream_if_ctx);
  GST_DEBUG_OBJECT(quicsrc,"MW: Connection created");
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
        quicsrc->connection_active = TRUE;
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
    GST_DEBUG_OBJECT(quicsrc, "MW: Accepted new stream for reading");
    lsquic_stream_wantread(stream, 1);

    struct stream_ctx* stream_ctx = malloc(sizeof(struct stream_ctx));
    stream_ctx->buffer = malloc(10000);
    stream_ctx->offset = 0;
    stream_ctx->ready = FALSE;

    quicsrc->stream_context_queue = g_list_append(quicsrc->stream_context_queue, (void *) stream_ctx);
    return (void *) stream_ctx;
}

static size_t
gst_quicsrc_readf (void *ctx, const unsigned char *data, size_t len, int fin)
{
    struct lsquic_stream *stream = (struct lsquic_stream *) ctx;
    struct stream_ctx *stream_ctx = (struct stream_ctx*) ((void *) lsquic_stream_get_ctx(stream));

    if (len) 
    {
        memcpy(stream_ctx->buffer+stream_ctx->offset, data, len);
        stream_ctx->offset += len;
        GST_DEBUG("MW: Read %lu bytes from stream", len);
    }
    if (fin)
    {
        GST_DEBUG("MW: Read end of stream");
        stream_ctx->ready = TRUE;
        lsquic_stream_shutdown(stream, 2);
    }
    return len;
}

static void
gst_quicsrc_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *lsquic_stream_ctx)
{
    ssize_t bytes_read;

    bytes_read = lsquic_stream_readf(stream, gst_quicsrc_readf, stream);
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
gst_quicsrc_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *lsquic_stream_ctx)
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
gst_quicsrc_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *lsquic_stream_ctx)
{
    struct stream_ctx *stream_ctx = (struct stream_ctx*) (stream_ctx);
    GST_DEBUG("stream closed");
}

/* Open a connection with the QUIC server */
static gboolean
gst_quicsrc_start (GstBaseSrc * src)
{
  socklen_t socklen;
  struct lsquic_engine_api engine_api;
  struct lsquic_engine_settings engine_settings;
  server_addr_u server_addr;

  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "start");

  if (0 != lsquic_global_init(LSQUIC_GLOBAL_CLIENT))
  {
    GST_ELEMENT_ERROR (quicsrc, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  printf("Host is: %s, port is: %d, log is: %s", quicsrc->host, quicsrc->port, quicsrc->log_file);
  fflush(stdout);

  /* Initialize logging */
  FILE *s_log_fh = fopen(quicsrc->log_file, "wb");

  if (0 != lsquic_set_log_level("debug"))
  {
    GST_ELEMENT_ERROR (quicsrc, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  setvbuf(s_log_fh, NULL, _IOLBF, 0);
  lsquic_logger_init(&logger_if, s_log_fh, LLTS_HHMMSSUS);

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
  engine_api.ea_packets_out = gst_quic_send_packets;
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

  while (!quicsrc->connection_active) {
    gst_quic_read_packets(GST_ELEMENT(quicsrc), quicsrc->socket, quicsrc->engine, quicsrc->local_address);
  }
  

  return TRUE;
}

static gboolean
gst_quicsrc_stop (GstBaseSrc * src)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "stop called, closing connection");
  if (quicsrc->connection && quicsrc->connection_active) 
  {
    lsquic_conn_close(quicsrc->connection);
  }

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
  gboolean new_data = FALSE;
  struct stream_ctx* stream_to_be_processed;

  if (!quicsrc->connection_active) {
    return GST_FLOW_EOS;
  }

  if (quicsrc->stream_context_queue != NULL) {
    if (((struct stream_ctx*) (quicsrc->stream_context_queue->data))->ready) {
      new_data = TRUE;
      stream_to_be_processed = quicsrc->stream_context_queue->data;
      quicsrc->stream_context_queue = quicsrc->stream_context_queue->next;
    }
  }

  while (!new_data)
  {
    gst_quic_read_packets(GST_ELEMENT(quicsrc), quicsrc->socket, quicsrc->engine, quicsrc->local_address);
    if (quicsrc->stream_context_queue != NULL) {
      if (((struct stream_ctx*) (quicsrc->stream_context_queue->data))->ready) {
        new_data = TRUE;
        stream_to_be_processed = quicsrc->stream_context_queue->data;
        quicsrc->stream_context_queue = quicsrc->stream_context_queue->next;
      }
    }
  }

  *outbuf = gst_buffer_new_and_alloc(stream_to_be_processed->offset);

  gst_buffer_map (*outbuf, &map, GST_MAP_READWRITE);

  memcpy(map.data, stream_to_be_processed->buffer, stream_to_be_processed->offset);

  gst_buffer_unmap(*outbuf, &map);
  gst_buffer_resize(*outbuf, 0, stream_to_be_processed->offset);

  GST_DEBUG_OBJECT (quicsrc, "created buffer of size %lu", gst_buffer_get_size(*outbuf));

  return GST_FLOW_OK;
}
