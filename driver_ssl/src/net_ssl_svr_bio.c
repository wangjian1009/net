#include "cpe/pal/pal_string.h"
#include "net_ssl_svr_bio.h"

int net_ssl_svr_endpoint_bio_new(BIO *b) {
	BIO_set_init(b, 0);
	BIO_set_data(b, NULL);
	return 1;
}


int net_ssl_svr_endpoint_bio_free(BIO *b) {
	/* if (!b) */
	/* 	return 0; */
	/* if (BIO_get_shutdown(b)) { */
	/* 	if (BIO_get_init(b) && BIO_get_data(b)) */
	/* 		bufferevent_free(BIO_get_data(b)); */
	/* 	BIO_free(b); */
	/* } */
	return 1;
}

int net_ssl_svr_endpoint_bio_read(BIO *b, char *out, int outlen) {
	/* int r = 0; */
	/* struct evbuffer *input; */

	/* BIO_clear_retry_flags(b); */

	/* if (!out) */
	/* 	return 0; */
	/* if (!BIO_get_data(b)) */
	/* 	return -1; */

	/* input = bufferevent_get_input(BIO_get_data(b)); */
	/* if (evbuffer_get_length(input) == 0) { */
	/* 	/\* If there's no data to read, say so. *\/ */
	/* 	BIO_set_retry_read(b); */
	/* 	return -1; */
	/* } else { */
	/* 	r = evbuffer_remove(input, out, outlen); */
	/* } */

	/* return r; */
    return 0;
}

int net_ssl_svr_endpoint_bio_write(BIO *b, const char *in, int inlen) {
	/* struct bufferevent *bufev = BIO_get_data(b); */
	/* struct evbuffer *output; */
	/* size_t outlen; */

	/* BIO_clear_retry_flags(b); */

	/* if (!BIO_get_data(b)) */
	/* 	return -1; */

	/* output = bufferevent_get_output(bufev); */
	/* outlen = evbuffer_get_length(output); */

	/* /\* Copy only as much data onto the output buffer as can fit under the */
	/*  * high-water mark. *\/ */
	/* if (bufev->wm_write.high && bufev->wm_write.high <= (outlen+inlen)) { */
	/* 	if (bufev->wm_write.high <= outlen) { */
	/* 		/\* If no data can fit, we'll need to retry later. *\/ */
	/* 		BIO_set_retry_write(b); */
	/* 		return -1; */
	/* 	} */
	/* 	inlen = bufev->wm_write.high - outlen; */
	/* } */

	/* EVUTIL_ASSERT(inlen > 0); */
	/* evbuffer_add(output, in, inlen); */
	/* return inlen; */
    return 0;
}

/* Called to handle various requests */
long net_ssl_svr_endpoint_bio_ctrl(BIO *b, int cmd, long num, void *ptr) {
	/* struct bufferevent *bufev = BIO_get_data(b); */
	/* long ret = 1; */

	/* switch (cmd) { */
	/* case BIO_CTRL_GET_CLOSE: */
	/* 	ret = BIO_get_shutdown(b); */
	/* 	break; */
	/* case BIO_CTRL_SET_CLOSE: */
	/* 	BIO_set_shutdown(b, (int)num); */
	/* 	break; */
	/* case BIO_CTRL_PENDING: */
	/* 	ret = evbuffer_get_length(bufferevent_get_input(bufev)) != 0; */
	/* 	break; */
	/* case BIO_CTRL_WPENDING: */
	/* 	ret = evbuffer_get_length(bufferevent_get_output(bufev)) != 0; */
	/* 	break; */
	/* /\* XXXX These two are given a special-case treatment because */
	/*  * of cargo-cultism.  I should come up with a better reason. *\/ */
	/* case BIO_CTRL_DUP: */
	/* case BIO_CTRL_FLUSH: */
	/* 	ret = 1; */
	/* 	break; */
	/* default: */
	/* 	ret = 0; */
	/* 	break; */
	/* } */
	/* return ret; */
    return 0;
}

int net_ssl_svr_endpoint_bio_puts(BIO *b, const char *s) {
	return net_ssl_svr_endpoint_bio_write(b, s, strlen(s));
}
