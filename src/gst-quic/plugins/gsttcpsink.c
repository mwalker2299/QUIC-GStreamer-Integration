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
 * SECTION:element-gsttcpsink
 *
 * The tcpsink element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! tcpsink ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "gsttcpsink.h"
#include "gstquicutils.h"

#define TCP_DEFAULT_PORT 5000
#define TCP_DEFAULT_HOST "127.0.0.1"

GST_DEBUG_CATEGORY_STATIC (gst_tcpsink_debug_category);
#define GST_CAT_DEFAULT gst_tcpsink_debug_category

/* gstreamer method prototypes */

static void gst_tcpsink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_tcpsink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_tcpsink_dispose (GObject * object);
static void gst_tcpsink_finalize (GObject * object);

static GstCaps *gst_tcpsink_get_caps (GstBaseSink * sink, GstCaps * filter);
// static gboolean gst_tcpsink_set_caps (GstBaseSink * sink, GstCaps * caps);
// static GstCaps *gst_tcpsink_fixate (GstBaseSink * sink, GstCaps * caps);
// static gboolean gst_tcpsink_activate_pull (GstBaseSink * sink,
//     gboolean active);
// static void gst_tcpsink_get_times (GstBaseSink * sink, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end);
// static gboolean gst_tcpsink_propose_allocation (GstBaseSink * sink,
//     GstQuery * query);
static gboolean gst_tcpsink_start (GstBaseSink * sink);
static gboolean gst_tcpsink_stop (GstBaseSink * sink);
static gboolean gst_tcpsink_unlock (GstBaseSink * sink);
static gboolean gst_tcpsink_unlock_stop (GstBaseSink * sink);
// static gboolean gst_tcpsink_query (GstBaseSink * sink, GstQuery * query);
// static gboolean gst_tcpsink_event (GstBaseSink * sink, GstEvent * event);
// static GstFlowReturn gst_tcpsink_wait_event (GstBaseSink * sink,
//     GstEvent * event);
// static GstFlowReturn gst_tcpsink_prepare (GstBaseSink * sink,
//     GstBuffer * buffer);
// static GstFlowReturn gst_tcpsink_prepare_list (GstBaseSink * sink,
//     GstBufferList * buffer_list);
static GstFlowReturn gst_tcpsink_preroll (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_tcpsink_render (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_tcpsink_render_list (GstBaseSink * sink,
    GstBufferList * buffer_list);


enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT,
};

/* pad templates */

static GstStaticPadTemplate gst_tcpsink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstTcpsink, gst_tcpsink, GST_TYPE_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT (gst_tcpsink_debug_category, "tcpsink", 0,
        "debug category for tcpsink element"));

