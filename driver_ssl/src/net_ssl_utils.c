#include "mbedtls/ssl.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pem.h"
#include "mbedtls/x509.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "net_protocol.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_ssl_utils.h"

/* EVP_PKEY * net_ssl_pkey_from_string(error_monitor_t em, const char * key) { */
/*     BIO * pkey_bio = BIO_new_mem_buf(key, (int)strlen(key)); */
/*     if (pkey_bio == NULL) { */
/*         CPE_ERROR( */
/*             em, "ssl: pkey from string: create bio failed: %s", */
/*             ERR_error_string(ERR_get_error(), NULL)); */
/*         return NULL; */
/*     } */

/*     EVP_PKEY * pkey = PEM_read_bio_PrivateKey(pkey_bio, NULL, NULL, NULL); */
/*     BIO_free(pkey_bio); */
/*     if (pkey == NULL) { */
/*         CPE_ERROR( */
/*             em, "ssl: read pkey failed: %s\n%s", */
/*             ERR_error_string(ERR_get_error(), NULL), key); */
/*         return NULL; */
/*     } */

/*     return pkey; */
/* } */

mbedtls_pk_context * net_ssl_pkey_generate(net_ssl_protocol_t protocol) {
    net_protocol_t base_protocol = net_protocol_from_data(protocol);
    
    mbedtls_pk_context * pk = mem_alloc(protocol->m_alloc, sizeof(mbedtls_pk_context));
    if (pk == NULL) {
        CPE_ERROR(protocol->m_em, "ssl: %s: generate pkey: alloc fail", net_protocol_name(base_protocol));
        return NULL;
    }
    
/*     BIGNUM *a = BN_new(); */
/*     if (a == NULL) { */
/*         CPE_ERROR( */
/*             em, "ssl: generate pkey: BN_new failed\n%s", */
/*             ERR_error_string(ERR_get_error(), NULL)); */
/*         return NULL; */
/*     } */
/*     BN_set_word(a, 65537); */

/*     RSA *rsa = RSA_new(); */
/*     if (rsa == NULL) { */
/*         CPE_ERROR( */
/*             em, "ssl: generate pkey: RSA_new failed\n%s", */
/*             ERR_error_string(ERR_get_error(), NULL)); */
/*         BN_free(a); */
/*         return NULL; */
/*     } */
    
/*     if (RSA_generate_key_ex(rsa, 2048, a, NULL) != 1) { */
/*         CPE_ERROR( */
/*             em, "sfox: sfox-http2: generate pkey: generate key failed\n%s", */
/*             ERR_error_string(ERR_get_error(), NULL)); */
/*         BN_free(a); */
/*         RSA_free(rsa); */
/*         return NULL; */
/*     } */
/*     BN_free(a); */

/*     EVP_PKEY* pkey = EVP_PKEY_new(); */
/*     if (pkey == NULL) { */
/*         CPE_ERROR( */
/*             em, "ssl: generate pkey: PKEY_new failed\n%s", */
/*             ERR_error_string(ERR_get_error(), NULL)); */
/*         RSA_free(rsa); */
/*         return NULL; */
/*     } */
    
/*     if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) { */
/*         CPE_ERROR( */
/*             em, "ssl: generate pkey: PKEY_assign_RSA failed\n%s", */
/*             ERR_error_string(ERR_get_error(), NULL)); */
/*         EVP_PKEY_free(pkey); */
/*         return NULL; */
/*     } */
    
    return pk;

PROCESS_ERROR:
    mem_free(protocol->m_alloc, pk);
    return NULL;
}

void net_ssl_print_cert_info(write_stream_t ws, mbedtls_x509_crt * cert) {
    char line[128];

    /* stream_printf( */
    /*     ws, "subject=[%s]", */
    /*     X509_NAME_oneline(X509_get_subject_name(cert), line, sizeof(line))); */

    /* stream_printf( */
    /*     ws, ", issuer=[%s]", */
    /*     X509_NAME_oneline(X509_get_issuer_name(cert), line, sizeof(line))); */
}

