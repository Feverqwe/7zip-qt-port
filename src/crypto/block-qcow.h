/* Read-only adaptation of QEMU's LGPL-2.1-or-later QCOW crypto driver.
 * Full notices and original inventory: block-qcow.c and the QEMU baseline. */
#ifndef SEVENZIP_CRYPTO_BLOCK_QCOW_H
#define SEVENZIP_CRYPTO_BLOCK_QCOW_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void qcrypto_block_qcow_init(const char *password, unsigned char keybuf[16]);
typedef int32_t (*QcowCipherDecryptFunc)(void *cipher, const unsigned char *iv, unsigned char *buf,
                                         size_t len);
int32_t qcrypto_block_qcow_decrypt(void *cipher, QcowCipherDecryptFunc decrypt, uint64_t offset,
                                   unsigned char *buf, size_t len);
int32_t qcrypto_block_cipher_decrypt_helper(void *cipher, QcowCipherDecryptFunc decrypt,
                                            uint64_t offset, unsigned char *buf, size_t len);
void qcrypto_ivgen_plain_calculate(uint64_t sector, unsigned char *iv, size_t niv);
#ifdef __cplusplus
}
#endif
#endif
