// SPDX-License-Identifier: GPL-2.0
/* Linux fs/btrfs/fs.c checksum dispatch with existing portable dependencies. */
#include "formats/btrfs/fs.h"
#include "C/Sha256.h"
#include "C/Xxh64.h"
#include "formats/btrfs/btrfsreader.h"
#include "compat/linux/include/crypto/blake2b.h"
#include "compat/linux/include/uapi/linux/btrfs_tree.h"

u32 crc32c(u32, const void *, size_t);

/* Original Linux checksum table; unrecognized types have no usable size. */
static const struct btrfs_csums {
    u16 size;
    const char name[10];
} btrfs_csums[] = {{4, "crc32c"}, {8, "xxhash64"}, {32, "sha256"}, {32, "blake2b"}};

u16 btrfs_csum_type_size(u16 type) {
    return type < sizeof(btrfs_csums) / sizeof(btrfs_csums[0]) ? btrfs_csums[type].size : 0;
}

static void sha256(const u8 *data, size_t len, u8 *out) {
    CSha256 state;
    Sha256_Init(&state);
    /* The software implementation needs no global CPU-dispatch preparation. */
    Sha256_SetFunction(&state, SHA256_ALGO_SW);
    Sha256_Update(&state, data, len);
    Sha256_Final(&state, out);
}

static u64 xxh64(const u8 *data, size_t len, unsigned int seed) {
    CXxh64 state;
    (void)seed; /* Btrfs uses the standard XXH64 seed 0. */
    Xxh64_Init(&state);
    Xxh64_Update(&state, data, len);
    return Xxh64_Digest(&state);
}

static void put_unaligned_le32(u32 n, void *p) {
    u8 *b = p;
    for (unsigned int i = 0; i < 4; i++)
        b[i] = (u8)(n >> (8 * i));
}
static void put_unaligned_le64(u64 n, void *p) {
    u8 *b = p;
    for (unsigned int i = 0; i < 8; i++)
        b[i] = (u8)(n >> (8 * i));
}

/* Linux btrfs_csum, returning an error instead of a kernel BUG on an unknown type. */
static int btrfs_csum(u16 csum_type, const u8 *data, size_t len, u8 *out) {
    switch (csum_type) {
    case BTRFS_CSUM_TYPE_CRC32:
        put_unaligned_le32(~crc32c(~0U, data, len), out);
        break;
    case BTRFS_CSUM_TYPE_XXHASH:
        put_unaligned_le64(xxh64(data, len, 0), out);
        break;
    case BTRFS_CSUM_TYPE_SHA256:
        sha256(data, len, out);
        break;
    case BTRFS_CSUM_TYPE_BLAKE2:
        blake2b(NULL, 0, data, len, out, 32);
        break;
    default:
        return -EOPNOTSUPP;
    }
    return 0;
}

int btrfs_checksum_native(unsigned int type, const void *data, size_t len, uint8_t result[32]) {
    if ((!data && len) || !result)
        return -EINVAL;
    if (type > UINT16_MAX || !btrfs_csum_type_size((u16)type))
        return -EOPNOTSUPP;
    memset(result, 0, 32);
    return btrfs_csum((u16)type, data, len, result);
}
