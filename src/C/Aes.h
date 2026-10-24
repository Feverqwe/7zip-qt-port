/* Aes.h -- AES encryption / decryption
2023-04-02 : Igor Pavlov : Public domain */
#ifndef ZIP7_INC_AES_H
#define ZIP7_INC_AES_H
#include "7zTypes.h"
EXTERN_C_BEGIN
#define AES_BLOCK_SIZE 16
/* Preserve upstream's IV/key extent and subsequent CTR keystream position.
 * The key storage is opaque; its size is checked by the OpenSSL adapter. */
#define AES_NUM_IVMRK_WORDS ((1 + 1 + 15) * 4)
void AesCbc_Init(UInt32 *ivAes, const Byte *iv);
void Z7_FASTCALL AesCtr_Code(UInt32 *ivAes, Byte *data, size_t numBlocks);
EXTERN_C_END
#endif
