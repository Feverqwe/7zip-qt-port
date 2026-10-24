/*
 * QEMU Crypto block device encryption
 * Copyright (c) 2015-2016 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* do_qcrypto_block_cipher_encdec read branch. The fixed legacy QCOW cipher
 * uses 16-byte IVs and 512-byte sectors; native callbacks own the AES context. */
#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464)
#endif
#include "../C/Precomp.h"

#include "block-qcow.h"

int32_t qcrypto_block_cipher_decrypt_helper(void *cipher, QcowCipherDecryptFunc decrypt,
                                            uint64_t offset, unsigned char *buf, size_t len) {
    uint64_t startsector = offset / 512;
    while (len > 0) {
        unsigned char iv[16];
        const size_t nbytes = len > 512 ? 512 : len;
        int32_t ret;
        qcrypto_ivgen_plain_calculate(startsector, iv, sizeof(iv));
        ret = decrypt(cipher, iv, buf, nbytes);
        if (ret != 0)
            return ret;
        startsector++;
        buf += nbytes;
        len -= nbytes;
    }
    return 0;
}
