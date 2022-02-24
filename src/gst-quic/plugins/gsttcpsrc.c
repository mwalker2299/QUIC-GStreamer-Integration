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
 * SECTION:element-gsttcpsrc
 *
 * The tcpsrc element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! tcpsrc ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gsttcpsrc.h"
#include "gstquicutils.h"

#define TCP_DEFAULT_PORT 5000
#define TCP_DEFAULT_HOST "127.0.0.1"
#define TCP_DEFAULT_READ_SIZE 10000

GST_DEBUG_CATEGORY_STATIC (gst_tcpsrc_debug_category);
#define GST_CAT_DEFAULT gst_tcpsrc_debug_category

/* pad templates */

static GstStaticPadTemplate gst_tcpsrc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* property templates */

enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT,
  PROP_CAPS
};


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstTcpsrc, gst_tcpsrc, GST_TYPE_PUSH_SRC,
    GST_DEBUG_CATEGORY_INIT (gst_tcpsrc_debug_category, "tcpsrc", 0,
        "debug category for tcpsrc element"));

/* prototypes */

static void gst_tcpsrc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_tcpsrc_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static void gst_tcpsrc_dispose (GObject * object);
static void gst_tcpsrc_finalize (GObject * object);

static GstCaps *gst_tcpsrc_get_caps (GstBaseSrc * src, GstCaps * filter);

static gboolean gst_tcpsrc_start (GstBaseSrc * src);
static gboolean gst_tcpsrc_stop (GstBaseSrc * src);

static gboolean gst_tcpsrc_unlock (GstBaseSrc * src);
static gboolean gst_tcpsrc_unlock_stop (GstBaseSrc * src);
static gboolean gst_tcpsrc_event (GstBaseSrc * src, GstEvent * event);

static GstFlowReturn gst_tcpsrc_create (GstPushSrc * src, GstBuffer ** outbuf);

static void
gst_tcpsrc_class_init (GstTcpsrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *push_src_class = (GstPushSrcClass *) klass;

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_tcpsrc_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "TCP packet receiver", "Source/Network", "Receive packets over the network via TCP",
      "Matthew Walker <mjwalker2299@gmail.com>");

  gobject_class->set_property = gst_tcpsrc_set_property;
  gobject_class->get_property = gst_tcpsrc_get_property;
  gobject_class->dispose = gst_tcpsrc_dispose;
  gobject_class->finalize = gst_tcpsrc_finalize;

  g_object_class_install_property (gobject_class, PROP_HOST,
      g_param_spec_string ("host", "Host",
          "The server IP address to connect to", TCP_DEFAULT_HOST,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "Port", "The port used by the TCP server", 0,
          G_MAXUINT16, TCP_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CAPS,
      g_param_spec_boxed ("caps", "Caps",
          "The caps of the source pad", GST_TYPE_CAPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_tcpsrc_get_caps);

  base_src_class->start = GST_DEBUG_FUNCPTR (gst_tcpsrc_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_tcpsrc_stop);

  base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_tcpsrc_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_tcpsrc_unlock_stop);
  base_src_class->event = GST_DEBUG_FUNCPTR (gst_tcpsrc_event);

  push_src_class->create = GST_DEBUG_FUNCPTR (gst_tcpsrc_create);
}


static void
gst_tcpsrc_init (GstTcpsrc * tcpsrc)
{
  tcpsrc->port = TCP_DEFAULT_PORT;
  tcpsrc->host = g_strdup (TCP_DEFAULT_HOST);

  tcpsrc->socket = -1;
  tcpsrc->connection_active = FALSE;

  /* configure basesrc to be a live source */
  gst_base_src_set_live (GST_BASE_SRC (tcpsrc), TRUE);
  /* make basesrc output a segment in time */
  gst_base_src_set_format (GST_BASE_SRC (tcpsrc), GST_FORMAT_TIME);
  /* make basesrc set timestamps on outgoing buffers based on the running_time
   * when they were captured */
  gst_base_src_set_do_timestamp (GST_BASE_SRC (tcpsrc), TRUE);
}

