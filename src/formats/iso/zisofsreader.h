// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_ZISOFS_READER_H
#define SEVENZIP_ZISOFS_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_zisofs;
typedef int (*sevenzip_zisofs_read_fn)(void *, uint64_t, void *, size_t);
int sevenzip_zisofs_open(uint64_t, uint64_t, unsigned, unsigned, unsigned, sevenzip_zisofs_read_fn,
                         void *, struct sevenzip_zisofs **);
int sevenzip_zisofs_read(struct sevenzip_zisofs *, uint64_t, void *, size_t, size_t *);
void sevenzip_zisofs_close(struct sevenzip_zisofs *);
#ifdef __cplusplus
}
#endif
#endif
