// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_EXT_VERITY_READER_H
#define SEVENZIP_EXT_VERITY_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct ext_verity_reader;
typedef int (*ext_verity_read)(void *, uint64_t position, void *, size_t);
int ext_verity_open(ext_verity_read, void *, uint64_t file_size, unsigned block_bits,
                    uint32_t allocated_end, struct ext_verity_reader **, unsigned *verity_block_bits);
void ext_verity_close(struct ext_verity_reader *);
int ext_verity_verify(struct ext_verity_reader *, uint64_t position, const void *block);
#ifdef __cplusplus
}
#endif
#endif
