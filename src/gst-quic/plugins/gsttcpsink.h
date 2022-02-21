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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_TCPSINK_H_
#define _GST_TCPSINK_H_

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

G_BEGIN_DECLS

#define GST_TYPE_TCPSINK   (gst_tcpsink_get_type())
#define GST_TCPSINK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TCPSINK,GstTcpsink))
#define GST_TCPSINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TCPSINK,GstTcpsinkClass))
#define GST_IS_TCPSINK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TCPSINK))
#define GST_IS_TCPSINK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TCPSINK))

typedef struct _GstTcpsink GstTcpsink;
typedef struct _GstTcpsinkClass GstTcpsinkClass;


struct _GstTcpsink
{
  GstBaseSink parent;

  gint socket;

  /* tcp server address info */
  guint16 port;
  gchar *host;


  /* connection details */
  gboolean connection_active;
  gint connectionfd;
};

struct _GstTcpsinkClass
{
  GstBaseSinkClass parent_class;
};

GType gst_tcpsink_get_type (void);

G_END_DECLS

#endif
