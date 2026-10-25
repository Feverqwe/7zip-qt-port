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

/* Read-only adaptation: selected QCOW metadata interfaces and classification.
 * Block graph state is supplied by the native stream adapters.
 * Source revision and omitted scope: docs/upstream-port-audit/container-layers.md.
 */
#ifndef SEVENZIP_BLOCK_QCOW2_H
#define SEVENZIP_BLOCK_QCOW2_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Read-only stream service replacing QEMU's block graph I/O. Results use
 * zero for success; nonzero status values propagate without translation. */
typedef int32_t (*Qcow2ReadFunc)(void *opaque, uint64_t offset, void *data, size_t size);

typedef struct Qcow2HeaderExtensions {
    char *image_backing_format;
    char *image_data_file;
    uint64_t crypto_offset;
    uint64_t crypto_length;
    bool has_crypto;
} Qcow2HeaderExtensions;

#ifdef __cplusplus
extern "C" {
#endif
int32_t qcow2_read_extensions(Qcow2ReadFunc readfunc, void *opaque, uint64_t start_offset,
                              uint64_t end_offset, uint64_t file_size,
                              Qcow2HeaderExtensions *extensions);
void qcow2_free_extensions(Qcow2HeaderExtensions *extensions);
#ifdef __cplusplus
}
#endif

#define L2E_OFFSET_MASK 0x00fffffffffffe00ULL

/* indicate that the refcount of the referenced cluster is exactly one. */
#define QCOW_OFLAG_COPIED (1ULL << 63)
/* indicate that the cluster is compressed (they never have the copied flag) */
#define QCOW_OFLAG_COMPRESSED (1ULL << 62)
/* The cluster reads as all zeros */
#define QCOW_OFLAG_ZERO (1ULL << 0)

#define QCOW_EXTL2_SUBCLUSTERS_PER_CLUSTER 32

/* The subcluster X [0..31] is allocated */
#define QCOW_OFLAG_SUB_ALLOC(X) (1ULL << (X))
/* The subcluster X [0..31] reads as zeroes */
#define QCOW_OFLAG_SUB_ZERO(X) (QCOW_OFLAG_SUB_ALLOC(X) << 32)
/* Subclusters [X, Y) (0 <= X <= Y <= 32) are allocated */
#define QCOW_OFLAG_SUB_ALLOC_RANGE(X, Y) (QCOW_OFLAG_SUB_ALLOC(Y) - QCOW_OFLAG_SUB_ALLOC(X))
/* Subclusters [X, Y) (0 <= X <= Y <= 32) read as zeroes */
#define QCOW_OFLAG_SUB_ZERO_RANGE(X, Y) (QCOW_OFLAG_SUB_ALLOC_RANGE(X, Y) << 32)
/* L2 entry bitmap with all allocation bits set */
#define QCOW_L2_BITMAP_ALL_ALLOC (QCOW_OFLAG_SUB_ALLOC_RANGE(0, 32))
/* L2 entry bitmap with all "read as zeroes" bits set */
#define QCOW_L2_BITMAP_ALL_ZEROES (QCOW_OFLAG_SUB_ZERO_RANGE(0, 32))

typedef enum QCow2ClusterType {
    QCOW2_CLUSTER_UNALLOCATED,
    QCOW2_CLUSTER_ZERO_PLAIN,
    QCOW2_CLUSTER_ZERO_ALLOC,
    QCOW2_CLUSTER_NORMAL,
    QCOW2_CLUSTER_COMPRESSED,
} QCow2ClusterType;

typedef enum QCow2SubclusterType {
    QCOW2_SUBCLUSTER_UNALLOCATED_PLAIN,
    QCOW2_SUBCLUSTER_UNALLOCATED_ALLOC,
    QCOW2_SUBCLUSTER_ZERO_PLAIN,
    QCOW2_SUBCLUSTER_ZERO_ALLOC,
    QCOW2_SUBCLUSTER_NORMAL,
    QCOW2_SUBCLUSTER_COMPRESSED,
    QCOW2_SUBCLUSTER_INVALID,
} QCow2SubclusterType;

