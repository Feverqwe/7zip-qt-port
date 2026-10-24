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

/* Read-only adaptation of qcow2_do_read_snapshots/qcow2_free_snapshots.
 * Native callbacks replace the QEMU block graph; repair/writing and retention
 * of unknown extra data for rewriting are omitted. */
#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464)
#endif
#include "../C/Precomp.h"

#include "../C/CpuArch.h"
#include "qcow2.h"
#include <stdlib.h>

#define QCOW_MAX_SNAPSHOTS 65536
#define QCOW_MAX_SNAPSHOTS_SIZE (1024 * QCOW_MAX_SNAPSHOTS)
#define QCOW_MAX_SNAPSHOT_EXTRA_DATA 1024

void qcow2_free_snapshots(QCowSnapshot *snapshots, uint32_t nb_snapshots) {
    uint32_t i;
    if (!snapshots)
        return;
    for (i = 0; i < nb_snapshots; i++) {
        free(snapshots[i].name);
        free(snapshots[i].id_str);
    }
    free(snapshots);
}

int32_t qcow2_read_snapshots(Qcow2ReadFunc readfunc, void *opaque, uint64_t snapshots_offset,
                             uint32_t nb_snapshots, uint64_t disk_size, uint64_t file_size,
                             unsigned cluster_bits, QCowSnapshot **snapshots, uint64_t *table_end) {
    QCowSnapshot *items;
    uint64_t offset = snapshots_offset;
    uint32_t i;
    int32_t ret = 1;
    *snapshots = NULL;
    *table_end = 0;
    if (!nb_snapshots)
        return 0;
    if (nb_snapshots > QCOW_MAX_SNAPSHOTS || offset > file_size ||
        (offset & ((UINT64_C(1) << cluster_bits) - 1)) ||
        (uint64_t)nb_snapshots * 40 > file_size - offset)
        return 1;
    items = (QCowSnapshot *)calloc(nb_snapshots, sizeof(*items));
    if (!items)
        return (int32_t)0x8007000e;

    for (i = 0; i < nb_snapshots; i++) {
        unsigned char h[40], extra[24];
        uint32_t known_size, id_str_size, name_size;
        QCowSnapshot *sn = items + i;
        ret = 1;
        offset = (offset + 7) & ~UINT64_C(7);
        if (offset > file_size || sizeof(h) > file_size - offset)
            goto fail;
        ret = readfunc(opaque, offset, h, sizeof(h));
        if (ret != 0)
            goto fail;
        offset += sizeof(h);
        sn->l1_table_offset = GetBe64(h);
        sn->l1_size = GetBe32(h + 8);
        id_str_size = GetBe16(h + 12);
        name_size = GetBe16(h + 14);
        sn->date_sec = GetBe32(h + 16);
        sn->date_nsec = GetBe32(h + 20);
        sn->vm_clock_nsec = GetBe64(h + 24);
        sn->vm_state_size = GetBe32(h + 32);
        sn->extra_data_size = GetBe32(h + 36);
        ret = 1;
        if (sn->extra_data_size > QCOW_MAX_SNAPSHOT_EXTRA_DATA ||
            (uint64_t)sn->extra_data_size + id_str_size + name_size > file_size - offset ||
            offset - snapshots_offset + sn->extra_data_size + id_str_size + name_size >
                QCOW_MAX_SNAPSHOTS_SIZE)
            goto fail;

        known_size =
            sn->extra_data_size < sizeof(extra) ? sn->extra_data_size : (uint32_t)sizeof(extra);
        ret = readfunc(opaque, offset, extra, known_size);
        if (ret != 0)
            goto fail;
        if (known_size >= 8)
            sn->vm_state_size = GetBe64(extra);
        sn->disk_size = known_size >= 16 ? GetBe64(extra + 8) : disk_size;
        sn->icount = known_size >= 24 ? GetBe64(extra + 16) : UINT64_MAX;
        offset += sn->extra_data_size;

        sn->id_str = (char *)malloc(id_str_size + 1);
        sn->name = (char *)malloc(name_size + 1);
        if (!sn->id_str || !sn->name) {
            ret = (int32_t)0x8007000e;
            goto fail;
        }
        ret = readfunc(opaque, offset, sn->id_str, id_str_size);
        if (ret != 0)
            goto fail;
        offset += id_str_size;
        sn->id_str[id_str_size] = '\0';
        ret = readfunc(opaque, offset, sn->name, name_size);
        if (ret != 0)
            goto fail;
        offset += name_size;
        sn->name[name_size] = '\0';
    }
    *snapshots = items;
    *table_end = offset;
    return 0;
fail:
    qcow2_free_snapshots(items, nb_snapshots);
    return ret;
}