static void
gst_tcpsrc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (object);

  GST_DEBUG_OBJECT (tcpsrc, "set_property");

  switch (property_id) {
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (tcpsrc->host);
      tcpsrc->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_PORT:
      tcpsrc->port = g_value_get_int (value);
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

      GST_OBJECT_LOCK (tcpsrc);
      old_caps = tcpsrc->caps;
      tcpsrc->caps = new_caps;
      GST_OBJECT_UNLOCK (tcpsrc);
      if (old_caps)
        gst_caps_unref (old_caps);

      gst_pad_mark_reconfigure (GST_BASE_SRC_PAD (tcpsrc));
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_tcpsrc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (object);
  GST_DEBUG_OBJECT (tcpsrc, "get_property");

  switch (property_id) {
    case PROP_HOST:
      g_value_set_string (value, tcpsrc->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, tcpsrc->port);
      break;
    case PROP_CAPS:
      GST_OBJECT_LOCK (tcpsrc);
      gst_value_set_caps (value, tcpsrc->caps);
      GST_OBJECT_UNLOCK (tcpsrc);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_tcpsrc_dispose (GObject * object)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (object);

  GST_DEBUG_OBJECT (tcpsrc, "dispose");

  G_OBJECT_CLASS (gst_tcpsrc_parent_class)->dispose (object);
}

void
gst_tcpsrc_finalize (GObject * object)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (object);

  /* clean up object here */

  G_OBJECT_CLASS (gst_tcpsrc_parent_class)->finalize (object);
}

/* Provides the basesrc with possible pad capabilities
   to allow it to respond to queries from downstream elements.
   Since the data transmitted over TCP could be of any type, we 
   return either ANY caps or, if provided, the caps of the
   downstream element */
static GstCaps *
gst_tcpsrc_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstTcpsrc *tcpsrc;
  GstCaps *caps, *result;

  tcpsrc = GST_TCPSRC (src);

  GST_OBJECT_LOCK (src);
  if ((caps = tcpsrc->caps))
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

  GST_DEBUG_OBJECT (tcpsrc, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (result));
  return result;
}

