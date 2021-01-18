#include "cpe/pal/pal_string.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_ssl_svr_driver.h"
#include "net_ssl_svr_underline_i.h"
#include "net_ssl_svr_underline_bio.h"

int net_ssl_svr_underline_bio_new(BIO *b) {
	BIO_set_init(b, 0);
	BIO_set_data(b, NULL);
	return 1;
}

int net_ssl_svr_underline_bio_free(BIO *b) {
    net_endpoint_t base_underline = BIO_get_data(b);
    if (base_underline == NULL) return 0;

    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;
    
    if (net_endpoint_driver_debug(base_underline) >= 2) {
        CPE_INFO(
            driver->m_em, "net: ssl: %s: svr: bio: free success",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline));
    }
    
	return 1;
}

int net_ssl_svr_underline_bio_read(BIO *b, char *out, int outlen) {
	BIO_clear_retry_flags(b);

	if (!out) return 0;

    net_endpoint_t base_underline = BIO_get_data(b);
    if (base_underline == NULL) return -1;

    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;

    uint32_t length = net_endpoint_buf_size(base_underline, net_ep_buf_read);
    if (length == 0) {
		/* If there's no data to read, say so. */
		BIO_set_retry_read(b);
		return -1;
	}

    if (length > outlen) {
        if (net_endpoint_driver_debug(base_underline) >= 2) {
            CPE_INFO(
                driver->m_em, "net: ssl: %s: svr: bio: read: out-len=%d, buf-len=%d, read part",
                net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline),
                outlen, length);
        }

        length = outlen;
    }
    
    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(base_underline, net_ep_buf_read, length, &data) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: svr: bio: read: peak data fail, length=%d",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline),
            length);
        return -1;
    }

    memcpy(out, data, length);

    net_endpoint_buf_consume(base_underline, net_ep_buf_read, length);

    return length;
}

int net_ssl_svr_underline_bio_write(BIO *b, const char *in, int inlen) {
	BIO_clear_retry_flags(b);

    net_endpoint_t base_underline = BIO_get_data(b);
    if (base_underline == NULL) return -1;

    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;

    uint32_t write_size = inlen;
    if (net_endpoint_buf_append(base_underline, net_ep_buf_write, in, write_size) != 0) {
        CPE_ERROR(
            driver->m_em, "net: ssl: %s: svr: bio: write: append buf fail, len=%d!",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline), write_size);
        return -1;
    }

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

    return write_size;
}

/* Called to handle various requests */
long net_ssl_svr_underline_bio_ctrl(BIO *b, int cmd, long num, void *ptr) {
    net_endpoint_t base_underline = BIO_get_data(b);
    if (base_underline == NULL) return -1;

    net_ssl_svr_underline_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_underline));
    net_ssl_svr_driver_t driver = protocol->m_driver;
    
    if (net_endpoint_driver_debug(base_underline) >= 2) {
        CPE_INFO(
            driver->m_em, "net: ssl: %s: svr: bio: cmd %s",
            net_endpoint_dump(net_ssl_svr_driver_tmp_buffer(driver), base_underline),
            net_ssl_bio_ctrl_cmd_str(cmd));
    }
    
	long ret = 1;

	switch (cmd) {
	case BIO_CTRL_GET_CLOSE:
		ret = BIO_get_shutdown(b);
		break;
	case BIO_CTRL_SET_CLOSE:
		BIO_set_shutdown(b, (int)num);
		break;
	case BIO_CTRL_PENDING:
        ret = net_endpoint_buf_size(base_underline, net_ep_buf_read) != 0;
		break;
	case BIO_CTRL_WPENDING:
        ret = net_endpoint_buf_size(base_underline, net_ep_buf_write) != 0;
		break;
	/* XXXX These two are given a special-case treatment because
	 * of cargo-cultism.  I should come up with a better reason. */
	case BIO_CTRL_DUP:
	case BIO_CTRL_FLUSH:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

int net_ssl_svr_underline_bio_puts(BIO *b, const char *s) {
	return net_ssl_svr_underline_bio_write(b, s, strlen(s));
}
