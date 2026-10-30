/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Definitions for key type implementations
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#ifndef _LINUX_KEY_TYPE_H
#define _LINUX_KEY_TYPE_H
#include <linux/key.h>

struct key_match_data {
    /* Comparison function, defaults to exact description match, but can be
     * overridden by type->match_preparse().  Should return true if a match
     * is found and false if not.
     */
    bool (*cmp)(const struct key *key, const struct key_match_data *match_data);

    const void *raw_data;                    /* Raw match data */
    void *preparsed;                         /* For ->match_preparse() to stash stuff */
    unsigned lookup_type;                    /* Type of lookup for this search. */
#define KEYRING_SEARCH_LOOKUP_DIRECT 0x0000  /* Direct lookup by description. */
#define KEYRING_SEARCH_LOOKUP_ITERATE 0x0001 /* Iterative search. */
};

/* Selected immutable matching services; no kernel registration or keyctl. */
struct key_type {
    const char *name;
    int (*match_preparse)(struct key_match_data *);
    void (*match_free)(struct key_match_data *);
};
#endif
