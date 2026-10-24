/* Aes.c -- AES encryption / decryption
2024-03-01 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include "../crypto/opensslblock.h"
#include "Aes.h"
#include "CpuArch.h"

/* WinZip's pre-incremented little-endian counter is retained from upstream;
 * key expansion and block encryption are supplied exclusively by OpenSSL. */

void AesCbc_Init(UInt32 *p, const Byte *iv) {
    unsigned i;
    for (i = 0; i < 4; i++)
        p[i] = GetUi32(iv + i * 4);
}

void Z7_FASTCALL AesCtr_Code(UInt32 *p, Byte *data, size_t numBlocks) {
    while (numBlocks != 0) {
        /* Batch only the library boundary; retain the source counter/XOR steps. */
        Byte counters[AES_BLOCK_SIZE * 16], encrypted[AES_BLOCK_SIZE * 16];
        const size_t batch = numBlocks > 16 ? 16 : numBlocks;
        size_t block;
        unsigned i;
        for (block = 0; block < batch; block++) {
            if (++p[0] == 0)
                p[1]++;
            for (i = 0; i < 4; i++) {
                SetUi32(counters + block * AES_BLOCK_SIZE + i * 4, p[i])
            }
        }
        if (!sevenzip_aes_blocks(p + 4, counters, encrypted, batch))
            return;
        for (block = 0; block < batch; block++) {
            UInt32 temp[4];
            for (i = 0; i < 4; i++)
                temp[i] = GetUi32(encrypted + block * AES_BLOCK_SIZE + i * 4);
            for (i = 0; i < 4; i++, data += 4) {
                const UInt32 t = temp[i];

#ifdef MY_CPU_LE_UNALIGN
                *((UInt32 *)(void *)data) ^= t;
#else
                data[0] = (Byte)(data[0] ^ (t & 0xFF));
                data[1] = (Byte)(data[1] ^ ((t >> 8) & 0xFF));
                data[2] = (Byte)(data[2] ^ ((t >> 16) & 0xFF));
                data[3] = (Byte)(data[3] ^ ((t >> 24)));
#endif
            }
        }
        numBlocks -= batch;
    }
}