/* Open a connection with the TCP server */
static gboolean
gst_tcpsrc_start (GstBaseSrc * src)
{
  socklen_t server_addrlen;
  server_addr_u server_addr;

  GstTcpsrc *tcpsrc = GST_TCPSRC (src);

  GST_DEBUG_OBJECT (tcpsrc, "start");
  GST_DEBUG_OBJECT (tcpsrc, "Host is: %s, port is: %d", tcpsrc->host, tcpsrc->port);


  // Parse IP address and set port number
  if (!gst_quic_set_addr(tcpsrc->host, tcpsrc->port, &server_addr))
  {
      GST_ELEMENT_ERROR (tcpsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to resolve host: %s", tcpsrc->host));
      return FALSE;
  }

  // Create socket
  tcpsrc->socket = socket(server_addr.sa.sa_family, SOCK_STREAM, 0);

  if (tcpsrc->socket < 0)
  {
    GST_ELEMENT_ERROR (tcpsrc, RESOURCE, OPEN_READ, (NULL),
      ("Failed to open socket: %d", errno));
    return FALSE;
  }

  int activate = 1;
  if (0 != setsockopt(tcpsrc->socket, IPPROTO_TCP, TCP_NODELAY, &activate, sizeof(activate))) {
    GST_ELEMENT_ERROR (tcpsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add enable TCP_NODELAY using setsockopt"));
    return FALSE;
  }
  if (0 != setsockopt(tcpsrc->socket, IPPROTO_TCP, TCP_QUICKACK, &activate, sizeof(activate))) {
    GST_ELEMENT_ERROR (tcpsrc, RESOURCE, OPEN_READ, (NULL),
        ("Failed to add enable TCP_QUICKACK using setsockopt"));
    return FALSE;
  }

  server_addrlen = sizeof(server_addr);

  // Connect to server
  if (connect(tcpsrc->socket, &server_addr.sa, server_addrlen) == -1) {
    GST_ELEMENT_ERROR (tcpsrc, RESOURCE, OPEN_READ, (NULL),
      ("Failed to create connection: %d", errno));
    return FALSE;
  } else {
    tcpsrc->connection_active = TRUE;
  }

  GST_DEBUG_OBJECT(tcpsrc, "Connection established");

  return TRUE;
}

static gboolean
gst_tcpsrc_stop (GstBaseSrc * src)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (src);

  close(tcpsrc->socket);

  return TRUE;
}

// /* given a buffer, return start and stop time when it should be pushed
//  * out. The base class will sync on the clock using these times. */
// static void
// gst_tcpsrc_get_times (GstBaseSrc * src, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end)
// {
//   GstTcpsrc *tcpsrc = GST_TCPSRC (src);

//   GST_DEBUG_OBJECT (tcpsrc, "get_times");

// }

// /* get the total size of the resource in bytes */
// static gboolean
// gst_tcpsrc_get_size (GstBaseSrc * src, guint64 * size)
// {
//   GstTcpsrc *tcpsrc = GST_TCPSRC (src);

//   GST_DEBUG_OBJECT (tcpsrc, "get_size");

//   return TRUE;
// }

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_tcpsrc_unlock (GstBaseSrc * src)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (src);

  GST_DEBUG_OBJECT (tcpsrc, "unlock");

  return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean
gst_tcpsrc_unlock_stop (GstBaseSrc * src)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (src);

  GST_DEBUG_OBJECT (tcpsrc, "unlock_stop");

  return TRUE;
}

/* notify subclasses of an event */
static gboolean
gst_tcpsrc_event (GstBaseSrc * src, GstEvent * event)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (src);

  GST_DEBUG_OBJECT (tcpsrc, "event");

  return TRUE;
}

/* Ask the subclass to fill the buffer with data. */
static GstFlowReturn
gst_tcpsrc_create (GstPushSrc * src, GstBuffer ** outbuf)
{
  GstTcpsrc *tcpsrc = GST_TCPSRC (src);
  GstMapInfo map;
  gboolean new_data = FALSE;
  gint data_availiable = 0;
  gsize bytes_read = 0;

  if (!tcpsrc->connection_active) {
    return GST_FLOW_EOS;
  }

  // Check to see how much data is availiable
  if (ioctl(tcpsrc->socket, FIONREAD, &data_availiable) == -1)
  {
    GST_ELEMENT_ERROR (tcpsrc, RESOURCE, OPEN_READ, (NULL),
        ("Error when calling ioctl: %d", errno));
    return FALSE;
  }

  // If there is no data yet availiable, then we still call recv as it will block
  // until there is some data to write to a buffer. We use 10000 bytes as the default value
  if (data_availiable == 0) 
  {
    data_availiable = TCP_DEFAULT_READ_SIZE;
  }
    *outbuf = gst_buffer_new_and_alloc(data_availiable);

  gst_buffer_map (*outbuf, &map, GST_MAP_READWRITE);
  bytes_read = recv(tcpsrc->socket, map.data, data_availiable, 0);
  if (bytes_read == -1) {
    GST_ELEMENT_ERROR (tcpsrc, RESOURCE, OPEN_READ, (NULL),
        ("Error when reading: %d", errno));
    return FALSE;
  }

  gst_buffer_unmap(*outbuf, &map);
  gst_buffer_resize(*outbuf, 0, bytes_read);

  // Connection closed by server, send EOS to pipeline
  if (bytes_read == 0)
  {
    return GST_FLOW_EOS;
  }


  GST_DEBUG_OBJECT (tcpsrc, "Read %lu bytes from socket. Pushing buffer of size %lu", bytes_read, bytes_read);

  return GST_FLOW_OK;
}
