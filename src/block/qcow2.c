/*
 * Block driver for the QCOW version 2 format
 *
 * Copyright (c) 2004-2006 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Adapted qcow2_read_extensions: preserve the read-side extension dispatch;
 * 7-Zip supplies bounded stream reads instead of the QEMU block graph.
 * Feature/bitmap writers and crypto-context creation stay with their owners.
 * Provenance: docs/qcow-qemu-baseline.json and the QCOW source audit.
 */
#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) /* relative include path, as in 7-Zip Common.h */
#endif
#include "../C/Precomp.h"

#include "../C/CpuArch.h"
#include "qcow2.h"
#include <stdlib.h>
#include <string.h>

#define QCOW2_EXT_MAGIC_END 0
#define QCOW2_EXT_MAGIC_BACKING_FORMAT 0xe2792aca
#define QCOW2_EXT_MAGIC_DATA_FILE 0x44415441
#define QCOW2_EXT_MAGIC_CRYPTO_HEADER 0x0537be77

void qcow2_free_extensions(Qcow2HeaderExtensions *extensions) {
    free(extensions->image_backing_format);
    free(extensions->image_data_file);
    memset(extensions, 0, sizeof(*extensions));
}

int32_t qcow2_read_extensions(Qcow2ReadFunc readfunc, void *opaque, uint64_t start_offset,
                              uint64_t end_offset, uint64_t file_size,
                              Qcow2HeaderExtensions *extensions) {
    uint64_t offset = start_offset;
    int32_t ret;

    if (offset > end_offset || end_offset > file_size)
        return 1;

    while (offset < end_offset) {
        unsigned char header[8];
        uint32_t magic, len;
        uint64_t padded;
        char **name;

        if (end_offset - offset < sizeof(header))
            return 1;
        ret = readfunc(opaque, offset, header, sizeof(header));
        if (ret != 0)
            return ret;
        magic = GetBe32(header);
        len = GetBe32(header + 4);
        offset += sizeof(header);
        if (len > end_offset - offset)
            return 1;

        switch (magic) {
        case QCOW2_EXT_MAGIC_END:
            return 0;

        case QCOW2_EXT_MAGIC_BACKING_FORMAT:
        case QCOW2_EXT_MAGIC_DATA_FILE:
            name = magic == QCOW2_EXT_MAGIC_DATA_FILE ? &extensions->image_data_file
                                                      : &extensions->image_backing_format;
            if (*name)
                return 1;
            *name = (char *)malloc((size_t)len + 1);
            if (!*name)
                return (int32_t)0x8007000e; /* E_OUTOFMEMORY */
            ret = readfunc(opaque, offset, *name, len);
            if (ret != 0)
                return ret;
            if (memchr(*name, 0, len))
                return 1;
            (*name)[len] = 0;
            break;

        case QCOW2_EXT_MAGIC_CRYPTO_HEADER: {
            unsigned char crypto_header[16];
            if (len != sizeof(crypto_header) || extensions->has_crypto)
                return 1;
            ret = readfunc(opaque, offset, crypto_header, sizeof(crypto_header));
            if (ret != 0)
                return ret;
            extensions->crypto_offset = GetBe64(crypto_header);
            extensions->crypto_length = GetBe64(crypto_header + 8);
            if (extensions->crypto_offset > file_size ||
                extensions->crypto_length > file_size - extensions->crypto_offset)
                return 1;
            extensions->has_crypto = true;
            break;
        }

        default:
            /* Feature/bitmap extensions do not alter the guest data mapping.
             * Unknown extensions are skipped as prescribed by QEMU. */
            break;
        }

        padded = ((uint64_t)len + 7) & ~(uint64_t)7;
        if (padded > end_offset - offset)
            return 1;
        offset += padded;
    }
    return 0;
}
