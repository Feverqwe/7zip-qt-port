/* SPDX-License-Identifier: GPL-2.0-or-later */
/* keyctl kernel bits
 *
 * Copyright (C) 2016 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#ifndef __LINUX_KEYCTL_H
#define __LINUX_KEYCTL_H

enum kernel_pkey_operation {
	kernel_pkey_encrypt,
	kernel_pkey_decrypt,
	kernel_pkey_sign,
	kernel_pkey_verify,
};
#endif
