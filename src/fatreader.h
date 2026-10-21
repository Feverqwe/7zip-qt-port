// SPDX-License-Identifier: MIT
// Read-side boundary for the Linux FAT compatibility routines.
#ifndef SEVENZIP_FAT_READER_H
#define SEVENZIP_FAT_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int fat_linux_legacy_header(const void *boot, uint64_t size, void *normalized);
int fat_linux_volume_state(const void *boot, size_t size, int *dirty);
int fat_linux_short_name(const unsigned char alias[11], unsigned char flags, char output[73]);
int fat_linux_oem_name(const unsigned char *bytes, unsigned int length, char *output, int capacity);
int fat_linux_long_name(const void *utf16le, unsigned int units, char *output, unsigned int capacity);
void fat_linux_time(uint32_t dos_time, unsigned char centiseconds, int64_t *seconds, uint32_t *nsec);
#ifdef __cplusplus
}
#endif
#endif
