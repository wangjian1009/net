#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_ssl_utils.h"

const char * net_ssl_dump_data(mem_buffer_t buffer, void const * buf, size_t size) {
    mem_buffer_clear_data(buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    stream_dump_data((write_stream_t)&ws, buf, size, 1);
    stream_putc((write_stream_t)&ws, 0);
    return (const char *)mem_buffer_make_continuous(buffer, 0);
}

static const char * net_ssl_tls_rt_type(int type) {
  switch(type) {
#ifdef SSL3_RT_HEADER
  case SSL3_RT_HEADER:
    return "TLS header";
#endif
  case SSL3_RT_CHANGE_CIPHER_SPEC:
    return "TLS change cipher";
  case SSL3_RT_ALERT:
    return "TLS alert";
  case SSL3_RT_HANDSHAKE:
    return "TLS handshake";
  case SSL3_RT_APPLICATION_DATA:
    return "TLS app data";
  default:
    return "TLS Unknown";
  }
}

static const char * net_ssl_msg_type(int ssl_ver, int msg) {
#ifdef SSL2_VERSION_MAJOR
  if(ssl_ver == SSL2_VERSION_MAJOR) {
    switch(msg) {
      case SSL2_MT_ERROR:
        return "Error";
      case SSL2_MT_CLIENT_HELLO:
        return "Client hello";
      case SSL2_MT_CLIENT_MASTER_KEY:
        return "Client key";
      case SSL2_MT_CLIENT_FINISHED:
        return "Client finished";
      case SSL2_MT_SERVER_HELLO:
        return "Server hello";
      case SSL2_MT_SERVER_VERIFY:
        return "Server verify";
      case SSL2_MT_SERVER_FINISHED:
        return "Server finished";
      case SSL2_MT_REQUEST_CERTIFICATE:
        return "Request CERT";
      case SSL2_MT_CLIENT_CERTIFICATE:
        return "Client CERT";
    }
  }
  else
#endif
  if(ssl_ver == SSL3_VERSION_MAJOR) {
    switch(msg) {
      case SSL3_MT_HELLO_REQUEST:
        return "Hello request";
      case SSL3_MT_CLIENT_HELLO:
        return "Client hello";
      case SSL3_MT_SERVER_HELLO:
        return "Server hello";
#ifdef SSL3_MT_NEWSESSION_TICKET
      case SSL3_MT_NEWSESSION_TICKET:
        return "Newsession Ticket";
#endif
      case SSL3_MT_CERTIFICATE:
        return "Certificate";
      case SSL3_MT_SERVER_KEY_EXCHANGE:
        return "Server key exchange";
      case SSL3_MT_CLIENT_KEY_EXCHANGE:
        return "Client key exchange";
      case SSL3_MT_CERTIFICATE_REQUEST:
        return "Request CERT";
      case SSL3_MT_SERVER_DONE:
        return "Server finished";
      case SSL3_MT_CERTIFICATE_VERIFY:
        return "CERT verify";
      case SSL3_MT_FINISHED:
        return "Finished";
#ifdef SSL3_MT_CERTIFICATE_STATUS
      case SSL3_MT_CERTIFICATE_STATUS:
        return "Certificate Status";
#endif
#ifdef SSL3_MT_ENCRYPTED_EXTENSIONS
      case SSL3_MT_ENCRYPTED_EXTENSIONS:
        return "Encrypted Extensions";
#endif
#ifdef SSL3_MT_END_OF_EARLY_DATA
      case SSL3_MT_END_OF_EARLY_DATA:
        return "End of early data";
#endif
#ifdef SSL3_MT_KEY_UPDATE
      case SSL3_MT_KEY_UPDATE:
        return "Key update";
#endif
#ifdef SSL3_MT_NEXT_PROTO
      case SSL3_MT_NEXT_PROTO:
        return "Next protocol";
#endif
#ifdef SSL3_MT_MESSAGE_HASH
      case SSL3_MT_MESSAGE_HASH:
        return "Message hash";
#endif
    }
  }
  return "Unknown";
}

void net_ssl_dump_tls_info(
    net_schedule_t schedule, const char * prefix,
    int direction, int ssl_ver, int content_type, const void *buf, size_t len, SSL *ssl)
{
    if (direction != 0 && direction != 1) return;

    char unknown_verstr_buf[32];
    const char * verstr = NULL;

    switch (ssl_ver) {
#ifdef SSL2_VERSION /* removed in recent versions */
    case SSL2_VERSION:
        verstr = "SSLv2";
        break;
#endif
#ifdef SSL3_VERSION
    case SSL3_VERSION:
        verstr = "SSLv3";
        break;
#endif
    case TLS1_VERSION:
        verstr = "TLSv1.0";
        break;
#ifdef TLS1_1_VERSION
    case TLS1_1_VERSION:
        verstr = "TLSv1.1";
        break;
#endif
#ifdef TLS1_2_VERSION
    case TLS1_2_VERSION:
        verstr = "TLSv1.2";
        break;
#endif
#ifdef TLS1_3_VERSION
    case TLS1_3_VERSION:
        verstr = "TLSv1.3";
        break;
#endif
    case 0:
        break;
    default:
        snprintf(unknown_verstr_buf, sizeof(unknown_verstr_buf), "(%x)", ssl_ver);
        verstr = unknown_verstr_buf;
        break;
    }

    /* Log progress for interesting records only (like Handshake or Alert), skip
   * all raw record headers (content_type == SSL3_RT_HEADER or ssl_ver == 0).
   * For TLS 1.3, skip notification of the decrypted inner Content Type.
   */
    if (ssl_ver
#ifdef SSL3_RT_INNER_CONTENT_TYPE
        && content_type != SSL3_RT_INNER_CONTENT_TYPE
#endif
    ) {
        const char *msg_name, *tls_rt_name;
        char ssl_buf[1024];
        int msg_type, txt_len;

        /* the info given when the version is zero is not that useful for us */

        ssl_ver >>= 8; /* check the upper 8 bits only below */

        /* SSLv2 doesn't seem to have TLS record-type headers, so OpenSSL
         * always pass-up content-type as 0. But the interesting message-type
         * is at 'buf[0]'.
         */
        if (ssl_ver == SSL3_VERSION_MAJOR && content_type)
            tls_rt_name = net_ssl_tls_rt_type(content_type);
        else
            tls_rt_name = "";

        if (content_type == SSL3_RT_CHANGE_CIPHER_SPEC) {
            msg_type = *(char *)buf;
            msg_name = "Change cipher spec";
        }
        else if (content_type == SSL3_RT_ALERT) {
            msg_type = (((char *)buf)[0] << 8) + ((char *)buf)[1];
            msg_name = SSL_alert_desc_string_long(msg_type);
        }
        else {
            msg_type = *(char *)buf;
            msg_name = net_ssl_msg_type(ssl_ver, msg_type);
        }

        txt_len = snprintf(
            ssl_buf, sizeof(ssl_buf), "%s (%s), %s, %s (%d):",
            verstr, direction ? "OUT" : "IN", tls_rt_name, msg_name, msg_type);
        if (0 <= txt_len && (unsigned)txt_len < sizeof(ssl_buf)) {
            CPE_INFO(net_schedule_em(schedule), "%s%.*s", prefix, (int)txt_len, ssl_buf);
        }
    }

    CPE_INFO(
        net_schedule_em(schedule), "%s%s %d bytes\n%s",
        prefix, direction == 1 ? "==>" : "<==",
        (int)len,
        net_ssl_dump_data(net_schedule_tmp_buffer(schedule), buf, len));
}

const char * net_ssl_errno_str(int e) {
    static char buf[32];
    
    switch(e) {
    case SSL_ERROR_NONE:
        return "SSL_ERROR_NONE";
    case SSL_ERROR_SSL:
        return "SSL_ERROR_SSL";
    case SSL_ERROR_WANT_READ:
        return "SSL_ERROR_WANT_READ";
    case SSL_ERROR_WANT_WRITE:
        return "SSL_ERROR_WANT_WRITE";
    case SSL_ERROR_WANT_X509_LOOKUP:
        return "SSL_ERROR_WANT_X509_LOOKUP";
    case SSL_ERROR_SYSCALL:
        return "SSL_ERROR_SYSCALL";
    case SSL_ERROR_ZERO_RETURN:
        return "SSL_ERROR_ZERO_RETURN";
    case SSL_ERROR_WANT_CONNECT:
        return "SSL_ERROR_WANT_CONNECT";
    case SSL_ERROR_WANT_ACCEPT:
        return "SSL_ERROR_WANT_ACCEPT";
    case SSL_ERROR_WANT_ASYNC:
        return "SSL_ERROR_WANT_ASYNC";
    case SSL_ERROR_WANT_ASYNC_JOB:
        return "SSL_ERROR_WANT_ASYNC_JOB";
    case SSL_ERROR_WANT_CLIENT_HELLO_CB:
        return "SSL_ERROR_WANT_CLIENT_HELLO_CB";
    default:
        snprintf(buf, sizeof(buf), "unknown ssl error %d", e);
        return buf;
    }
}
