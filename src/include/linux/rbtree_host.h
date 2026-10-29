/* SPDX-License-Identifier: GPL-2.0-only */
/* Owned, unpublished trees need no kernel RCU or volatile access services. */
#ifndef SEVENZIP_RBTREE_HOST_H
#define SEVENZIP_RBTREE_HOST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifndef __always_inline
#define __always_inline inline
#endif
#ifndef likely
#define likely(x) (x)
#define unlikely(x) (x)
#endif
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x, value) ((x) = (value))
#define rcu_assign_pointer(x, value) ((x) = (value))
#define rcu_dereference_raw(x) (x)
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#define EXPORT_SYMBOL(name)
#define EXPORT_SYMBOL_GPL(name)
#endif
