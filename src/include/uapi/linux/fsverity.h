/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * fs-verity user API
 *
 * These ioctls can be used on filesystems that support fs-verity.  See the
 * "User API" section of Documentation/filesystems/fsverity.rst.
 *
 * Copyright 2019 Google LLC
 */
#ifndef SEVENZIP_FSVERITY_DISK_H
#define SEVENZIP_FSVERITY_DISK_H
#define FS_VERITY_HASH_ALG_SHA256 1
#define FS_VERITY_HASH_ALG_SHA512 2
struct fsverity_descriptor {
	__u8 version;		/* must be 1 */
	__u8 hash_algorithm;	/* Merkle tree hash algorithm */
	__u8 log_blocksize;	/* log2 of size of data and tree blocks */
	__u8 salt_size;		/* size of salt in bytes; 0 if none */
#if 1
	__le32 sig_size;
#else
	__le32 __reserved_0x04;	/* must be 0 */
#endif
	__le64 data_size;	/* size of file the Merkle tree is built over */
	__u8 root_hash[64];	/* Merkle tree root hash */
	__u8 salt[32];		/* salt prepended to each hashed block */
	__u8 __reserved[144];	/* must be 0's */
#if 1
	__u8 signature[];
#endif
};

#endif