const char * net_ssl_dump_cert_info(mem_buffer_t buffer, mbedtls_x509_crt * cert) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    net_ssl_print_cert_info((write_stream_t)&stream, cert);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

/* const char * net_ssl_dump_data(mem_buffer_t buffer, void const * buf, size_t size) { */
/*     mem_buffer_clear_data(buffer); */
/*     struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer); */
/*     stream_dump_data((write_stream_t)&ws, buf, size, 1); */
/*     stream_putc((write_stream_t)&ws, 0); */
/*     return (const char *)mem_buffer_make_continuous(buffer, 0); */
/* } */

/* static const char * net_ssl_tls_rt_type(int type) { */
/*   switch(type) { */
/* #ifdef SSL3_RT_HEADER */
/*   case SSL3_RT_HEADER: */
/*     return "TLS header"; */
/* #endif */
/*   case SSL3_RT_CHANGE_CIPHER_SPEC: */
/*     return "TLS change cipher"; */
/*   case SSL3_RT_ALERT: */
/*     return "TLS alert"; */
/*   case SSL3_RT_HANDSHAKE: */
/*     return "TLS handshake"; */
/*   case SSL3_RT_APPLICATION_DATA: */
/*     return "TLS app data"; */
/*   default: */
/*     return "TLS Unknown"; */
/*   } */
/* } */

/* static const char * net_ssl_msg_type(int ssl_ver, int msg) { */
/* #ifdef SSL2_VERSION_MAJOR */
/*   if(ssl_ver == SSL2_VERSION_MAJOR) { */
/*     switch(msg) { */
/*       case SSL2_MT_ERROR: */
/*         return "Error"; */
/*       case SSL2_MT_CLIENT_HELLO: */
/*         return "Client hello"; */
/*       case SSL2_MT_CLIENT_MASTER_KEY: */
/*         return "Client key"; */
/*       case SSL2_MT_CLIENT_FINISHED: */
/*         return "Client finished"; */
/*       case SSL2_MT_SERVER_HELLO: */
/*         return "Server hello"; */
/*       case SSL2_MT_SERVER_VERIFY: */
/*         return "Server verify"; */
/*       case SSL2_MT_SERVER_FINISHED: */
/*         return "Server finished"; */
/*       case SSL2_MT_REQUEST_CERTIFICATE: */
/*         return "Request CERT"; */
/*       case SSL2_MT_CLIENT_CERTIFICATE: */
/*         return "Client CERT"; */
/*     } */
/*   } */
/*   else */
/* #endif */
/*   if(ssl_ver == SSL3_VERSION_MAJOR) { */
/*     switch(msg) { */
/*       case SSL3_MT_HELLO_REQUEST: */
/*         return "Hello request"; */
/*       case SSL3_MT_CLIENT_HELLO: */
/*         return "Client hello"; */
/*       case SSL3_MT_SERVER_HELLO: */
/*         return "Server hello"; */
/* #ifdef SSL3_MT_NEWSESSION_TICKET */
/*       case SSL3_MT_NEWSESSION_TICKET: */
/*         return "Newsession Ticket"; */
/* #endif */
/*       case SSL3_MT_CERTIFICATE: */
/*         return "Certificate"; */
/*       case SSL3_MT_SERVER_KEY_EXCHANGE: */
/*         return "Server key exchange"; */
/*       case SSL3_MT_CLIENT_KEY_EXCHANGE: */
/*         return "Client key exchange"; */
/*       case SSL3_MT_CERTIFICATE_REQUEST: */
/*         return "Request CERT"; */
/*       case SSL3_MT_SERVER_DONE: */
/*         return "Server finished"; */
/*       case SSL3_MT_CERTIFICATE_VERIFY: */
/*         return "CERT verify"; */
/*       case SSL3_MT_FINISHED: */
/*         return "Finished"; */
/* #ifdef SSL3_MT_CERTIFICATE_STATUS */
/*       case SSL3_MT_CERTIFICATE_STATUS: */
/*         return "Certificate Status"; */
/* #endif */
/* #ifdef SSL3_MT_ENCRYPTED_EXTENSIONS */
/*       case SSL3_MT_ENCRYPTED_EXTENSIONS: */
/*         return "Encrypted Extensions"; */
/* #endif */
/* #ifdef SSL3_MT_END_OF_EARLY_DATA */
/*       case SSL3_MT_END_OF_EARLY_DATA: */
/*         return "End of early data"; */
/* #endif */
/* #ifdef SSL3_MT_KEY_UPDATE */
/*       case SSL3_MT_KEY_UPDATE: */
/*         return "Key update"; */
/* #endif */
/* #ifdef SSL3_MT_NEXT_PROTO */
/*       case SSL3_MT_NEXT_PROTO: */
/*         return "Next protocol"; */
/* #endif */
/* #ifdef SSL3_MT_MESSAGE_HASH */
/*       case SSL3_MT_MESSAGE_HASH: */
/*         return "Message hash"; */
/* #endif */
/*     } */
/*   } */
/*   return "Unknown"; */
/* } */

