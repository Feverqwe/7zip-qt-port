// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hash Info: Hash algorithms information
 *
 * Copyright (c) 2013 Dmitry Kasatkin <d.kasatkin@samsung.com>
 */

#include <crypto/hash_info.h>
#include "crypto/primitives/userspace.h"

const char *const hash_algo_name[HASH_ALGO__LAST] = {
	"md4",
	"md5",
	"sha1",
	"rmd160",
	"sha256",
	"sha384",
	"sha512",
	"sha224",
	"rmd128",
	"rmd256",
	"rmd320",
	"wp256",
	"wp384",
	"wp512",
	"tgr128",
	"tgr160",
	"tgr192",
	"sm3",
	"streebog256",
	"streebog512",
	"sha3-256",
	"sha3-384",
	"sha3-512",
};
EXPORT_SYMBOL_GPL(hash_algo_name);