static inline QCow2ClusterType qcow2_get_cluster_type(bool extended_l2, bool external_data,
                                                      uint64_t l2_entry) {

    if (l2_entry & QCOW_OFLAG_COMPRESSED) {
        return QCOW2_CLUSTER_COMPRESSED;
    } else if ((l2_entry & QCOW_OFLAG_ZERO) && !extended_l2) {
        if (l2_entry & L2E_OFFSET_MASK) {
            return QCOW2_CLUSTER_ZERO_ALLOC;
        }
        return QCOW2_CLUSTER_ZERO_PLAIN;
    } else if (!(l2_entry & L2E_OFFSET_MASK)) {
        /* Offset 0 generally means unallocated, but it is ambiguous with
         * external data files because 0 is a valid offset there. However, all
         * clusters in external data files always have refcount 1, so we can
         * rely on QCOW_OFLAG_COPIED to disambiguate. */
        if (external_data && (l2_entry & QCOW_OFLAG_COPIED)) {
            return QCOW2_CLUSTER_NORMAL;
        } else {
            return QCOW2_CLUSTER_UNALLOCATED;
        }
    } else {
        return QCOW2_CLUSTER_NORMAL;
    }
}

/*
 * In an image without subsclusters @l2_bitmap is ignored and
 * @sc_index must be 0.
 * Return QCOW2_SUBCLUSTER_INVALID if an invalid l2 entry is detected
 * (this checks the whole entry and bitmap, not only the bits related
 * to subcluster @sc_index).
 */
static inline QCow2SubclusterType qcow2_get_subcluster_type(bool extended_l2, bool external_data,
                                                            uint64_t l2_entry, uint64_t l2_bitmap,
                                                            unsigned sc_index) {
    QCow2ClusterType type = qcow2_get_cluster_type(extended_l2, external_data, l2_entry);
    assert(sc_index < (extended_l2 ? 32U : 1U));

    if (extended_l2) {
        switch (type) {
        case QCOW2_CLUSTER_COMPRESSED:
            return QCOW2_SUBCLUSTER_COMPRESSED;
        case QCOW2_CLUSTER_NORMAL:
            if ((l2_bitmap >> 32) & l2_bitmap) {
                return QCOW2_SUBCLUSTER_INVALID;
            } else if (l2_bitmap & QCOW_OFLAG_SUB_ZERO(sc_index)) {
                return QCOW2_SUBCLUSTER_ZERO_ALLOC;
            } else if (l2_bitmap & QCOW_OFLAG_SUB_ALLOC(sc_index)) {
                return QCOW2_SUBCLUSTER_NORMAL;
            } else {
                return QCOW2_SUBCLUSTER_UNALLOCATED_ALLOC;
            }
        case QCOW2_CLUSTER_UNALLOCATED:
            if (l2_bitmap & QCOW_L2_BITMAP_ALL_ALLOC) {
                return QCOW2_SUBCLUSTER_INVALID;
            } else if (l2_bitmap & QCOW_OFLAG_SUB_ZERO(sc_index)) {
                return QCOW2_SUBCLUSTER_ZERO_PLAIN;
            } else {
                return QCOW2_SUBCLUSTER_UNALLOCATED_PLAIN;
            }
        case QCOW2_CLUSTER_ZERO_PLAIN:
        case QCOW2_CLUSTER_ZERO_ALLOC:
        default:
            assert(0);
            return QCOW2_SUBCLUSTER_INVALID;
        }
    } else {
        switch (type) {
        case QCOW2_CLUSTER_COMPRESSED:
            return QCOW2_SUBCLUSTER_COMPRESSED;
        case QCOW2_CLUSTER_ZERO_PLAIN:
            return QCOW2_SUBCLUSTER_ZERO_PLAIN;
        case QCOW2_CLUSTER_ZERO_ALLOC:
            return QCOW2_SUBCLUSTER_ZERO_ALLOC;
        case QCOW2_CLUSTER_NORMAL:
            return QCOW2_SUBCLUSTER_NORMAL;
        case QCOW2_CLUSTER_UNALLOCATED:
            return QCOW2_SUBCLUSTER_UNALLOCATED_PLAIN;
        default:
            assert(0);
            return QCOW2_SUBCLUSTER_INVALID;
        }
    }
}

#endif