/* void net_ssl_dump_tls_info( */
/*     error_monitor_t em, mem_buffer_t buffer, const char * prefix, uint8_t with_data, */
/*     int direction, int ssl_ver, int content_type, const void *buf, size_t len, SSL *ssl) */
/* { */
/*     if (direction != 0 && direction != 1) return; */

/*     char unknown_verstr_buf[32]; */
/*     const char * verstr = NULL; */

/*     switch (ssl_ver) { */
/* #ifdef SSL2_VERSION /\* removed in recent versions *\/ */
/*     case SSL2_VERSION: */
/*         verstr = "SSLv2"; */
/*         break; */
/* #endif */
/* #ifdef SSL3_VERSION */
/*     case SSL3_VERSION: */
/*         verstr = "SSLv3"; */
/*         break; */
/* #endif */
/*     case TLS1_VERSION: */
/*         verstr = "TLSv1.0"; */
/*         break; */
/* #ifdef TLS1_1_VERSION */
/*     case TLS1_1_VERSION: */
/*         verstr = "TLSv1.1"; */
/*         break; */
/* #endif */
/* #ifdef TLS1_2_VERSION */
/*     case TLS1_2_VERSION: */
/*         verstr = "TLSv1.2"; */
/*         break; */
/* #endif */
/* #ifdef TLS1_3_VERSION */
/*     case TLS1_3_VERSION: */
/*         verstr = "TLSv1.3"; */
/*         break; */
/* #endif */
/*     case 0: */
/*         break; */
/*     default: */
/*         snprintf(unknown_verstr_buf, sizeof(unknown_verstr_buf), "(%x)", ssl_ver); */
/*         verstr = unknown_verstr_buf; */
/*         break; */
/*     } */

/*     /\* Log progress for interesting records only (like Handshake or Alert), skip */
/*    * all raw record headers (content_type == SSL3_RT_HEADER or ssl_ver == 0). */
/*    * For TLS 1.3, skip notification of the decrypted inner Content Type. */
/*    *\/ */
/*     if (ssl_ver */
/* #ifdef SSL3_RT_INNER_CONTENT_TYPE */
/*         && content_type != SSL3_RT_INNER_CONTENT_TYPE */
/* #endif */
/*     ) { */
/*         const char *msg_name, *tls_rt_name; */
/*         char ssl_buf[1024]; */
/*         int msg_type, txt_len; */

/*         /\* the info given when the version is zero is not that useful for us *\/ */

/*         ssl_ver >>= 8; /\* check the upper 8 bits only below *\/ */

