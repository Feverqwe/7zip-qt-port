/* SPDX-License-Identifier: MIT */
#include "opensslblock.h"
#include <limits.h>
#include <openssl/evp.h>
#include <string.h>

/* The C++ filter owns the EVP context. The original C counter loop borrows it
 * through its existing key-storage region; failures are checked by the filter
 * before it reports any successfully processed bytes. */
typedef struct {
    EVP_CIPHER_CTX *context;
    int failed;
} CAesContext;
_Static_assert(sizeof(CAesContext) <= 256, "AES state exceeds upstream key storage");

void SEVENZIP_CRYPTO_CALL sevenzip_aes_attach(void *storage, void *context) {
    CAesContext state = {context, 0};
    memcpy(storage, &state, sizeof(state));
}

int SEVENZIP_CRYPTO_CALL sevenzip_aes_blocks(void *storage, const unsigned char *input,
                                             unsigned char *output, size_t blocks) {
    CAesContext state;
    int produced = 0;
    memcpy(&state, storage, sizeof(state));
    if (!state.failed && blocks <= INT_MAX / 16 &&
        EVP_EncryptUpdate(state.context, output, &produced, input, (int)(blocks * 16)) == 1 &&
        produced == (int)(blocks * 16))
        return 1;
    state.failed = 1;
    memcpy(storage, &state, sizeof(state));
    return 0;
}

int SEVENZIP_CRYPTO_CALL sevenzip_aes_failed(const void *storage) {
    CAesContext state;
    memcpy(&state, storage, sizeof(state));
    return state.failed;
}
