// SPDX-License-Identifier: GPL-2.0-or-later
/* bit search implementation
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * Copyright (C) 2008 IBM Corporation
 * 'find_last_bit' is written by Rusty Russell <rusty@rustcorp.com.au>
 * (Inspired by David Howell's find_next_bit implementation)
 *
 * Rewritten by Yury Norov <yury.norov@gmail.com> to decrease
 * size and improve performance, 2015.
 */

#include "../include/asm-generic/bitops/__ffs.h"
#define unlikely(value) (value)
#define min(a, b) ((a) < (b) ? (a) : (b))
#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) % BITS_PER_LONG))

unsigned long _find_next_zero_bit(const unsigned long *addr, unsigned long nbits,
				 unsigned long start)
{
	unsigned long mask, idx, tmp, sz = nbits, __start = start;

	if (unlikely(__start >= sz))
		goto out;

	mask = BITMAP_FIRST_WORD_MASK(__start);
	idx = __start / BITS_PER_LONG;

	for (tmp = ~addr[idx] & mask; !tmp; tmp = ~addr[idx]) {
		if ((idx + 1) * BITS_PER_LONG >= sz)
			goto out;
		idx++;
	}

	sz = min(idx * BITS_PER_LONG + __ffs(tmp), sz);
out:
	return sz;
}

/* FIND_NEXT_BIT(addr[idx], ...), expanded for portable C11 as above. */
unsigned long _find_next_bit(const unsigned long *addr, unsigned long nbits,
			     unsigned long start)
{
	unsigned long mask, idx, tmp, sz = nbits, __start = start;

	if (unlikely(__start >= sz))
		goto out;

	mask = BITMAP_FIRST_WORD_MASK(__start);
	idx = __start / BITS_PER_LONG;

	for (tmp = addr[idx] & mask; !tmp; tmp = addr[idx]) {
		if ((idx + 1) * BITS_PER_LONG >= sz)
			goto out;
		idx++;
	}

	sz = min(idx * BITS_PER_LONG + __ffs(tmp), sz);
out:
	return sz;
}
