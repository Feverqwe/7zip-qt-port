/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Selected key payload/reference interface with archive-owned storage.
 * Original key reference services: Copyright (C) Red Hat, Inc.
 * Pointer tagging uses uintptr_t on LLP64 hosts. */
#ifndef _LINUX_KEY_H
#define _LINUX_KEY_H
#include "../../lib/crypto/userspace.h"
struct key_type;
struct sevenzip_trust_entry;
union key_payload {
    void *data[4];
};
struct key {
    const struct key_type *type;
    union key_payload payload;
    const struct sevenzip_trust_entry *entries;
    size_t count;
};
typedef struct __key_reference_with_attributes *key_ref_t;
static inline key_ref_t make_key_ref(const struct key *key, bool possession) {
    return (key_ref_t)((uintptr_t)key | possession);
}
static inline struct key *key_ref_to_ptr(const key_ref_t key_ref) {
    return (struct key *)((uintptr_t)key_ref & ~(uintptr_t)1);
}
extern const struct key_type key_type_asymmetric;
key_ref_t keyring_search(key_ref_t, const struct key_type *, const char *, bool);
void key_put(struct key *);
#endif
