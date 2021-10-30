#include "gstquicutils.h"

const struct lsquic_logger_if logger_if = { gst_quic_log_buf, };


gint gst_quic_log_buf (void *ctx, const char *buf, size_t len)
{
    FILE *out = ctx;
    fwrite(buf, 1, len, out);
    fflush(out);
    return 0;
}

gboolean gst_quic_set_addr(gchar* host, guint16 port, server_addr_u *server_addr) 
{
  if (inet_pton(AF_INET, host, &(server_addr->addr4.sin_addr)))
  {
      server_addr->addr4.sin_family = AF_INET;
      server_addr->addr4.sin_port   = htons(port);
      return TRUE;
  }
  else if (memset(&(server_addr->addr6), 0, sizeof(server_addr->addr6)),
      inet_pton(AF_INET6, host, &(server_addr->addr6.sin6_addr)))
  {
      server_addr->addr6.sin6_family = AF_INET6;
      server_addr->addr6.sin6_port   = htons(port);
      return TRUE;
  }
  else
  {
    return FALSE;
  }
} 
