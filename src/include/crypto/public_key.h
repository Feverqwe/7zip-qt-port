/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Asymmetric public-key algorithm definitions
 *
 * See Documentation/crypto/asymmetric-keys.rst
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#ifndef _LINUX_PUBLIC_KEY_H
#define _LINUX_PUBLIC_KEY_H
#include <linux/oid_registry.h>

/*
 * Cryptographic data for the public-key subtype of the asymmetric key type.
 *
 * Note that this may include private part of the key as well as the public
 * part.
 */
struct public_key {
	void *key;
	u32 keylen;
	enum OID algo;
	void *params;
	u32 paramlen;
	bool key_is_private;
	const char *id_type;
	const char *pkey_algo;
	unsigned long key_eflags;	/* key extension flags */
#define KEY_EFLAG_CA		0	/* set if the CA basic constraints is set */
#define KEY_EFLAG_DIGITALSIG	1	/* set if the digitalSignature usage is set */
#define KEY_EFLAG_KEYCERTSIGN	2	/* set if the keyCertSign usage is set */
};

extern void public_key_free(struct public_key *key);

/*
 * Public key cryptography signature data
 */
struct public_key_signature {
	struct asymmetric_key_id *auth_ids[3];
	u8 *s;			/* Signature */
	u8 *m;			/* Message data to pass to verifier */
	u32 s_size;		/* Number of bytes in signature */
	u32 m_size;		/* Number of bytes in ->m */
	bool m_free;		/* T if ->m needs freeing */
	bool algo_takes_data;	/* T if public key algo operates on data, not a hash */
	const char *pkey_algo;
	const char *hash_algo;
	const char *encoding;
};

extern void public_key_signature_free(struct public_key_signature *sig);
extern int public_key_verify_signature(const struct public_key *,
				       const struct public_key_signature *);
struct key;
extern int verify_signature(const struct key *, const struct public_key_signature *);

#endif