/*         /\* SSLv2 doesn't seem to have TLS record-type headers, so OpenSSL */
/*          * always pass-up content-type as 0. But the interesting message-type */
/*          * is at 'buf[0]'. */
/*          *\/ */
/*         if (ssl_ver == SSL3_VERSION_MAJOR && content_type) */
/*             tls_rt_name = net_ssl_tls_rt_type(content_type); */
/*         else */
/*             tls_rt_name = ""; */

/*         if (content_type == SSL3_RT_CHANGE_CIPHER_SPEC) { */
/*             msg_type = *(char *)buf; */
/*             msg_name = "Change cipher spec"; */
/*         } */
/*         else if (content_type == SSL3_RT_ALERT) { */
/*             msg_type = (((char *)buf)[0] << 8) + ((char *)buf)[1]; */
/*             msg_name = SSL_alert_desc_string_long(msg_type); */
/*         } */
/*         else { */
/*             msg_type = *(char *)buf; */
/*             msg_name = net_ssl_msg_type(ssl_ver, msg_type); */
/*         } */

/*         txt_len = snprintf( */
/*             ssl_buf, sizeof(ssl_buf), "%s (%s), %s, %s (%d):", */
/*             verstr, direction ? "OUT" : "IN", tls_rt_name, msg_name, msg_type); */
/*         if (0 <= txt_len && (unsigned)txt_len < sizeof(ssl_buf)) { */
/*             CPE_INFO(em, "%s%.*s", prefix, (int)txt_len, ssl_buf); */
/*         } */
/*     } */

/*     if (with_data) { */
/*         CPE_INFO( */
/*             em, "%s%s %d bytes\n%s", prefix, direction == 1 ? "==>" : "<==", (int)len, */
/*             net_ssl_dump_data(buffer, buf, len)); */
/*     } else { */
/*         CPE_INFO(em, "%s%s %d bytes", prefix, direction == 1 ? "==>" : "<==", (int)len); */
/*     } */
/* } */

/* const char * net_ssl_bio_ctrl_cmd_str(int cmd) { */
/*     static char buf[32]; */

/*     switch(cmd) { */
/*     case BIO_CTRL_RESET: */
/*         return "BIO_CTRL_RESET"; */
/*     case BIO_CTRL_EOF: */
/*         return "BIO_CTRL_EOF"; */
/*     case BIO_CTRL_INFO: */
/*         return "BIO_CTRL_INFO"; */
/*     case BIO_CTRL_SET: */
/*         return "BIO_CTRL_SET"; */
/*     case BIO_CTRL_GET: */
/*         return "BIO_CTRL_GET"; */
/*     case BIO_CTRL_PUSH: */
/*         return "BIO_CTRL_PUSH"; */
/*     case BIO_CTRL_POP: */
/*         return "BIO_CTRL_POP"; */
/*     case BIO_CTRL_GET_CLOSE: */
/*         return "BIO_CTRL_GET_CLOSE"; */
/*     case BIO_CTRL_SET_CLOSE: */
/*         return "BIO_CTRL_SET_CLOSE"; */
/*     case BIO_CTRL_PENDING: */
/*         return "BIO_CTRL_PENDING"; */
/*     case BIO_CTRL_FLUSH: */
/*         return "BIO_CTRL_FLUSH"; */
/*     case BIO_CTRL_DUP: */
/*         return "BIO_CTRL_DUP"; */
/*     case BIO_CTRL_WPENDING: */
/*         return "BIO_CTRL_WPENDING"; */
/*     case BIO_CTRL_SET_CALLBACK: */
/*         return "BIO_CTRL_SET_CALLBACK"; */
/*     case BIO_CTRL_GET_CALLBACK: */
/*         return "BIO_CTRL_GET_CALLBACK"; */
/*     case BIO_CTRL_PEEK: */
/*         return "BIO_CTRL_PEEK"; */
/*     case BIO_CTRL_SET_FILENAME: */
/*         return "BIO_CTRL_SET_FILENAME"; */
/*     default: */
/*         snprintf(buf, sizeof(buf), "unknown bio cmd %d", cmd); */
/*         return buf; */
/*     } */
/* } */
