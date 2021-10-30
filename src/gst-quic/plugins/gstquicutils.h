
#include <stddef.h>
#include <stdio.h>

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

#include <lsquic.h>

#include <gst/gst.h>

typedef union server_addr {
  struct sockaddr     sa;
  struct sockaddr_in  addr4;
  struct sockaddr_in6 addr6;
} server_addr_u;

gint gst_quic_log_buf (void *ctx, const char *buf, size_t len);
gboolean gst_quic_set_addr(gchar* host, guint16 port, server_addr_u *server_addr);

const struct lsquic_logger_if logger_if;