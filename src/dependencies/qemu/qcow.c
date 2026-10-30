/*
 * Block driver for the QCOW format
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

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464)
#endif
#include "C/Precomp.h"

#include "dependencies/qemu/qcow.h"

/* QEMU decompress_cluster's descriptor decoding. I/O, cache and the existing
 * 7-Zip Deflate decoder are owned by the native archive stream adapter. */
uint64_t qcow_get_compressed_cluster(uint64_t cluster_offset, unsigned cluster_bits,
                                     size_t *csize) {
    const uint64_t coffset = cluster_offset & ((UINT64_C(1) << (63 - cluster_bits)) - 1);
    *csize = (size_t)(cluster_offset >> (63 - cluster_bits));
    *csize &= ((size_t)1 << cluster_bits) - 1;
    return coffset;
}
