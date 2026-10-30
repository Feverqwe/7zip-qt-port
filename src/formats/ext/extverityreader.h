// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_EXT_VERITY_READER_H
#define SEVENZIP_EXT_VERITY_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct ext_verity_reader;
struct sevenzip_asymmetric_trust;
struct ext_verity_signature_options {
    const struct sevenzip_asymmetric_trust *trust;
    int require_signatures;
};
typedef int (*ext_verity_read)(void *, uint64_t position, void *, size_t);
/* Signature options are borrowed only during this synchronous open call. */
int ext_verity_open(ext_verity_read, void *, uint64_t file_size, unsigned block_bits,
                    uint32_t allocated_end, const struct ext_verity_signature_options *,
                    struct ext_verity_reader **, unsigned *verity_block_bits);
void ext_verity_close(struct ext_verity_reader *);
int ext_verity_verify(struct ext_verity_reader *, uint64_t position, const void *block);
#ifdef __cplusplus
}
#endif
#endif
