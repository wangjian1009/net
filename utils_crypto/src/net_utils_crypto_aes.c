#include "mbedtls/aes.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/error.h"
#include "cpe/utils/stream.h"
#include "utils_crypto_aes.h"

ssize_t utils_crypto_aes_encode(
    write_stream_t ws, read_stream_t is,
    void const * passwd, uint_t passwd_sz, net_crypto_pending_t pending,
    error_monitor_t em)
{
    return 0;
}

ssize_t utils_crypto_aes_decode(
    write_stream_t ws, read_stream_t is,
    void const * passwd, uint_t passwd_sz, net_crypto_pending_t pending,
    error_monitor_t em)
{
    struct mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    if (mbedtls_aes_setkey_dec(&ctx, (unsigned const char*)passwd, passwd_sz) != 0) {
        CPE_ERROR(em, "crypto: decode: set dec code fail!");
        return -1;
    }

    ssize_t rv = 0;
    do {
        unsigned char input_block[16];
        unsigned char output_block[16];

        int32_t sz = stream_read(is, input_block, sizeof(input_block));
        if (sz < 0) {
            CPE_ERROR(em, "crypto: decode: read input block fail!");
            rv = -1;
            break;
        }
        else if (sz == 0) {
            break;
        }
        else if (sz < 16) {
            bzero(input_block + sz, sizeof(input_block) - sz);
            if (mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, output_block, input_block) != 0) {
                CPE_ERROR(em, "crypto: decode: decrypto loast block fail!");
                rv = -1;
                break;
            }

            uint32_t output_sz = sizeof(output_block);
            if (pending == net_crypto_pending_a) {
                if (output_block[sizeof(output_block) - 1] < output_sz) {
                    output_sz -= output_block[sizeof(output_block) - 1];
                }
            }
            else {
                CPE_ERROR(em, "crypto: decode: unknown pending %d!", pending);
                rv = -1;
                break;
            }

            if (stream_write(ws, output_block, output_sz) != output_sz) {
                CPE_ERROR(em, "crypto: decode: write loast block fail, sz=%d!", output_sz);
                rv = -1;
                break;
            }

            rv += output_sz;
            break;
        }
        else {
            if (mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, output_block, input_block) != 0) {
                CPE_ERROR(em, "crypto: decode: decrypto full block fail!");
                rv = -1;
                break;
            }

            if (stream_write(ws, output_block, sizeof(output_block)) != sizeof(output_block)) {
                CPE_ERROR(em, "crypto: decode: write block fail, sz=%d!", (int)sizeof(output_block));
                rv = -1;
                break;
            }

            rv += sizeof(output_block);
        }
    } while(1);

    mbedtls_aes_free(&ctx);

    return rv; 
}
