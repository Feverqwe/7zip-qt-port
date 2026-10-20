// SPDX-License-Identifier: GPL-2.0-or-later
/* Asymmetric public-key cryptography key type
 *
 * See Documentation/crypto/asymmetric-keys.rst
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */
#include "userspace.h"
#include <keys/asymmetric-type.h>
#include <linux/key-type.h>
#include <linux/hex.h>

/**
 * asymmetric_key_generate_id: Construct an asymmetric key ID
 * @val_1: First binary blob
 * @len_1: Length of first binary blob
 * @val_2: Second binary blob
 * @len_2: Length of second binary blob
 *
 * Construct an asymmetric key ID from a pair of binary blobs.
 */
struct asymmetric_key_id *asymmetric_key_generate_id(const void *val_1,
						     size_t len_1,
						     const void *val_2,
						     size_t len_2)
{
	struct asymmetric_key_id *kid;
	size_t kid_sz;
	size_t len;

	if (check_add_overflow(len_1, len_2, &len))
		return ERR_PTR(-EOVERFLOW);
	if (check_add_overflow(sizeof(struct asymmetric_key_id), len, &kid_sz))
		return ERR_PTR(-EOVERFLOW);
	kid = kmalloc(kid_sz, GFP_KERNEL);
	if (!kid)
		return ERR_PTR(-ENOMEM);
	kid->len = len;
	memcpy(kid->data, val_1, len_1);
	memcpy(kid->data + len_1, val_2, len_2);
	return kid;
}
EXPORT_SYMBOL_GPL(asymmetric_key_generate_id);

/**
 * asymmetric_key_id_same - Return true if two asymmetric keys IDs are the same.
 * @kid1: The key ID to compare
 * @kid2: The key ID to compare
 */
bool asymmetric_key_id_same(const struct asymmetric_key_id *kid1,
			    const struct asymmetric_key_id *kid2)
{
	if (!kid1 || !kid2)
		return false;
	if (kid1->len != kid2->len)
		return false;
	return memcmp(kid1->data, kid2->data, kid1->len) == 0;
}
EXPORT_SYMBOL_GPL(asymmetric_key_id_same);

/**
 * asymmetric_key_id_partial - Return true if two asymmetric keys IDs
 * partially match
 * @kid1: The key ID to compare
 * @kid2: The key ID to compare
 */
bool asymmetric_key_id_partial(const struct asymmetric_key_id *kid1,
			       const struct asymmetric_key_id *kid2)
{
	if (!kid1 || !kid2)
		return false;
	if (kid1->len < kid2->len)
		return false;
	return memcmp(kid1->data + (kid1->len - kid2->len),
		      kid2->data, kid2->len) == 0;
}
EXPORT_SYMBOL_GPL(asymmetric_key_id_partial);

/**
 * find_asymmetric_key - Find a key by ID.
 * @keyring: The keys to search.
 * @id_0: The first ID to look for or NULL.
 * @id_1: The second ID to look for or NULL, matched together with @id_0
 * against @keyring keys' id[0] and id[1].
 * @id_2: The fallback ID to match against @keyring keys' id[2] if both of the
 * other IDs are NULL.
 * @partial: Use partial match for @id_0 and @id_1 if true, exact if false.
 *
 * Find a key in the given keyring by identifier.  The preferred identifier is
 * the id_0 and the fallback identifier is the id_1.  If both are given, the
 * former is matched (exactly or partially) against either of the sought key's
 * identifiers and the latter must match the found key's second identifier
 * exactly.  If both are missing, id_2 must match the sought key's third
 * identifier exactly.
 */
struct key *find_asymmetric_key(struct key *keyring,
				const struct asymmetric_key_id *id_0,
				const struct asymmetric_key_id *id_1,
				const struct asymmetric_key_id *id_2,
				bool partial)
{
	struct key *key;
	key_ref_t ref;
	const void *lookup;
	char *req, *p;
	int len;

	if (id_0) {
		lookup = id_0->data;
		len = id_0->len;
	} else if (id_1) {
		lookup = id_1->data;
		len = id_1->len;
	} else if (id_2) {
		lookup = id_2->data;
		len = id_2->len;
	} else {
		WARN_ON(1);
		return ERR_PTR(-EINVAL);
	}

	/* Construct an identifier "id:<keyid>". */
	p = req = kmalloc(2 + 1 + len * 2 + 1, GFP_KERNEL);
	if (!req)
		return ERR_PTR(-ENOMEM);

	if (!id_0 && !id_1) {
		*p++ = 'd';
		*p++ = 'n';
	} else if (partial) {
		*p++ = 'i';
		*p++ = 'd';
	} else {
		*p++ = 'e';
		*p++ = 'x';
	}
	*p++ = ':';
	p = bin2hex(p, lookup, len);
	*p = 0;

	pr_debug("Look up: \"%s\"\n", req);

	ref = keyring_search(make_key_ref(keyring, 1),
			     &key_type_asymmetric, req, true);
	if (IS_ERR(ref))
		pr_debug("Request for key '%s' err %ld\n", req, PTR_ERR(ref));
	kfree(req);

	if (IS_ERR(ref)) {
		switch (PTR_ERR(ref)) {
			/* Hide some search errors */
		case -EACCES:
		case -ENOTDIR:
		case -EAGAIN:
			return ERR_PTR(-ENOKEY);
		default:
			return ERR_CAST(ref);
		}
	}

	key = key_ref_to_ptr(ref);
	if (id_0 && id_1) {
		const struct asymmetric_key_ids *kids = asymmetric_key_ids(key);

		if (!kids->id[1]) {
			pr_debug("First ID matches, but second is missing\n");
			goto reject;
		}
		if (!asymmetric_key_id_same(id_1, kids->id[1])) {
			pr_debug("First ID matches, but second does not\n");
			goto reject;
		}
	}

	pr_devel("<==%s() = 0 [%x]\n", __func__, key_serial(key));
	return key;

reject:
	key_put(key);
	return ERR_PTR(-EKEYREJECTED);
}
EXPORT_SYMBOL_GPL(find_asymmetric_key);

