#include "cpe/pal/pal_string.h"
#include "cpe/utils/md5.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_mem.h"
#include "net_log_util.h"
#include "hmac-sha.h"

static const char *g_hex_hash = "0123456789ABCDEF";

void md5_to_string(const char * buffer, int bufLen, char * md5) {
    struct cpe_md5_ctx ctx;
    
    cpe_md5_ctx_init(&ctx);
    cpe_md5_ctx_update(&ctx, buffer, bufLen);
    cpe_md5_ctx_final(&ctx);

    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(md5, 32);
    cpe_md5_print((write_stream_t)&ws, &ctx.value);
    cpe_str_toupper(md5);
}

int aos_base64_encode(const unsigned char *in, int inLen, char *out) {
    static const char *ENC = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *original_out = out;

    while (inLen) {
        // first 6 bits of char 1
        *out++ = ENC[*in >> 2];
        if (!--inLen) {
            // last 2 bits of char 1, 4 bits of 0
            *out++ = ENC[(*in & 0x3) << 4];
            *out++ = '=';
            *out++ = '=';
            break;
        }
        // last 2 bits of char 1, first 4 bits of char 2
        *out++ = ENC[((*in & 0x3) << 4) | (*(in + 1) >> 4)];
        in++;
        if (!--inLen) {
            // last 4 bits of char 2, 2 bits of 0
            *out++ = ENC[(*in & 0xF) << 2];
            *out++ = '=';
            break;
        }
        // last 4 bits of char 2, first 2 bits of char 3
        *out++ = ENC[((*in & 0xF) << 2) | (*(in + 1) >> 6)];
        in++;
        // last 6 bits of char 3
        *out++ = ENC[*in & 0x3F];
        in++;
        inLen--;
    }

    return (int)(out - original_out);
}

int signature_to_base64(const char * sig, int sigLen, const char * key, int keyLen, char * base64) {
    unsigned char sha1Buf[20];
    hmac_sha1(sha1Buf, key, keyLen << 3, sig, sigLen << 3);
    return aos_base64_encode((const unsigned char *)sha1Buf, 20, base64);
}
