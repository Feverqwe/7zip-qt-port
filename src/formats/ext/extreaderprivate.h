/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SEVENZIP_EXT_READER_PRIVATE_H
#define SEVENZIP_EXT_READER_PRIVATE_H
#include "formats/ext/ext4.h"
struct ext_volume {
    struct super_block sb;
    struct ext4_super_block raw;
};
struct ext_node {
    struct inode inode;
    const struct ext_link_services *link_services;
    int xattr_error;
};
#endif
