// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "formats/ntfs/ntfscompat.h"

#define LZNT_CHUNK_SIZE 0x1000

static const size_t s_max_off[] = {
	0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000,
};

static inline size_t parse_pair(u16 pair, size_t *offset, size_t index)
{
	*offset = 1 + (pair >> (12 - index));
	return 3 + (pair & ((1 << (12 - index)) - 1));
}


static inline ssize_t decompress_chunk(u8 *unc, u8 *unc_end, const u8 *cmpr,
				       const u8 *cmpr_end)
{
	u8 *up = unc;
	u8 ch = *cmpr++;
	size_t bit = 0;
	size_t index = 0;
	u16 pair;
	size_t offset, length;

	/* Do decompression until pointers are inside range. */
	while (up < unc_end && cmpr < cmpr_end) {
		// return err if more than LZNT_CHUNK_SIZE bytes are written
		if (up - unc > LZNT_CHUNK_SIZE)
			return -EINVAL;
		/* Correct index */
		while (index < ARRAY_SIZE(s_max_off) - 1 &&
		       unc + s_max_off[index] < up) {
			index += 1;
		}

		/* Check the current flag for zero. */
		if (!(ch & (1 << bit))) {
			/* Just copy byte. */
			*up++ = *cmpr++;
			goto next;
		}

		/* Check for boundary. */
		if (cmpr + 1 >= cmpr_end)
			return -EINVAL;

		/* Read a short from little endian stream. */
		pair = cmpr[1];
		pair <<= 8;
		pair |= cmpr[0];

		cmpr += 2;

		/* Translate packed information into offset and length. */
		length = parse_pair(pair, &offset, index);

		/* Check offset for boundary. */
		if (unc + offset > up)
			return -EINVAL;

		/* Truncate the length if necessary. */
		if (up + length >= unc_end)
			length = unc_end - up;

		/* Now we copy bytes. This is the heart of LZ algorithm. */
		for (; length > 0; length--, up++)
			*up = *(up - offset);

next:
		/* Advance flag bit value. */
		bit = (bit + 1) & 7;

		if (!bit) {
			if (cmpr >= cmpr_end)
				break;

			ch = *cmpr++;
		}
	}

	/* Return the size of uncompressed data. */
	return up - unc;
}


ssize_t decompress_lznt(const void *cmpr, size_t cmpr_size, void *unc,
			size_t unc_size)
{
	const u8 *cmpr_chunk = cmpr;
	const u8 *cmpr_end = cmpr_chunk + cmpr_size;
	u8 *unc_chunk = unc;
	u8 *unc_end = unc_chunk + unc_size;
	u16 chunk_hdr;

	if (cmpr_size < sizeof(short))
		return -EINVAL;

	/* Read chunk header. */
	chunk_hdr = cmpr_chunk[1];
	chunk_hdr <<= 8;
	chunk_hdr |= cmpr_chunk[0];

	/* Loop through decompressing chunks. */
	for (;;) {
		size_t chunk_size_saved;
		size_t unc_use;
		size_t cmpr_use = 3 + (chunk_hdr & (LZNT_CHUNK_SIZE - 1));

		/* Check that the chunk actually fits the supplied buffer. */
		if (cmpr_chunk + cmpr_use > cmpr_end)
			return -EINVAL;

		/* First make sure the chunk contains compressed data. */
		if (chunk_hdr & 0x8000) {
			/* Decompress a chunk and return if we get an error. */
			ssize_t err =
				decompress_chunk(unc_chunk, unc_end,
						 cmpr_chunk + sizeof(chunk_hdr),
						 cmpr_chunk + cmpr_use);
			if (err < 0)
				return err;
			unc_use = err;
		} else {
			/* This chunk does not contain compressed data. */
			unc_use = unc_chunk + LZNT_CHUNK_SIZE > unc_end ?
					  unc_end - unc_chunk :
					  LZNT_CHUNK_SIZE;

			if (cmpr_chunk + sizeof(chunk_hdr) + unc_use >
			    cmpr_end) {
				return -EINVAL;
			}

			memcpy(unc_chunk, cmpr_chunk + sizeof(chunk_hdr),
			       unc_use);
		}

		/* Advance pointers. */
		cmpr_chunk += cmpr_use;
		unc_chunk += unc_use;

		/* Check for the end of unc buffer. */
		if (unc_chunk >= unc_end)
			break;

		/* Proceed the next chunk. */
		if (cmpr_chunk > cmpr_end - 2)
			break;

		chunk_size_saved = LZNT_CHUNK_SIZE;

		/* Read chunk header. */
		chunk_hdr = cmpr_chunk[1];
		chunk_hdr <<= 8;
		chunk_hdr |= cmpr_chunk[0];

		if (!chunk_hdr)
			break;

		/* Check the size of unc buffer. */
		if (unc_use < chunk_size_saved) {
			size_t t1 = chunk_size_saved - unc_use;
			u8 *t2 = unc_chunk + t1;

			/* 'Zero' memory. */
			if (t2 >= unc_end)
				break;

			memset(unc_chunk, 0, t1);
			unc_chunk = t2;
		}
	}

	/* Check compression boundary. */
	if (cmpr_chunk > cmpr_end)
		return -EINVAL;

	/*
	 * The unc size is just a difference between current
	 * pointer and original one.
	 */
	return PtrOffset(unc, unc_chunk);
}
