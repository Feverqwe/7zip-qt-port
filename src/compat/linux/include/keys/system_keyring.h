/* SPDX-License-Identifier: GPL-2.0-or-later */
/* System keyring containing trusted public keys.
 *
 * Copyright (C) 2013 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#ifndef _KEYS_SYSTEM_KEYRING_H
#define _KEYS_SYSTEM_KEYRING_H
#include "crypto/primitives/userspace.h"
/* Selected original CONFIG_SYSTEM_BLACKLIST_KEYRING=n path. Archive trust
 * uses explicit owned anchors; there is no system blacklist/keyring access. */
enum blacklist_hash_type {
    /* TBSCertificate hash */
    BLACKLIST_HASH_X509_TBS = 1,
    /* Raw data hash */
    BLACKLIST_HASH_BINARY = 2,
};

static inline int is_hash_blacklisted(const u8 *hash, size_t hash_len,
                                      enum blacklist_hash_type hash_type) {
    return 0;
}

struct pkcs7_message;
/* Selected original CONFIG_SYSTEM_REVOCATION_LIST=n service. */
static inline int is_key_on_revocation_list(struct pkcs7_message *pkcs7) { return -ENOKEY; }

#endif
