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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_QUICSINKSS_H_
#define _GST_QUICSINKSS_H_

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "gstquicutils.h"

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

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>

#include <lsquic.h>

G_BEGIN_DECLS

#define GST_TYPE_QUICSINKSS   (gst_quicsinkss_get_type())
#define GST_QUICSINKSS(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_QUICSINKSS,GstQuicsinkss))
#define GST_QUICSINKSS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_QUICSINKSS,GstQuicsinkssClass))
#define GST_IS_QUICSINKSS(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_QUICSINKSS))
#define GST_IS_QUICSINKSS_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_QUICSINKSS))

typedef struct _GstQuicsinkss GstQuicsinkss;
typedef struct _GstQuicsinkssClass GstQuicsinkssClass;

struct _GstQuicsinkss
{
  GstBaseSink parent;

  gint socket;

  /* SSL context */
  SSL_CTX *ssl_ctx;

  /* SSL key and cert path */
  gchar *cert_file;
  gchar *key_file;

  /* LSQUIC logging path */
  gchar *log_file;

  /* QUIC server address info */
  guint16 port;
  gchar *host;

  /* Local address storage */
  struct sockaddr_storage local_address;

  /* QUIC Components */
  gboolean connection_active;
  lsquic_engine_t *engine;
  lsquic_conn_t *connection;
  lsquic_stream_t *stream;

  /* stream context */
  struct server_stream_ctx stream_ctx;
};

struct _GstQuicsinkssClass
{
  GstBaseSinkClass parent_class;
};

GType gst_quicsinkss_get_type (void);

G_END_DECLS

#endif
