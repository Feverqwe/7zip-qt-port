// SPDX-License-Identifier: GPL-2.0-only
/* Selected Linux string duplication services for filesystem metadata. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef int gfp_t;
#define EXPORT_SYMBOL(name)
#ifndef __always_inline
#define __always_inline inline
#endif
#define kmalloc_track_caller(size, flags) ((void)(flags), malloc(size))

/**
 * __kmemdup_nul - Create a NUL-terminated string from @s, which might be unterminated.
 * @s: The data to copy
 * @len: The size of the data, not including the NUL terminator
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 *
 * Return: newly allocated copy of @s with NUL-termination or %NULL in
 * case of error
 */
static __always_inline char *__kmemdup_nul(const char *s, size_t len, gfp_t gfp)
{
	char *buf;

	/* Retain the archive bridge's allocation-overflow protection. */
	if (len == SIZE_MAX)
		return NULL;

	/* '+1' for the NUL terminator */
	buf = kmalloc_track_caller(len + 1, gfp);
	if (!buf)
		return NULL;

	memcpy(buf, s, len);
	/* Ensure the buf is always NUL-terminated, regardless of @s. */
	buf[len] = '\0';
	return buf;
}

/**
 * kstrndup - allocate space for and copy an existing string
 * @s: the string to duplicate
 * @max: read at most @max chars from @s
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 *
 * Note: Use kmemdup_nul() instead if the size is known exactly.
 *
 * Return: newly allocated copy of @s or %NULL in case of error
 */
char *kstrndup(const char *s, size_t max, gfp_t gfp)
{
	return s ? __kmemdup_nul(s, strnlen(s, max), gfp) : NULL;
}
EXPORT_SYMBOL(kstrndup);

/**
 * kmemdup_nul - Create a NUL-terminated string from unterminated data
 * @s: The data to stringify
 * @len: The size of the data
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 *
 * Return: newly allocated copy of @s with NUL-termination or %NULL in
 * case of error
 */
char *kmemdup_nul(const char *s, size_t len, gfp_t gfp)
{
	return s ? __kmemdup_nul(s, len, gfp) : NULL;
}
EXPORT_SYMBOL(kmemdup_nul);
