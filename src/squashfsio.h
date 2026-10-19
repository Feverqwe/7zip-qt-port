// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEVENZIP_SQUASHFS_IO_H
#define SEVENZIP_SQUASHFS_IO_H
#include "squashfsdecoder.h"
struct sevenzip_squashfs_io {
    sevenzip_squashfs_read read;
    void *opaque;
};
#endif
