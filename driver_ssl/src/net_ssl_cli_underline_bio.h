#ifndef NET_SSL_CLI_BIO_I_H_INCLEDED
#define NET_SSL_CLI_BIO_I_H_INCLEDED
#include "net_ssl_cli_endpoint_i.h"

int net_ssl_cli_underline_bio_new(BIO *b);
int net_ssl_cli_underline_bio_free(BIO *b);
int net_ssl_cli_underline_bio_read(BIO *b, char *out, int outlen);
int net_ssl_cli_underline_bio_write(BIO *b, const char *in, int inlen);
long net_ssl_cli_underline_bio_ctrl(BIO *b, int cmd, long num, void *ptr);
int net_ssl_cli_underline_bio_puts(BIO *b, const char *s);

#endif
