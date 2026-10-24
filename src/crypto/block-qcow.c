/*
 * QEMU Crypto block device encryption QCow/QCow2 AES-CBC format
 *
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

/*
 * Note that the block encryption implemented in this file is broken
 * by design. This exists only to allow data to be liberated from
 * existing qcow[2] images and should not be used in any new areas.
 */

/* Read-side adaptation: native callbacks replace QEMU's secret lookup and
 * cipher backend. The existing 7-Zip AES provider owns cipher initialization. */
#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464)
#endif
#include "../C/Precomp.h"

#include "block-qcow.h"
#include <string.h>

void qcrypto_block_qcow_init(const char *password, unsigned char keybuf[16]) {
    const size_t len = strlen(password);
    memset(keybuf, 0, 16);
    memcpy(keybuf, password, len < 16 ? len : 16);
}

int32_t qcrypto_block_qcow_decrypt(void *cipher, QcowCipherDecryptFunc decrypt, uint64_t offset,
                                   unsigned char *buf, size_t len) {
    if ((offset & 511) || (len & 511))
        return 1;
    return qcrypto_block_cipher_decrypt_helper(cipher, decrypt, offset, buf, len);
}
