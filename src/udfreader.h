// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_UDF_READER_H
#define SEVENZIP_UDF_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*sevenzip_udf_read)(void *, uint16_t, uint32_t, void *, size_t);
typedef int (*sevenzip_udf_extent)(void *, unsigned, uint32_t, uint32_t, uint16_t);
struct sevenzip_udf_sparing;
struct sevenzip_udf_virtual;
struct sevenzip_udf_file {
    uint64_t size;
    uint16_t partition;
    unsigned inline_data;
    const void *data;
    void *handle;
};
typedef int (*sevenzip_udf_open_file)(void *, uint16_t, uint32_t, struct sevenzip_udf_file *);
typedef int (*sevenzip_udf_read_file)(void *, uint32_t, void *, size_t);
typedef void (*sevenzip_udf_close_file)(void *);
int sevenzip_udf_virtual_create(void *, sevenzip_udf_open_file, sevenzip_udf_read_file,
    sevenzip_udf_close_file, uint32_t, uint32_t, uint64_t, uint32_t, uint32_t, uint16_t, uint16_t,
    int, struct sevenzip_udf_virtual **);
void sevenzip_udf_virtual_free(struct sevenzip_udf_virtual *);
int sevenzip_udf_virtual_map(struct sevenzip_udf_virtual *, uint32_t, uint32_t,
                            uint16_t *, uint32_t *);
int sevenzip_udf_sparing_create(void *, sevenzip_udf_read, uint32_t, uint32_t,
                                const void *, struct sevenzip_udf_sparing **);
void sevenzip_udf_sparing_free(struct sevenzip_udf_sparing *);
int sevenzip_udf_sparing_span(const struct sevenzip_udf_sparing *, uint32_t,
                              uint32_t *, uint32_t *);
int sevenzip_udf_icb(void *, sevenzip_udf_read, uint32_t, const uint32_t *,
                     uint32_t, uint16_t *, uint32_t *, void *);
int sevenzip_udf_extents(void *, sevenzip_udf_read, sevenzip_udf_extent,
                         uint32_t, uint32_t, uint16_t, unsigned, int,
                         const void *, uint32_t, uint64_t, uint32_t);
#ifdef __cplusplus
}
#endif
#endif