static void
gst_tcpsink_class_init (GstTcpsinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_tcpsink_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "TCP packet sender", "Sink/Network", "Send packets over the network via TCP",
      "Matthew Walker <mjwalker2299@gmail.com>");

  gobject_class->set_property = gst_tcpsink_set_property;
  gobject_class->get_property = gst_tcpsink_get_property;
  gobject_class->dispose = gst_tcpsink_dispose;
  gobject_class->finalize = gst_tcpsink_finalize;

  g_object_class_install_property (gobject_class, PROP_HOST,
      g_param_spec_string ("host", "Host",
          "The IP address on which the server receives connections", TCP_DEFAULT_HOST,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "Port", "The port used by the tcp server", 0,
          G_MAXUINT16, TCP_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_sink_class->get_caps = GST_DEBUG_FUNCPTR (gst_tcpsink_get_caps);
  // base_sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_tcpsink_set_caps);
  // base_sink_class->fixate = GST_DEBUG_FUNCPTR (gst_tcpsink_fixate);
  // base_sink_class->activate_pull =
  //     GST_DEBUG_FUNCPTR (gst_tcpsink_activate_pull);
  // base_sink_class->get_times = GST_DEBUG_FUNCPTR (gst_tcpsink_get_times);
  // base_sink_class->propose_allocation =
  //     GST_DEBUG_FUNCPTR (gst_tcpsink_propose_allocation);
  base_sink_class->start = GST_DEBUG_FUNCPTR (gst_tcpsink_start);
  base_sink_class->stop = GST_DEBUG_FUNCPTR (gst_tcpsink_stop);
  base_sink_class->unlock = GST_DEBUG_FUNCPTR (gst_tcpsink_unlock);
  base_sink_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_tcpsink_unlock_stop);
  // base_sink_class->query = GST_DEBUG_FUNCPTR (gst_tcpsink_query);
  // base_sink_class->event = GST_DEBUG_FUNCPTR (gst_tcpsink_event);
  // base_sink_class->wait_event = GST_DEBUG_FUNCPTR (gst_tcpsink_wait_event);
  // base_sink_class->prepare = GST_DEBUG_FUNCPTR (gst_tcpsink_prepare);
  // base_sink_class->prepare_list = GST_DEBUG_FUNCPTR (gst_tcpsink_prepare_list);
  base_sink_class->preroll = GST_DEBUG_FUNCPTR (gst_tcpsink_preroll);
  base_sink_class->render = GST_DEBUG_FUNCPTR (gst_tcpsink_render);
  base_sink_class->render_list = GST_DEBUG_FUNCPTR (gst_tcpsink_render_list);

}

static void
gst_tcpsink_init (GstTcpsink * tcpsink)
{
  tcpsink->port = TCP_DEFAULT_PORT;
  tcpsink->host = g_strdup (TCP_DEFAULT_HOST);

  tcpsink->socket = -1;
  tcpsink->connectionfd = -1;
  tcpsink->connection_active = FALSE;
}

