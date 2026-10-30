/* SPDX-License-Identifier: GPL-2.0-only */
/* Sized zeroed allocations and sensitive-data release for kernel API callers. */
#include "crypto/modes/skciphercompat.h"

union allocation_header {
	size_t size;
	u64 alignment;
};
void *sevenzip_crypto_zalloc(size_t size)
{
	if (size > SIZE_MAX - sizeof(union allocation_header)) return NULL;
	union allocation_header *header = calloc(1, sizeof(*header) + size);
	if (!header) return NULL;
	header->size = size;
	return header + 1;
}
void sevenzip_crypto_free_sensitive(void *data)
{
	if (!data) return;
	union allocation_header *header = (union allocation_header *)data - 1;
	size_t size = header->size;
	memzero_explicit(header, sizeof(*header) + size);
	free(header);
}
