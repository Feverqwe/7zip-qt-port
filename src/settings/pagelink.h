/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SEVENZIP_PAGE_LINK_H
#define SEVENZIP_PAGE_LINK_H
#include <stddef.h>
#include <stdint.h>
#define SEVENZIP_PAGE_LINK_SIZE 4096
#ifdef __cplusplus
extern "C" {
#endif
/* The archive owner supplies a zero-filled first page containing the file bytes. */
size_t sevenzip_page_link(void *page, uint64_t file_size);
#ifdef __cplusplus
}
#endif
#endif
