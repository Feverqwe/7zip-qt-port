// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 1991, 1992 Linus Torvalds.
 * Selected page_get_link read body from Linux fs/namei.c.
 * VFS page acquisition is replaced by an already-read, caller-owned page. */
#include "../pagelink.h"
#include <string.h>
#define min(a, b) ((a) < (b) ? (a) : (b))
#include "../include/linux/namei.h"
#include "../include/linux/delayed_call.h"
#define PAGE_SIZE SEVENZIP_PAGE_LINK_SIZE
#define IS_ERR(p) ((uintptr_t)(p) >= (uintptr_t)-4095)

struct dentry { char *page; };
struct inode { uint64_t i_size; };

static char *__page_get_link(struct dentry *dentry, struct inode *inode,
                            struct delayed_call *callback)
{
    /* Transport errors have already been propagated by the stream owner.
     * No page-cache lookup, RCU path, highmem mapping or reference is needed. */
    (void)inode;
    (void)callback;
    return dentry->page;
}

const char *page_get_link(struct dentry *dentry, struct inode *inode,
					struct delayed_call *callback)
{
	char *kaddr = __page_get_link(dentry, inode, callback);

	if (!IS_ERR(kaddr))
		nd_terminate_link(kaddr, inode->i_size, PAGE_SIZE - 1);
	return kaddr;
}

size_t sevenzip_page_link(void *page, uint64_t file_size)
{
    struct dentry dentry = {page};
    struct inode inode = {file_size};
    DEFINE_DELAYED_CALL(done);
    return strlen(page_get_link(&dentry, &inode, &done));
}
