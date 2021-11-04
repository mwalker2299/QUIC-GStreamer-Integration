/* GStreamer
 * Copyright (C) 2021 FIXME <fixme@example.com>
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
 * SECTION:element-gstquicsink
 *
 * The quicsink element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! quicsink ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "gstquicsink.h"
#include "gstquicutils.h"

//FIXME: Theses are test defaults and should be updated to a more appropriate value
#define QUIC_SERVER 1
#define QUIC_DEFAULT_PORT 12345
#define QUIC_DEFAULT_HOST "127.0.0.1"
#define QUIC_DEFAULT_CERTIFICATE_PATH "/home/matt/Documents/lsquic-tutorial/mycert-cert.pem"
#define QUIC_DEFAULT_KEY_PATH "/home/matt/Documents/lsquic-tutorial/mycert-key.pem"

GST_DEBUG_CATEGORY_STATIC (gst_quicsink_debug_category);
#define GST_CAT_DEFAULT gst_quicsink_debug_category

/* gstreamer method prototypes */

static void gst_quicsink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_quicsink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_quicsink_dispose (GObject * object);
static void gst_quicsink_finalize (GObject * object);

static GstCaps *gst_quicsink_get_caps (GstBaseSink * sink, GstCaps * filter);
// static gboolean gst_quicsink_set_caps (GstBaseSink * sink, GstCaps * caps);
// static GstCaps *gst_quicsink_fixate (GstBaseSink * sink, GstCaps * caps);
// static gboolean gst_quicsink_activate_pull (GstBaseSink * sink,
//     gboolean active);
// static void gst_quicsink_get_times (GstBaseSink * sink, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end);
// static gboolean gst_quicsink_propose_allocation (GstBaseSink * sink,
//     GstQuery * query);
static gboolean gst_quicsink_start (GstBaseSink * sink);
static gboolean gst_quicsink_stop (GstBaseSink * sink);
static gboolean gst_quicsink_unlock (GstBaseSink * sink);
static gboolean gst_quicsink_unlock_stop (GstBaseSink * sink);
// static gboolean gst_quicsink_query (GstBaseSink * sink, GstQuery * query);
// static gboolean gst_quicsink_event (GstBaseSink * sink, GstEvent * event);
// static GstFlowReturn gst_quicsink_wait_event (GstBaseSink * sink,
//     GstEvent * event);
// static GstFlowReturn gst_quicsink_prepare (GstBaseSink * sink,
//     GstBuffer * buffer);
// static GstFlowReturn gst_quicsink_prepare_list (GstBaseSink * sink,
//     GstBufferList * buffer_list);
static GstFlowReturn gst_quicsink_preroll (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_quicsink_render (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_quicsink_render_list (GstBaseSink * sink,
    GstBufferList * buffer_list);


/* lsquic method prototypes */
static void gst_quicsink_load_cert_and_key (GstQuicsink * quicsink);

enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT,
  PROP_CERT,
  PROP_KEY
};

/* pad templates */

static GstStaticPadTemplate gst_quicsink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstQuicsink, gst_quicsink, GST_TYPE_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT (gst_quicsink_debug_category, "quicsink", 0,
        "debug category for quicsink element"));

static void
gst_quicsink_class_init (GstQuicsinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_quicsink_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "QUIC packet sender", "Sink/Network", "Send packets over the network via QUIC",
      "Matthew Walker <mjwalker2299@gmail.com>");

  gobject_class->set_property = gst_quicsink_set_property;
  gobject_class->get_property = gst_quicsink_get_property;
  gobject_class->dispose = gst_quicsink_dispose;
  gobject_class->finalize = gst_quicsink_finalize;

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
  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "Port", "The port used by the quic server", 0,
          G_MAXUINT16, QUIC_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_sink_class->get_caps = GST_DEBUG_FUNCPTR (gst_quicsink_get_caps);
  // base_sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_quicsink_set_caps);
  // base_sink_class->fixate = GST_DEBUG_FUNCPTR (gst_quicsink_fixate);
  // base_sink_class->activate_pull =
  //     GST_DEBUG_FUNCPTR (gst_quicsink_activate_pull);
  // base_sink_class->get_times = GST_DEBUG_FUNCPTR (gst_quicsink_get_times);
  // base_sink_class->propose_allocation =
  //     GST_DEBUG_FUNCPTR (gst_quicsink_propose_allocation);
  base_sink_class->start = GST_DEBUG_FUNCPTR (gst_quicsink_start);
  base_sink_class->stop = GST_DEBUG_FUNCPTR (gst_quicsink_stop);
  base_sink_class->unlock = GST_DEBUG_FUNCPTR (gst_quicsink_unlock);
  base_sink_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_quicsink_unlock_stop);
  // base_sink_class->query = GST_DEBUG_FUNCPTR (gst_quicsink_query);
  // base_sink_class->event = GST_DEBUG_FUNCPTR (gst_quicsink_event);
  // base_sink_class->wait_event = GST_DEBUG_FUNCPTR (gst_quicsink_wait_event);
  // base_sink_class->prepare = GST_DEBUG_FUNCPTR (gst_quicsink_prepare);
  // base_sink_class->prepare_list = GST_DEBUG_FUNCPTR (gst_quicsink_prepare_list);
  base_sink_class->preroll = GST_DEBUG_FUNCPTR (gst_quicsink_preroll);
  base_sink_class->render = GST_DEBUG_FUNCPTR (gst_quicsink_render);
  base_sink_class->render_list = GST_DEBUG_FUNCPTR (gst_quicsink_render_list);

}

