/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Native ext4 ciphertext proves policy/KDF/IV/name composition and key lifetime. */
#include "../src/fscryptreader.h"
#include "../src/fs/crypto/userspace.h"
#include <stdio.h>

struct fscrypt_native_unit {
	uint64_t index;
	size_t size;
	const unsigned char *cipher;
	const char *digest;
};
struct fscrypt_native_vector {
	const char *name;
	unsigned key_type;
	int casefolded;
	uint32_t hash, minor_hash;
	const char *raw_key, *descriptor, *file_context, *directory_context, *symlink_context;
	size_t raw_key_size, descriptor_size, file_context_size, directory_context_size, symlink_context_size;
	uint64_t file_inode, directory_inode, symlink_inode;
	const char *cipher_name, *symlink_payload;
	size_t cipher_name_size, symlink_payload_size;
	const char *filename, *symlink_target, *nokey_name, *nokey_symlink_target;
	const struct fscrypt_native_unit *units;
	size_t unit_count;
};
#include "fixtures/fscrypt/native-vectors.h"

#define CHECK(e) do { if (!(e)) { fprintf(stderr, "%s key=%d line=%d: %s\n", \
	v->name, with_key, __LINE__, #e); failed = 1; goto out; } } while (0)

static int check_native(const struct fscrypt_native_vector *v, bool with_key)
{
	struct sevenzip_fscrypt_key *key = NULL, *decoy = NULL;
	struct sevenzip_fscrypt_inode *nodes[4] = {0};
	struct sevenzip_fscrypt_geometry geometry = {.max_file_size = native_max_file_size,
		.block_bits = 12, .stable_inodes = 1, .has_32bit_inodes = 1, .supports_subblock_units = 1};
	memcpy(geometry.uuid, native_uuid, 16);
	int failed = 0;
	unsigned char wrong_raw[64], wrong_descriptor[8];
	memset(wrong_raw, 0x63, sizeof(wrong_raw));
	memset(wrong_descriptor, 0x9c, sizeof(wrong_descriptor));
	CHECK(!sevenzip_fscrypt_key_create(v->key_type, v->raw_key, v->raw_key_size,
		(const uint8_t *)v->descriptor, &key));
	CHECK(!sevenzip_fscrypt_key_create(v->key_type, wrong_raw, sizeof(wrong_raw), wrong_descriptor, &decoy));
	const struct sevenzip_fscrypt_key *keys[] = {decoy, with_key ? key : NULL};
	const char *contexts[] = {v->file_context, v->directory_context, v->symlink_context};
	const size_t context_sizes[] = {v->file_context_size, v->directory_context_size, v->symlink_context_size};
	const uint64_t inodes[] = {v->file_inode, v->directory_inode, v->symlink_inode};
	const unsigned modes[] = {0100644, 0040755, 0120777};
	for (unsigned i = 0; i < 3; ++i) {
		unsigned char context[40];
		memcpy(context, contexts[i], context_sizes[i]);
		struct sevenzip_fscrypt_inode_options options = {.geometry = &geometry,
			.inode_number = inodes[i], .mode = modes[i], .context = context,
			.context_size = context_sizes[i], .keys = keys, .key_count = 2,
			.casefolded = i == 1 && v->casefolded};
		CHECK(!sevenzip_fscrypt_inode_create(&options, &nodes[i]));
		CHECK(sevenzip_fscrypt_has_key(nodes[i]) == with_key);
		memset(context, 0x33, sizeof(context));
	}
	CHECK(sevenzip_fscrypt_permitted_context(nodes[1], nodes[0], 0));
	CHECK(sevenzip_fscrypt_permitted_context(nodes[1], nodes[2], 0));
	CHECK(!sevenzip_fscrypt_permitted_context(nodes[1], NULL, 0100644));
	CHECK(sevenzip_fscrypt_permitted_context(nodes[1], NULL, 0020600));
	CHECK(sevenzip_fscrypt_permitted_context(NULL, nodes[0], 0));
	unsigned char other_context[40];
	memcpy(other_context, v->file_context, v->file_context_size);
	other_context[v->key_type == 1 ? 4 : 8] ^= 1;
	struct sevenzip_fscrypt_inode_options other = {.geometry = &geometry,
		.inode_number = v->file_inode, .mode = 0100644, .context = other_context,
		.context_size = v->file_context_size};
	CHECK(!sevenzip_fscrypt_inode_create(&other, &nodes[3]));
	CHECK(!sevenzip_fscrypt_permitted_context(nodes[1], nodes[3], 0));
	/* Streams must remain readable after every constructor input/key is gone. */
	sevenzip_fscrypt_key_free(key); key = NULL;
	sevenzip_fscrypt_key_free(decoy); decoy = NULL;
	memset(&geometry, 0, sizeof(geometry));
	keys[0] = keys[1] = NULL;
	unsigned char output[4097], digest[32];
	for (size_t i = 0; i < v->unit_count; ++i) {
		const struct fscrypt_native_unit *unit = &v->units[i];
		int err = sevenzip_fscrypt_decrypt_unit(nodes[0], unit->index, unit->cipher,
			output + 1, unit->size);
		if (with_key) {
			CHECK(!err);
			sha256(output + 1, unit->size, digest);
			CHECK(!memcmp(digest, unit->digest, sizeof(digest)));
			memcpy(output + 1, unit->cipher, unit->size);
			CHECK(!sevenzip_fscrypt_decrypt_unit(nodes[0], unit->index, output + 1,
				output + 1, unit->size));
			sha256(output + 1, unit->size, digest);
			CHECK(!memcmp(digest, unit->digest, sizeof(digest)));
		} else CHECK(err == -ENOKEY);
	}
	if (with_key) {
		const struct fscrypt_native_unit *unit = &v->units[0];
		CHECK(sevenzip_fscrypt_decrypt_unit(nodes[0], 0, unit->cipher, output, unit->size - 1) == -EINVAL);
		CHECK(sevenzip_fscrypt_decrypt_unit(nodes[0], UINT64_MAX, unit->cipher, output, unit->size) == -EINVAL);
	}
	size_t len = 0;
	const char *expected_name = with_key ? v->filename : v->nokey_name;
	CHECK(!sevenzip_fscrypt_present_name(nodes[1], v->hash, v->minor_hash, v->cipher_name, v->cipher_name_size,
		output, sizeof(output), &len));
	CHECK(len == strlen(expected_name) && !memcmp(output, expected_name, len + 1));
	if (with_key && v->casefolded) {
		/* This fixture's lowercase name is unchanged by native Unicode folding. */
		uint64_t hash = 0;
		CHECK(!sevenzip_fscrypt_hash_name(nodes[1], v->filename, strlen(v->filename), &hash));
		CHECK(((uint32_t)(hash >> 32) & ~UINT32_C(1)) == v->hash && (uint32_t)hash == v->minor_hash);
	}
	CHECK(sevenzip_fscrypt_present_name(nodes[1], 0, 0, v->cipher_name, v->cipher_name_size,
		output, len, &len) == -ERANGE);
	CHECK(sevenzip_fscrypt_present_name(nodes[0], 0, 0, v->cipher_name, v->cipher_name_size,
		output, sizeof(output), &len) == -EINVAL);
	CHECK(sevenzip_fscrypt_present_name(nodes[1], 0, 0, "bad", 3,
		output, sizeof(output), &len) == -EUCLEAN);
	CHECK(!sevenzip_fscrypt_present_name(nodes[1], 0, 0, "..", 2,
		output, sizeof(output), &len) && len == 2 && !memcmp(output, "..", 3));
	const char *expected_link = with_key ? v->symlink_target : v->nokey_symlink_target;
	CHECK(!sevenzip_fscrypt_present_symlink(nodes[2], v->symlink_payload, v->symlink_payload_size,
		output, sizeof(output), &len));
	CHECK(len == strlen(expected_link) && !memcmp(output, expected_link, len + 1));
	CHECK(sevenzip_fscrypt_present_symlink(nodes[2], v->symlink_payload, v->symlink_payload_size - 1,
		output, sizeof(output), &len) == -EUCLEAN);
	CHECK(sevenzip_fscrypt_present_symlink(nodes[2], "\0\0", 2,
		output, sizeof(output), &len) == -EUCLEAN);
out:
	for (unsigned i = 0; i < 4; ++i) sevenzip_fscrypt_inode_free(nodes[i]);
	sevenzip_fscrypt_key_free(key);
	sevenzip_fscrypt_key_free(decoy);
	return failed;
}

