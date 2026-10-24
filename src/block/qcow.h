/* Read-only QCOW1 descriptor adapter; see qcow.c for upstream notice. */
#ifndef SEVENZIP_QCOW_H
#define SEVENZIP_QCOW_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Adapted from QEMU decompress_cluster; caller supplies validated cluster_bits. */
uint64_t qcow_get_compressed_cluster(uint64_t cluster_offset, unsigned cluster_bits, size_t *csize);

#ifdef __cplusplus
}
#endif
#endif