/* This creates a new ssl context. TLS_method indicates that the highest 
 * mutually supported version will be negotiated. Since quic requires TLS 1.3
 * at minimum, we later restrict the possible protocol versions to TLS 1.3.
 * If there are any SSL error, a Gstreamer error will be thrown, causing the 
 * pipeline to be reset.
*/
static void
gst_quicsink_load_cert_and_key (GstQuicsink * quicsink)
{
    quicsink->ssl_ctx = SSL_CTX_new(TLS_method());
    if (!quicsink->ssl_ctx)
    {
        GST_ELEMENT_ERROR (quicsink, LIBRARY, FAILED,
          (NULL),
          ("Could not create an SSL context"));
    }
    SSL_CTX_set_min_proto_version(quicsink->ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(quicsink->ssl_ctx, TLS1_3_VERSION);
    if (!SSL_CTX_use_certificate_chain_file(quicsink->ssl_ctx, quicsink->cert_file))
    {
        SSL_CTX_free(quicsink->ssl_ctx);
        GST_ELEMENT_ERROR (quicsink, LIBRARY, FAILED,
          (NULL),
          ("SSL_CTX_use_certificate_chain_file failed, is the path to the cert file correct?"));
    }
    if (!SSL_CTX_use_PrivateKey_file(quicsink->ssl_ctx, quicsink->key_file,
                                                            SSL_FILETYPE_PEM))
    {
        SSL_CTX_free(quicsink->ssl_ctx);
        GST_ELEMENT_ERROR (quicsink, LIBRARY, FAILED,
          (NULL),
          ("SSL_CTX_use_PrivateKey_file failed, is the path to the key file correct?"));
    }
}

static void
gst_quicsink_init (GstQuicsink * quicsink)
{
  quicsink->port = QUIC_DEFAULT_PORT;
  quicsink->host = g_strdup (QUIC_DEFAULT_HOST);
  quicsink->cert_file = g_strdup (QUIC_DEFAULT_CERTIFICATE_PATH);
  quicsink->key_file = g_strdup (QUIC_DEFAULT_KEY_PATH);

  quicsink->socket = -1;
  quicsink->connection_active = FALSE;
  quicsink->engine = NULL;
  quicsink->connection = NULL;
  quicsink->ssl_ctx = NULL;

  memset(&quicsink->stream_ctx, 0, sizeof(quicsink->stream_ctx));
}

void
gst_quicsink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQuicsink *quicsink = GST_QUICSINK (object);

  GST_DEBUG_OBJECT (quicsink, "set_property");

  switch (property_id) {
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (quicsink->host);
      quicsink->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_PORT:
      quicsink->port = g_value_get_int (value);
      break;
    case PROP_CERT:
      if (!g_value_get_string (value)) {
        g_warning ("SSL certificate path property cannot be NULL");
        break;
      }
      g_free (quicsink->cert_file);
      quicsink->cert_file = g_strdup (g_value_get_string (value));
      break;
    case PROP_KEY:
      if (!g_value_get_string (value)) {
        g_warning ("SSL key path property cannot be NULL");
        break;
      }
      g_free (quicsink->key_file);
      quicsink->key_file = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_quicsink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstQuicsink *quicsink = GST_QUICSINK (object);
  GST_DEBUG_OBJECT (quicsink, "get_property");

  switch (property_id) {
    case PROP_HOST:
      g_value_set_string (value, quicsink->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, quicsink->port);
      break;
    case PROP_CERT:
      g_value_set_string (value, quicsink->cert_file);
      break;
    case PROP_KEY:
      g_value_set_string (value, quicsink->key_file);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_quicsink_dispose (GObject * object)
{
  GstQuicsink *quicsink = GST_QUICSINK (object);

  GST_DEBUG_OBJECT (quicsink, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_quicsink_parent_class)->dispose (object);
}

void
gst_quicsink_finalize (GObject * object)
{
  GstQuicsink *quicsink = GST_QUICSINK (object);

  GST_DEBUG_OBJECT (quicsink, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_quicsink_parent_class)->finalize (object);
}

/* Provides the basesink with possible pad capabilities
   to allow it to respond to queries from upstream elements.
   Since the data transmitted over QUIC could be of any type, we 
   return either ANY caps or, if provided, the caps of the
   upstream element */
static GstCaps *
gst_quicsink_get_caps (GstBaseSink * sink, GstCaps * filter)
{
  GstQuicsink *quicsink;
  GstCaps *caps = NULL;

  quicsink = GST_QUICSINK (sink);

  caps = (filter ? gst_caps_ref (filter) : gst_caps_new_any ());

  GST_DEBUG_OBJECT (quicsink, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (caps));
  return caps;
}

/* fixate sink caps during pull-mode negotiation */
// static GstCaps *
// gst_quicsink_fixate (GstBaseSink * sink, GstCaps * caps)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "fixate");

//   return NULL;
// }

// /* start or stop a pulling thread */
// static gboolean
// gst_quicsink_activate_pull (GstBaseSink * sink, gboolean active)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "activate_pull");

//   return TRUE;
// }

// /* get the start and end times for syncing on this buffer */
// static void
// gst_quicsink_get_times (GstBaseSink * sink, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "get_times");

// }

// /* propose allocation parameters for upstream */
// static gboolean
// gst_quicsink_propose_allocation (GstBaseSink * sink, GstQuery * query)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "propose_allocation");

//   return TRUE;
// }

/* start and stop processing, ideal for opening/closing the resource */
static gboolean
gst_quicsink_start (GstBaseSink * sink)
{
  socklen_t socklen;
  struct lsquic_engine_api engine_api;
  struct lsquic_engine_settings engine_settings;
  server_addr_u server_addr;

  GstQuicsink *quicsink = GST_QUICSINK (sink);

  GST_DEBUG_OBJECT (quicsink, "start");

  if (0 != lsquic_global_init(LSQUIC_GLOBAL_SERVER))
  {
    GST_ELEMENT_ERROR (quicsink, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  /* Initialize logging */
  FILE *s_log_fh = stderr;

  if (0 != lsquic_set_log_level("debug"))
  {
    GST_ELEMENT_ERROR (quicsink, LIBRARY, INIT,
        (NULL),
        ("Failed to initialise lsquic"));
    return FALSE;
  }

  setvbuf(s_log_fh, NULL, _IOLBF, 0);
  lsquic_logger_init(&logger_if, s_log_fh, LLTS_HHMMSSUS);

  // Initialize engine settings to default values
  lsquic_engine_init_settings(&engine_settings, QUIC_SERVER);

  // Parse IP address and set port number
  if (!gst_quic_set_addr(quicsink->host, quicsink->port, &server_addr))
  {
      GST_ELEMENT_ERROR (quicsink, RESOURCE, OPEN_READ, (NULL),
        ("Failed to resolve host: %s", quicsink->host));
      return FALSE;
  }

  gst_quicsink_load_cert_and_key(quicsink);

  return TRUE;
}

static gboolean
gst_quicsink_stop (GstBaseSink * sink)
{
  GstQuicsink *quicsink = GST_QUICSINK (sink);

  GST_DEBUG_OBJECT (quicsink, "stop");

  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_quicsink_unlock (GstBaseSink * sink)
{
  GstQuicsink *quicsink = GST_QUICSINK (sink);

  GST_DEBUG_OBJECT (quicsink, "unlock");

  return TRUE;
}

/* Clear a previously indicated unlock request not that unlocking is
 * complete. Sub-classes should clear any command queue or indicator they
 * set during unlock */
static gboolean
gst_quicsink_unlock_stop (GstBaseSink * sink)
{
  GstQuicsink *quicsink = GST_QUICSINK (sink);

  GST_DEBUG_OBJECT (quicsink, "unlock_stop");

  return TRUE;
}

// /* notify subclass of query */
// static gboolean
// gst_quicsink_query (GstBaseSink * sink, GstQuery * query)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "query");

//   return TRUE;
// }

// /* notify subclass of event */
// static gboolean
// gst_quicsink_event (GstBaseSink * sink, GstEvent * event)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "event");

//   return TRUE;
// }

// /* wait for eos or gap, subclasses should chain up to parent first */
// static GstFlowReturn
// gst_quicsink_wait_event (GstBaseSink * sink, GstEvent * event)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "wait_event");

//   return GST_FLOW_OK;
// }

// /* notify subclass of buffer or list before doing sync */
// static GstFlowReturn
// gst_quicsink_prepare (GstBaseSink * sink, GstBuffer * buffer)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "prepare");

//   return GST_FLOW_OK;
// }

// static GstFlowReturn
// gst_quicsink_prepare_list (GstBaseSink * sink, GstBufferList * buffer_list)
// {
//   GstQuicsink *quicsink = GST_QUICSINK (sink);

//   GST_DEBUG_OBJECT (quicsink, "prepare_list");

//   return GST_FLOW_OK;
// }

/* notify subclass of preroll buffer or real buffer */
static GstFlowReturn
gst_quicsink_preroll (GstBaseSink * sink, GstBuffer * buffer)
{
  GstQuicsink *quicsink = GST_QUICSINK (sink);

  GST_DEBUG_OBJECT (quicsink, "preroll");

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_quicsink_render (GstBaseSink * sink, GstBuffer * buffer)
{
  GstQuicsink *quicsink = GST_QUICSINK (sink);

  GST_DEBUG_OBJECT (quicsink, "render");

  return GST_FLOW_OK;
}

/* Render a BufferList */
static GstFlowReturn
gst_quicsink_render_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  GstQuicsink *quicsink = GST_QUICSINK (sink);

  GST_DEBUG_OBJECT (quicsink, "render_list");

  return GST_FLOW_OK;
}