void
gst_tcpsink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTcpsink *tcpsink = GST_TCPSINK (object);

  GST_DEBUG_OBJECT (tcpsink, "set_property");

  switch (property_id) {
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (tcpsink->host);
      tcpsink->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_PORT:
      tcpsink->port = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_tcpsink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstTcpsink *tcpsink = GST_TCPSINK (object);
  GST_DEBUG_OBJECT (tcpsink, "get_property");

  switch (property_id) {
    case PROP_HOST:
      g_value_set_string (value, tcpsink->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, tcpsink->port);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_tcpsink_dispose (GObject * object)
{
  GstTcpsink *tcpsink = GST_TCPSINK (object);

  GST_DEBUG_OBJECT (tcpsink, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_tcpsink_parent_class)->dispose (object);
}

void
gst_tcpsink_finalize (GObject * object)
{
  GstTcpsink *tcpsink = GST_TCPSINK (object);

  GST_DEBUG_OBJECT (tcpsink, "finalize");

  close(tcpsink->socket);

  G_OBJECT_CLASS (gst_tcpsink_parent_class)->finalize (object);
}

/* Provides the basesink with possible pad capabilities
   to allow it to respond to queries from upstream elements.
   Since the data transmitted over TCP could be of any type, we 
   return either ANY caps or, if provided, the caps of the
   upstream element */
static GstCaps *
gst_tcpsink_get_caps (GstBaseSink * sink, GstCaps * filter)
{
  GstTcpsink *tcpsink;
  GstCaps *caps = NULL;

  tcpsink = GST_TCPSINK (sink);

  caps = (filter ? gst_caps_ref (filter) : gst_caps_new_any ());

  GST_DEBUG_OBJECT (tcpsink, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (caps));
  return caps;
}

/* fixate sink caps during pull-mode negotiation */
// static GstCaps *
// gst_tcpsink_fixate (GstBaseSink * sink, GstCaps * caps)
// {
//   GstTcpsink *tcpsink = GST_TCPSINK (sink);

//   GST_DEBUG_OBJECT (tcpsink, "fixate");

//   return NULL;
// }

// /* start or stop a pulling thread */
// static gboolean
// gst_tcpsink_activate_pull (GstBaseSink * sink, gboolean active)
// {
//   GstTcpsink *tcpsink = GST_TCPSINK (sink);

//   GST_DEBUG_OBJECT (tcpsink, "activate_pull");

//   return TRUE;
// }

// /* get the start and end times for syncing on this buffer */
// static void
// gst_tcpsink_get_times (GstBaseSink * sink, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end)
// {
//   GstTcpsink *tcpsink = GST_TCPSINK (sink);

//   GST_DEBUG_OBJECT (tcpsink, "get_times");

// }

// /* propose allocation parameters for upstream */
// static gboolean
// gst_tcpsink_propose_allocation (GstBaseSink * sink, GstQuery * query)
// {
//   GstTcpsink *tcpsink = GST_TCPSINK (sink);

//   GST_DEBUG_OBJECT (tcpsink, "propose_allocation");

//   return TRUE;
// }

/* start and stop processing, ideal for opening/closing the resource */
static gboolean
gst_tcpsink_start (GstBaseSink * sink)
{
  socklen_t server_addrlen, client_addrlen;
  server_addr_u server_addr, client_addr;

  GstTcpsink *tcpsink = GST_TCPSINK (sink);

  GST_DEBUG_OBJECT (tcpsink, "start");
  GST_DEBUG_OBJECT (tcpsink, "Host is: %s, port is: %d", tcpsink->host, tcpsink->port);

  // Parse IP address and set port number
  if (!gst_quic_set_addr(tcpsink->host, tcpsink->port, &server_addr))
  {
      GST_ELEMENT_ERROR (tcpsink, RESOURCE, OPEN_READ, (NULL),
        ("Failed to resolve host: %s", tcpsink->host));
      return FALSE;
  }

  // Create socket
  tcpsink->socket = socket(server_addr.sa.sa_family, SOCK_STREAM, 0);

  GST_DEBUG_OBJECT(tcpsink, "Socket fd = %d", tcpsink->socket);

  if (tcpsink->socket < 0)
  {
    GST_ELEMENT_ERROR (tcpsink, RESOURCE, OPEN_READ, (NULL),
      ("Failed to open socket"));
    return FALSE;
  }

  int activate = 1;
  if (0 != setsockopt(tcpsink->socket, IPPROTO_TCP, TCP_NODELAY, &activate, sizeof(activate))) {
    GST_ELEMENT_ERROR (tcpsink, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add enable TCP_NODELAY using setsockopt"));
    return FALSE;
  }
  if (0 != setsockopt(tcpsink->socket, IPPROTO_TCP, TCP_QUICKACK, &activate, sizeof(activate))) {
    GST_ELEMENT_ERROR (tcpsink, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add enable TCP_QUICKACK using setsockopt"));
    return FALSE;
  }


  // Bind socket to address of our server and save local address in a sockaddr_storage struct.
  // sockaddr_storage is used as it is big enough to allow IPV6 addresses to be stored.
  server_addrlen = sizeof(server_addr);
  if (0 != bind(tcpsink->socket, &server_addr.sa, server_addrlen))
  {
      GST_ELEMENT_ERROR (tcpsink, RESOURCE, OPEN_READ, (NULL),
        ("Failed to bind socket: %d", errno));
      return FALSE;
  }

  if (listen(tcpsink->socket, 1) == -1) {
    GST_ELEMENT_ERROR (tcpsink, RESOURCE, OPEN_READ, (NULL),
        ("Listen returned an error: %d", errno));
      return FALSE;
  }

  GST_DEBUG_OBJECT(tcpsink, "Ready to accept connections");

  tcpsink->connectionfd = accept(tcpsink->socket, &client_addr.sa, &client_addrlen);
  if (tcpsink->connectionfd == -1) {
    GST_ELEMENT_ERROR (tcpsink, RESOURCE, OPEN_READ, (NULL),
        ("Accept returned an error: %d", errno));
      return FALSE;
  } else {
    tcpsink->connection_active = TRUE;
  }

  GST_DEBUG_OBJECT(tcpsink, "connection established");


  return TRUE;
}

static gboolean
gst_tcpsink_stop (GstBaseSink * sink)
{
  GstTcpsink *tcpsink = GST_TCPSINK (sink);

  GST_DEBUG_OBJECT (tcpsink, "stop called, closing connection");
  close(tcpsink->connectionfd);

  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_tcpsink_unlock (GstBaseSink * sink)
{
  GstTcpsink *tcpsink = GST_TCPSINK (sink);

  GST_DEBUG_OBJECT (tcpsink, "unlock");

  return TRUE;
}

/* Clear a previously indicated unlock request not that unlocking is
 * complete. Sub-classes should clear any command queue or indicator they
 * set during unlock */
static gboolean
gst_tcpsink_unlock_stop (GstBaseSink * sink)
{
  GstTcpsink *tcpsink = GST_TCPSINK (sink);

  GST_DEBUG_OBJECT (tcpsink, "unlock_stop");

  return TRUE;
}

/* notify subclass of preroll buffer or real buffer */
static GstFlowReturn
gst_tcpsink_preroll (GstBaseSink * sink, GstBuffer * buffer)
{
  GstTcpsink *tcpsink = GST_TCPSINK (sink);

  GST_DEBUG_OBJECT (tcpsink, "preroll");

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_tcpsink_render (GstBaseSink * sink, GstBuffer * buffer)
{
  GstTcpsink *tcpsink = GST_TCPSINK (sink);
  gsize bytes_sent, buffer_size;
  GstMapInfo map;


  if (!tcpsink->connection_active) 
  {
    GST_ELEMENT_ERROR (tcpsink, RESOURCE, READ, (NULL),
        ("There is no active connection to send data to"));
    return GST_FLOW_ERROR;
  } 

  GST_DEBUG_OBJECT (tcpsink, "render -- duration: %" GST_TIME_FORMAT ",  dts: %" GST_TIME_FORMAT ",  pts: %" GST_TIME_FORMAT, GST_TIME_ARGS(buffer->duration), GST_TIME_ARGS(buffer->dts), GST_TIME_ARGS(buffer->pts));
  
  bytes_sent = 0;
  buffer_size = gst_buffer_get_size(buffer);

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  while (1) { 
    bytes_sent += send(tcpsink->connectionfd, map.data+bytes_sent, (buffer_size)-bytes_sent, MSG_NOSIGNAL);
    if (bytes_sent == -1) {
        GST_ELEMENT_ERROR (tcpsink, RESOURCE, WRITE, (NULL),
            ("Error when sending data: %d", errno));
        return GST_FLOW_ERROR;
    } else if (bytes_sent != (buffer_size)) {
      GST_DEBUG_OBJECT(tcpsink, "Unable to send full %ld bytes of data in one go, only sent %ld\n", (buffer_size), bytes_sent);
      continue;
    }
    break;
  }
   GST_DEBUG_OBJECT(tcpsink, "sent full buffer: %ld bytes", bytes_sent);

  gst_buffer_unmap(buffer, &map);

  return GST_FLOW_OK;
}

/* Render a BufferList */
static GstFlowReturn
gst_tcpsink_render_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  GstFlowReturn flow_return;
  guint i, num_buffers;

  GstTcpsink *tcpsink = GST_TCPSINK (sink);

  GST_DEBUG_OBJECT (tcpsink, "render_list");

  num_buffers = gst_buffer_list_length (buffer_list);
  if (num_buffers == 0) {
    GST_DEBUG_OBJECT (tcpsink, "Empty buffer list provided, returning immediately");
    return GST_FLOW_OK;
  }
    
  for (i = 0; i < num_buffers; i++) {

    flow_return = gst_tcpsink_render (sink, gst_buffer_list_get (buffer_list, i));

    if (flow_return != GST_FLOW_OK) {
      GST_WARNING_OBJECT(tcpsink, "flow_return was not GST_FLOW_OK, returning after sending %u/%u buffers", i+1, num_buffers);
      return flow_return;
    }

  }

  return GST_FLOW_OK;
}
