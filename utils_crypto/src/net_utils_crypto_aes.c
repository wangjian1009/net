#include "mbedtls/aes.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/error.h"
#include "cpe/utils/stream.h"
#include "utils_crypto_aes.h"

#define UTILS_CRYPTO_AES_BLOCK_SIZE (16)

ssize_t utils_crypto_aes_encode(
    write_stream_t ws, read_stream_t is,
    void const * passwd, uint32_t passwd_sz, net_crypto_pending_t pending,
    error_monitor_t em)
{
    return 0;
}

static ssize_t utils_crypto_aes_decode_output_one(mbedtls_aes_context ctx, write_stream_t ws, unsigned char * input_block, uint8_t is_last_block, net_crypto_pending_t pending, error_monitor_t em) {
    unsigned char block[UTILS_CRYPTO_AES_BLOCK_SIZE];

    if (mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, input_block, block) != 0) {
        CPE_ERROR(em, "crypto: decode: decrypto full block fail!");
        return -1;
    }

    if (is_last_block) {
        uint32_t output_sz = UTILS_CRYPTO_AES_BLOCK_SIZE;
        if (pending == net_crypto_pending_pkcs5padding) {
            if (block[UTILS_CRYPTO_AES_BLOCK_SIZE - 1] <= output_sz) {
                output_sz -= block[UTILS_CRYPTO_AES_BLOCK_SIZE - 1];
            }
        }
        else {
            CPE_ERROR(em, "crypto: decode: unknown pending %d!", pending);
            return -1;
        }

        if (stream_write(ws, block, output_sz) != output_sz) {
            CPE_ERROR(em, "crypto: decode: write loast block fail, sz=%d!", output_sz);
            return -1;
        }

        return output_sz;
    }
    else {
        if (stream_write(ws, block, UTILS_CRYPTO_AES_BLOCK_SIZE) != UTILS_CRYPTO_AES_BLOCK_SIZE) {
            CPE_ERROR(em, "crypto: decode: write block fail, sz=%d!", UTILS_CRYPTO_AES_BLOCK_SIZE);
            return -1;
        }

        return UTILS_CRYPTO_AES_BLOCK_SIZE;
    }
}

ssize_t utils_crypto_aes_decode(
    write_stream_t ws, read_stream_t is,
    void const * passwd, uint32_t passwd_sz, net_crypto_pending_t pending,
    error_monitor_t em)
{
    struct mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    if (mbedtls_aes_setkey_dec(&ctx, (unsigned const char*)passwd, passwd_sz) != 0) {
        CPE_ERROR(em, "crypto: decode: set dec code fail!");
        return -1;
    }

    ssize_t rv = 0;
    unsigned char pre_input_block[16];
    size_t pre_input_block_sz = 0;
    do {
        unsigned char input_block[16];
    
        ssize_t sz = stream_read(is, input_block, sizeof(input_block));
        if (sz < 0) {
            CPE_ERROR(em, "crypto: decode: read input block fail!");
            rv = -1;
            break;
        }
        else if (sz == 0) {
            if (pre_input_block_sz) {
                sz = utils_crypto_aes_decode_output_one(ctx, ws, pre_input_block, 1, pending, em);
                if (sz < 0) {
                    rv = -1;
                    break;
                }
                rv += sz;
            }
            break;
        }
        else {
            if (sz < 16) {
                bzero(input_block + sz, sizeof(input_block) - sz);
            }

            if (pre_input_block_sz) {
                sz = utils_crypto_aes_decode_output_one(ctx, ws, pre_input_block, 0, pending, em);
                if (sz < 0) {
                    rv = -1;
                    break;
                }
                rv += sz;
            }

            memcpy(pre_input_block, input_block, sz);
            pre_input_block_sz = sz;
        }
    } while(1);

    mbedtls_aes_free(&ctx);

    return rv; 
}
