// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_LINUX_FRAME_H
#define SEVENZIP_LINUX_FRAME_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_linux_frame_io {
    void *opaque;
    long (*fill)(void *, void *, unsigned long);
    long (*flush)(void *, void *, unsigned long);
    void (*error)(void *, const char *);
    int allocation_failed;
};
int unlz4(struct sevenzip_linux_frame_io *, uint64_t *consumed);
int unlzo(struct sevenzip_linux_frame_io *, uint64_t *consumed);
int sevenzip_lzop_probe(unsigned char *, size_t);
#ifdef __cplusplus
}
#endif
#endif
