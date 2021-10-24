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
#include <lsquic.h>

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
  PROP_0
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
static gboolean gst_quicsrc_query (GstBaseSrc * src, GstQuery * query);
static gboolean gst_quicsrc_event (GstBaseSrc * src, GstEvent * event);

static GstFlowReturn gst_quicsrc_create (GstPushSrc * src, GstBuffer ** outbuf);

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
  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_quicsrc_get_caps);

  base_src_class->start = GST_DEBUG_FUNCPTR (gst_quicsrc_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_quicsrc_stop);

  base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_quicsrc_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_quicsrc_unlock_stop);
  base_src_class->query = GST_DEBUG_FUNCPTR (gst_quicsrc_query);
  base_src_class->event = GST_DEBUG_FUNCPTR (gst_quicsrc_event);

  push_src_class->create = GST_DEBUG_FUNCPTR (gst_quicsrc_create);
}

static void
gst_quicsrc_init (GstQuicsrc * quicsrc)
{
}

void
gst_quicsrc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);

  GST_DEBUG_OBJECT (quicsrc, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_quicsrc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);

  GST_DEBUG_OBJECT (quicsrc, "get_property");

  switch (property_id) {
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

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_quicsrc_parent_class)->dispose (object);
}

void
gst_quicsrc_finalize (GObject * object)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (object);

  GST_DEBUG_OBJECT (quicsrc, "finalize");

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

  GST_DEBUG_OBJECT (src, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (caps));
  return caps;
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean
gst_quicsrc_start (GstBaseSrc * src)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "start");

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

/* notify subclasses of a query */
static gboolean
gst_quicsrc_query (GstBaseSrc * src, GstQuery * query)
{
  GstQuicsrc *quicsrc = GST_QUICSRC (src);

  GST_DEBUG_OBJECT (quicsrc, "query");

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


  GST_ELEMENT_ERROR (src, STREAM, FAILED,
        ("Internal data flow error."),
        ("Element: %s error", G_OBJECT_TYPE_NAME (quicsrc)));
    return GST_FLOW_ERROR;

  lsquic_conn_crypto_cipher(NULL);

  GST_DEBUG_OBJECT (quicsrc, "create");

  return GST_FLOW_OK;
}
