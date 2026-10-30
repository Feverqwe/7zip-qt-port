/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Malformed contexts and unsafe IV policies must be rejected before key setup. */
#include "crypto/fscrypt/fscrypt_private.h"
#include <stdio.h>

static bool stable_inodes(struct super_block *sb) { return sb->stable_inodes; }
static bool supports(struct inode *inode, const u8 *bytes, size_t len)
{
	union fscrypt_context context = {0};
	union fscrypt_policy policy;
	if (len > sizeof(context)) return false;
	memcpy(&context, bytes, len);
	return fscrypt_policy_from_context(&policy, &context, (int)len) == 0 &&
	       fscrypt_supported_policy(&policy, inode);
}

static int check_contexts(struct inode *inode)
{
	/* Linux on-disk context versions differ from policy versions. */
	u8 legacy[40] = {1, 1, 4, 3};
	u8 modern[40] = {2, 1, 10, 0};
	if (!supports(inode, legacy, 28) || !supports(inode, modern, sizeof(modern)))
		return 1;
	for (unsigned len = 0; len < sizeof(modern); ++len) {
		if (len != 28 && supports(inode, legacy, len)) return 1;
		if (supports(inode, modern, len)) return 1;
	}
	legacy[0] = 0;
	modern[0] = 3;
	if (supports(inode, legacy, 28) || supports(inode, modern, sizeof(modern))) return 1;
	modern[0] = 2;
	for (unsigned reserved = 5; reserved < 8; ++reserved) {
		modern[reserved] = 1;
		if (supports(inode, modern, sizeof(modern))) return 1;
		modern[reserved] = 0;
	}
	return 0;
}

static int check_mode_rules(struct inode *inode)
{
	const struct { u8 version, contents, names, flags; bool folded, accepted; } cases[] = {
		{1, 1, 10, 0, false, false}, /* HCTR2 is a v2 filename mode. */
		{2, 1, 10, 0, false, true},
		{2, 7, 8, 0, false, true},
		{1, 7, 8, 0, false, false},
		{2, 1, 8, 0, false, false},
		{1, 9, 9, 4, false, true},
		{2, 9, 9, 4, false, true},
		{2, 1, 4, 4, false, false}, /* Direct-key needs matching large-IV modes. */
		{2, 9, 9, 12, false, false}, /* Mutually exclusive IV methods. */
		{2, 1, 4, 128, false, false},
		{1, 1, 4, 0, true, false}, /* v1 cannot derive a casefold dirhash key. */
		{2, 1, 4, 0, true, true},
	};
	for (unsigned i = 0; i < ARRAY_SIZE(cases); ++i) {
		u8 context[40] = {cases[i].version, cases[i].contents, cases[i].names, cases[i].flags};
		inode->casefolded = cases[i].folded;
		if (supports(inode, context, context[0] == 1 ? 28 : 40) != cases[i].accepted) return 1;
	}
	inode->casefolded = false;
	return 0;
}

static int check_geometry(struct inode *inode, struct fscrypt_operations *ops)
{
	u8 context[40] = {2, 1, 4, 0, 9};
	if (!supports(inode, context, sizeof(context))) return 1;
	ops->supports_subblock_data_units = false;
	if (supports(inode, context, sizeof(context))) return 1;
	ops->supports_subblock_data_units = true;
	context[4] = 8;
	if (supports(inode, context, sizeof(context))) return 1;
	context[4] = 13;
	if (supports(inode, context, sizeof(context))) return 1;
	context[4] = 12;
	context[3] = 8; /* Inode+logical-index IV, 32 bits for each. */
	if (!supports(inode, context, sizeof(context))) return 1;
	inode->i_sb->stable_inodes = false;
	if (supports(inode, context, sizeof(context))) return 1;
	inode->i_sb->stable_inodes = true;
	ops->has_32bit_inodes = false;
	if (supports(inode, context, sizeof(context))) return 1;
	ops->has_32bit_inodes = true;
	context[4] = 9;
	if (supports(inode, context, sizeof(context))) return 1; /* Too many DUN bits. */
	inode->i_sb->s_maxbytes = (UINT64_C(1) << 41) - 4096;
	if (!supports(inode, context, sizeof(context))) return 1;
	context[3] = 16;
	if (supports(inode, context, sizeof(context))) return 1; /* Ino32 forbids sub-block DUNs. */
	context[4] = 12;
	if (!supports(inode, context, sizeof(context))) return 1;
	inode->i_sb->s_page_size = 65536;
	if (supports(inode, context, sizeof(context))) return 1;
	return 0;
}

int fscrypt_policy_tests(void)
{
	struct fscrypt_operations ops = {.has_stable_inodes = stable_inodes,
		.has_32bit_inodes = true, .supports_subblock_data_units = true};
	struct super_block sb = {.s_cop = &ops, .s_maxbytes = (UINT64_C(1) << 44) - 4096,
		.s_blocksize = 4096, .s_page_size = 4096, .stable_inodes = true};
	struct inode inode = {.i_sb = &sb, .i_blkbits = 12};
	if (check_contexts(&inode) || check_mode_rules(&inode) || check_geometry(&inode, &ops)) {
		fputs("fscrypt context/policy acceptance failed\n", stderr);
		return 1;
	}
	return 0;
}