int fscrypt_reader_tests(void)
{
	/* Public construction distinguishes missing, malformed and unsupported
	 * metadata. Tolerating unsupported names must not hide a missing xattr. */
	struct sevenzip_fscrypt_geometry geometry = {.max_file_size = native_max_file_size,
		.block_bits = 12, .stable_inodes = 1, .has_32bit_inodes = 1};
	unsigned char invalid[41] = {1, 1, 4, 0};
	const struct { const void *context; size_t size; int expected; } cases[] = {
		{NULL, 0, -ENODATA}, {invalid, 0, -EINVAL}, {invalid, 27, -EINVAL},
		{invalid, 29, -EINVAL}, {invalid, 41, -ERANGE}};
	for (unsigned allow = 0; allow < 2; ++allow) {
		for (size_t i = 0; i < ARRAY_SIZE(cases); ++i) {
			struct sevenzip_fscrypt_inode_options options = {.geometry = &geometry,
				.inode_number = 17, .mode = 0040755, .context = cases[i].context,
				.context_size = cases[i].size, .allow_unsupported = (int)allow};
			struct sevenzip_fscrypt_inode *node = NULL;
			int err = sevenzip_fscrypt_inode_create(&options, &node);
			int expected = allow && cases[i].context ? 0 : cases[i].expected;
			sevenzip_fscrypt_inode_free(node);
			if (err != expected) {
				fprintf(stderr, "fscrypt context size=%zu allow=%u error=%d expected=%d\n",
					cases[i].size, allow, err, expected);
				return 1;
			}
		}
	}
	for (size_t i = 0; i < ARRAY_SIZE(native_vectors); ++i)
		if (check_native(&native_vectors[i], false) || check_native(&native_vectors[i], true))
			return 1;
	return 0;
}