/**
 * asymmetric_match_key_ids - Search asymmetric key IDs 1 & 2
 * @kids: The pair of key IDs to check
 * @match_id: The key ID we're looking for
 * @match: The match function to use
 */
static bool asymmetric_match_key_ids(
	const struct asymmetric_key_ids *kids,
	const struct asymmetric_key_id *match_id,
	bool (*match)(const struct asymmetric_key_id *kid1,
		      const struct asymmetric_key_id *kid2))
{
	int i;

	if (!kids || !match_id)
		return false;
	for (i = 0; i < 2; i++)
		if (match(kids->id[i], match_id))
			return true;
	return false;
}

/* helper function can be called directly with pre-allocated memory */
static inline int __asymmetric_key_hex_to_key_id(const char *id,
				   struct asymmetric_key_id *match_id,
				   size_t hexlen)
{
	match_id->len = hexlen;
	return hex2bin(match_id->data, id, hexlen);
}

/**
 * asymmetric_key_hex_to_key_id - Convert a hex string into a key ID.
 * @id: The ID as a hex string.
 */
struct asymmetric_key_id *asymmetric_key_hex_to_key_id(const char *id)
{
	struct asymmetric_key_id *match_id;
	size_t asciihexlen;
	int ret;

	if (!*id)
		return ERR_PTR(-EINVAL);
	asciihexlen = strlen(id);
	if (asciihexlen & 1)
		return ERR_PTR(-EINVAL);

	match_id = kmalloc(sizeof(struct asymmetric_key_id) + asciihexlen / 2,
			   GFP_KERNEL);
	if (!match_id)
		return ERR_PTR(-ENOMEM);
	ret = __asymmetric_key_hex_to_key_id(id, match_id, asciihexlen / 2);
	if (ret < 0) {
		kfree(match_id);
		return ERR_PTR(-EINVAL);
	}
	return match_id;
}

/*
 * Match asymmetric keys by an exact match on one of the first two IDs.
 */
static bool asymmetric_key_cmp(const struct key *key,
			       const struct key_match_data *match_data)
{
	const struct asymmetric_key_ids *kids = asymmetric_key_ids(key);
	const struct asymmetric_key_id *match_id = match_data->preparsed;

	return asymmetric_match_key_ids(kids, match_id,
					asymmetric_key_id_same);
}

/*
 * Match asymmetric keys by a partial match on one of the first two IDs.
 */
static bool asymmetric_key_cmp_partial(const struct key *key,
				       const struct key_match_data *match_data)
{
	const struct asymmetric_key_ids *kids = asymmetric_key_ids(key);
	const struct asymmetric_key_id *match_id = match_data->preparsed;

	return asymmetric_match_key_ids(kids, match_id,
					asymmetric_key_id_partial);
}

/*
 * Match asymmetric keys by an exact match on the third IDs.
 */
static bool asymmetric_key_cmp_name(const struct key *key,
				    const struct key_match_data *match_data)
{
	const struct asymmetric_key_ids *kids = asymmetric_key_ids(key);
	const struct asymmetric_key_id *match_id = match_data->preparsed;

	return kids && asymmetric_key_id_same(kids->id[2], match_id);
}

/*
 * Preparse the match criterion.  If we don't set lookup_type and cmp,
 * the default will be an exact match on the key description.
 *
 * There are some specifiers for matching key IDs rather than by the key
 * description:
 *
 *	"id:<id>" - find a key by partial match on one of the first two IDs
 *	"ex:<id>" - find a key by exact match on one of the first two IDs
 *	"dn:<id>" - find a key by exact match on the third ID
 *
 * These have to be searched by iteration rather than by direct lookup because
 * the key is hashed according to its description.
 */
static int asymmetric_key_match_preparse(struct key_match_data *match_data)
{
	struct asymmetric_key_id *match_id;
	const char *spec = match_data->raw_data;
	const char *id;
	bool (*cmp)(const struct key *, const struct key_match_data *) =
		asymmetric_key_cmp;

	if (!spec || !*spec)
		return -EINVAL;
	if (spec[0] == 'i' &&
	    spec[1] == 'd' &&
	    spec[2] == ':') {
		id = spec + 3;
		cmp = asymmetric_key_cmp_partial;
	} else if (spec[0] == 'e' &&
		   spec[1] == 'x' &&
		   spec[2] == ':') {
		id = spec + 3;
	} else if (spec[0] == 'd' &&
		   spec[1] == 'n' &&
		   spec[2] == ':') {
		id = spec + 3;
		cmp = asymmetric_key_cmp_name;
	} else {
		goto default_match;
	}

	match_id = asymmetric_key_hex_to_key_id(id);
	if (IS_ERR(match_id))
		return PTR_ERR(match_id);

	match_data->preparsed = match_id;
	match_data->cmp = cmp;
	match_data->lookup_type = KEYRING_SEARCH_LOOKUP_ITERATE;
	return 0;

default_match:
	return 0;
}

/*
 * Free the preparsed the match criterion.
 */
static void asymmetric_key_match_free(struct key_match_data *match_data)
{
	kfree(match_data->preparsed);
}

/* Selected original immutable type metadata for owned keyring lookup. */
const struct key_type key_type_asymmetric = {
	.name = "asymmetric",
	.match_preparse = asymmetric_key_match_preparse,
	.match_free = asymmetric_key_match_free,
};
