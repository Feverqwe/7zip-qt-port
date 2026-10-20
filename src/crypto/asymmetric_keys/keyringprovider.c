/* SPDX-License-Identifier: GPL-2.0-only */
/* Archive-owned immutable keyring storage for Linux's original ID matching
 * and PKCS#7 trust traversal. No host keyring, permissions or registration. */
#include "userspace.h"
#include "x509_parser.h"
#include <linux/key-type.h>
#include <keys/asymmetric-subtype.h>
#include <crypto/pkcs7.h>
#include <linux/hex.h>
#include "asymmetrictrust.h"

extern const struct asymmetric_key_subtype public_key_subtype;
struct sevenzip_trust_entry {
	u8 *der;
	struct x509_certificate *certificate;
	struct asymmetric_key_ids ids;
	struct key key;
};
struct sevenzip_asymmetric_trust {
	struct key keyring;
	struct sevenzip_trust_entry *entries;
};
static const struct key_type archive_keyring_type = { .name = "keyring" };

/* Each result is a separately owned handle. Its immutable payload is borrowed
 * from the ring, which the synchronous caller retains for the entire check.
 * Concurrent searches therefore do not mutate shared reference counters. */
key_ref_t keyring_search(key_ref_t reference, const struct key_type *type,
			const char *description, bool recurse)
{
	(void)recurse;
	const struct key *ring = key_ref_to_ptr(reference);
	if (!ring || ring->type != &archive_keyring_type) return ERR_PTR(-ENOTDIR);
	struct key_match_data match = { .raw_data = description };
	int err = type->match_preparse(&match);
	if (err) return ERR_PTR(err);
	struct key *found = ERR_PTR(-ENOKEY);
	if (match.cmp) {
		for (size_t i = 0; i < ring->count; ++i) {
			const struct key *candidate = &ring->entries[i].key;
			if (candidate->type != type || !match.cmp(candidate, &match)) continue;
			found = kmemdup(candidate, sizeof(*candidate), GFP_KERNEL);
			if (!found) found = ERR_PTR(-ENOMEM);
			break;
		}
	}
	type->match_free(&match);
	return (key_ref_t)found;
}

void key_put(struct key *key)
{
	if (!IS_ERR(key)) kfree(key);
}

void sevenzip_asymmetric_trust_destroy(struct sevenzip_asymmetric_trust *trust)
{
	if (!trust) return;
	for (size_t i = 0; i < trust->keyring.count; ++i) {
		struct sevenzip_trust_entry *entry = &trust->entries[i];
		kfree(entry->ids.id[2]);
		x509_free_certificate(entry->certificate);
		kfree(entry->der);
	}
	kfree(trust->entries);
	kfree(trust);
}

int sevenzip_asymmetric_trust_create(const struct sevenzip_certificate *certs,
				    size_t count, struct sevenzip_asymmetric_trust **out)
{
	if (!out) return -EINVAL;
	*out = NULL;
	if (count && !certs) return -EINVAL;
	if (count > SIZE_MAX / sizeof(struct sevenzip_trust_entry)) return -EOVERFLOW;
	struct sevenzip_asymmetric_trust *trust = kzalloc_obj(struct sevenzip_asymmetric_trust);
	if (!trust) return -ENOMEM;
	trust->keyring.type = &archive_keyring_type;
	if (count) {
		trust->entries = kzalloc(count * sizeof(*trust->entries), GFP_KERNEL);
		if (!trust->entries) { kfree(trust); return -ENOMEM; }
	}
	trust->keyring.entries = trust->entries;
	trust->keyring.count = count;
	int err = 0;
	for (size_t i = 0; i < count; ++i) {
		struct sevenzip_trust_entry *entry = &trust->entries[i];
		if (!certs[i].data || !certs[i].size) { err = -EINVAL; goto fail; }
		size_t size = certs[i].size;
		if (certs[i].encoding == SEVENZIP_CERTIFICATE_HEX) {
			if (size & 1) { err = -EINVAL; goto fail; }
			size /= 2;
			entry->der = kmalloc(size, GFP_KERNEL);
			if (!entry->der) { err = -ENOMEM; goto fail; }
			err = hex2bin(entry->der, certs[i].data, size);
			if (err) goto fail;
		} else if (certs[i].encoding == SEVENZIP_CERTIFICATE_DER) {
			entry->der = kmemdup(certs[i].data, size, GFP_KERNEL);
		} else { err = -EINVAL; goto fail; }
		if (!entry->der) { err = -ENOMEM; goto fail; }
		struct x509_certificate *cert = x509_cert_parse(entry->der, size);
		if (IS_ERR(cert)) { err = PTR_ERR(cert); goto fail; }
		entry->certificate = cert;
		if (cert->blacklisted) { err = -EKEYREJECTED; goto fail; }
		entry->ids.id[0] = cert->id;
		entry->ids.id[1] = cert->skid;
		/* Same subject-name ID as original x509_key_preparse. */
		struct asymmetric_key_id *subject = asymmetric_key_generate_id(
			cert->raw_subject, cert->raw_subject_size, "", 0);
		if (IS_ERR(subject)) { err = PTR_ERR(subject); goto fail; }
		entry->ids.id[2] = subject;
		entry->key.type = &key_type_asymmetric;
		entry->key.payload.data[asym_crypto] = cert->pub;
		entry->key.payload.data[asym_subtype] = (void *)&public_key_subtype;
		entry->key.payload.data[asym_key_ids] = &entry->ids;
		entry->key.payload.data[asym_auth] = cert->sig;
	}
	*out = trust;
	return 0;
fail:
	sevenzip_asymmetric_trust_destroy(trust);
	return err;
}

size_t sevenzip_asymmetric_trust_count(const struct sevenzip_asymmetric_trust *trust)
{
	return trust ? trust->keyring.count : 0;
}

int sevenzip_asymmetric_trust_verify(const struct sevenzip_asymmetric_trust *trust,
				    const void *data, size_t data_size,
				    const void *signature, size_t signature_size)
{
	if (!trust || !data || !signature) return -EINVAL;
	return verify_pkcs7_signature(data, data_size, signature, signature_size,
		(struct key *)&trust->keyring, VERIFYING_UNSPECIFIED_SIGNATURE, NULL, NULL);
}
