/* SPDX-License-Identifier: GPL-2.0-only */
/* Explicit archive trust boundary and original CONFIG_SECURITY=n notification
 * service. No kernel credentials, system key store or LSM is accessed. */
#ifndef SEVENZIP_FSVERITY_SIGNATURE_COMPAT_H
#define SEVENZIP_FSVERITY_SIGNATURE_COMPAT_H
#include "../../asymmetrictrust.h"
#ifndef ENOKEY
#define ENOKEY 126
#endif
#ifndef EKEYREJECTED
#define EKEYREJECTED 129
#endif
enum lsm_integrity_type {
    LSM_INT_DMVERITY_SIG_VALID,
    LSM_INT_DMVERITY_ROOTHASH,
    LSM_INT_FSVERITY_BUILTINSIG_VALID,
};

static inline int security_inode_setintegrity(const struct inode *inode,
                                              enum lsm_integrity_type type, const void *value,
                                              size_t size) {
    return 0;
}

#endif
