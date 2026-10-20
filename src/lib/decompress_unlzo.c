// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * LZO decompressor for the Linux kernel. Code borrowed from the lzo
 * implementation by Markus Franz Xaver Johannes Oberhumer.
 *
 * Linux kernel adaptation:
 * Copyright (C) 2009
 * Albin Tonnerre, Free Electrons <albin.tonnerre@free-electrons.com>
 *
 * Original code:
 * Copyright (C) 1996-2005 Markus Franz Xaver Johannes Oberhumer
 * All Rights Reserved.
 *
 * Markus F.X.J. Oberhumer
 * <markus@oberhumer.com>
 * http://www.oberhumer.com/opensource/lzop/
 */

/* Sevenzip userspace I/O adaptation; original wrapper flow retained. */
#include "framecompat.h"
#include "../include/linux/lzo.h"

static const unsigned char lzop_magic[] = {
	0x89, 0x4c, 0x5a, 0x4f, 0x00, 0x0d, 0x0a, 0x1a, 0x0a };

#define LZO_BLOCK_SIZE        (256*1024l)
#define HEADER_HAS_FILTER      0x00000800L
#define HEADER_SIZE_MIN       (9 + 5     + 4 + 8     + 1       + 4)
#define HEADER_SIZE_MAX       (9 + 7 + 1 + 8 + 8 + 4 + 1 + 255 + 4)

STATIC inline long INIT parse_header(u8 *input, long *skip, long in_len, u32 *flags_out)
{
	int l;
	u8 *parse = input;
	u8 *end = input + in_len;
	u16 version;
	u32 flags;

	/*
	 * Check that there's enough input to possibly have a valid header.
	 * Then it is possible to parse several fields until the minimum
	 * size may have been used.
	 */
	if (in_len < HEADER_SIZE_MIN)
		return 0;

	/* read magic: 9 first bits */
	for (l = 0; l < 9; l++) {
		if (*parse++ != lzop_magic[l])
			return 0;
	}
	/* get version (2bytes), skip library version (2),
	 * 'need to be extracted' version (2) and
	 * method (1) */
	version = get_unaligned_be16(parse);
	const unsigned method_pos = version >= 0x0940 ? 6 : 4;
	if (parse[method_pos] < 1 || parse[method_pos] > 3)
		return 0;
	parse += version >= 0x0940 ? 7 : 5;
	if (version >= 0x0940)
		parse++;
	flags = get_unaligned_be32(parse);
	/* Extra fields and nonidentity filters have no usable
	 * read implementation in the kernel wrapper. Never return shifted data. */
	if ((flags & 0x40) || ((flags & 2) && !(flags & 1)) ||
	    ((flags & 0x200) && !(flags & 0x100)))
		return 0;
	if ((flags & HEADER_HAS_FILTER) && get_unaligned_be32(parse + 4))
		return 0;
	if (flags & HEADER_HAS_FILTER)
		parse += 8; /* flags + filter info */
	else
		parse += 4; /* flags */

	/*
	 * At least mode, mtime_low, filename length, and checksum must
	 * be left to be parsed. If also mtime_high is present, it's OK
	 * because the next input buffer check is after reading the
	 * filename length.
	 */
	if (end - parse < 8 + 1 + 4)
		return 0;

	/* skip mode and mtime_low */
	parse += 8;
	if (version >= 0x0940)
		parse += 4;	/* skip mtime_high */

	l = *parse++;
	/* don't care about the file name, and skip checksum */
	if (end - parse < l + 4)
		return 0;
	if (sevenzip_frame_checksum(flags & 0x1000, input + 9,
				   (size_t)(parse + l - input - 9)) != get_unaligned_be32(parse + l))
		return 0;
	parse += l + 4;

	*skip = parse - input;
	*flags_out = flags;
	return 1;
}

int sevenzip_lzop_probe(unsigned char *input, size_t size)
{
	long skip;
	u32 flags;
	return size <= LONG_MAX && parse_header(input, &skip, (long)size, &flags);
}

int unlzo(struct sevenzip_linux_frame_io *io, uint64_t *posp)
{
	u8 *input = NULL, *output = NULL;
	long in_len = 0;
	const int fill = 1, flush = 1;
	if (!io || !io->fill || !io->flush || !io->error || !posp)
		return -1;
	io->allocation_failed = 0;

	u8 r = 0;
	long skip = 0;
	u32 src_len, dst_len;
	u32 flags, sums[4];
	size_t tmp;
	u8 *in_buf, *in_buf_save, *out_buf;
	int ret = -1;

	if (output) {
		out_buf = output;
	} else if (!flush) {
		error("NULL output pointer and no flush function provided");
		goto exit;
	} else {
		out_buf = malloc(LZO_BLOCK_SIZE);
		if (!out_buf) {
			error("Could not allocate output buffer");
			goto exit;
		}
	}

	if (input && fill) {
		error("Both input pointer and fill function provided, don't know what to do");
		goto exit_1;
	} else if (input) {
		in_buf = input;
	} else if (!fill) {
		error("NULL input pointer and missing fill function");
		goto exit_1;
	} else {
		in_buf = malloc(lzo1x_worst_compress(LZO_BLOCK_SIZE));
		if (!in_buf) {
			error("Could not allocate input buffer");
			goto exit_1;
		}
	}
	in_buf_save = in_buf;

	if (posp)
		*posp = 0;

	if (fill) {
		/*
		 * Start from in_buf + HEADER_SIZE_MAX to make it possible
		 * to use memcpy() to copy the unused data to the beginning
		 * of the buffer. This way memmove() isn't needed which
		 * is missing from pre-boot environments of most archs.
		 */
		in_buf += HEADER_SIZE_MAX;
		in_len = fill(in_buf, HEADER_SIZE_MAX);
	}

	if (!parse_header(in_buf, &skip, in_len, &flags)) {
		error("invalid header");
		goto exit_2;
	}
	in_buf += skip;
	in_len -= skip;

	if (fill) {
		/* Move the unused data to the beginning of the buffer. */
		memcpy(in_buf_save, in_buf, in_len);
		in_buf = in_buf_save;
	}

	if (posp)
		*posp = skip;

	for (;;) {
		/* read uncompressed block size */
		if (fill && in_len < 4) {
			skip = fill(in_buf + in_len, 4 - in_len);
			if (skip > 0)
				in_len += skip;
		}
		if (in_len < 4) {
			error("file corrupted");
			goto exit_2;
		}
		dst_len = get_unaligned_be32(in_buf);
		in_buf += 4;
		in_len -= 4;

		/* exit if last block */
		if (dst_len == 0) {
			if (posp)
				*posp += 4;
			break;
		}

		if (dst_len > LZO_BLOCK_SIZE) {
			error("dest len longer than block size");
			goto exit_2;
		}

		/* read compressed block size */
		if (fill && in_len < 4) {
			skip = fill(in_buf + in_len, 4 - in_len);
			if (skip > 0)
				in_len += skip;
		}
		if (in_len < 4) {
			error("file corrupted");
			goto exit_2;
		}
		src_len = get_unaligned_be32(in_buf);
		in_buf += 4;
		in_len -= 4;

		if (src_len <= 0 || src_len > dst_len) {
			error("file corrupted");
			goto exit_2;
		}
		/* lzop checksum ordering is shared by stored and compressed blocks.
		 * The kernel's fixed four-byte skip is only valid for one D checksum. */
		for (unsigned i = 0; i < 4; ++i) {
			const u32 mask = i == 0 ? 1 : i == 1 ? 0x100 : i == 2 ? 2 : 0x200;
			sums[i] = 0;
			if (!(flags & mask) || (i >= 2 && src_len == dst_len))
				continue;
			if (fill && in_len < 4) {
				skip = fill(in_buf + in_len, 4 - in_len);
				if (skip > 0) in_len += skip;
			}
			if (in_len < 4) {
				error("file corrupted");
				goto exit_2;
			}
			sums[i] = get_unaligned_be32(in_buf);
			in_buf += 4;
			in_len -= 4;
		}

		/* decompress */
		if (fill && in_len < src_len) {
			skip = fill(in_buf + in_len, src_len - in_len);
			if (skip > 0)
				in_len += skip;
		}
		if (in_len < src_len) {
			error("file corrupted");
			goto exit_2;
		}
		if (src_len != dst_len &&
		    (((flags & 2) && sums[2] != sevenzip_frame_checksum(0, in_buf, src_len)) ||
		     ((flags & 0x200) && sums[3] != sevenzip_frame_checksum(1, in_buf, src_len)))) {
			error("Compressed checksum violation");
			goto exit_2;
		}
		tmp = dst_len;

		/* When the input data is not compressed at all,
		 * lzo1x_decompress_safe will fail, so call memcpy()
		 * instead */
		if (unlikely(dst_len == src_len))
			memcpy(out_buf, in_buf, src_len);
		else {
			r = lzo1x_decompress_safe((u8 *) in_buf, src_len,
						out_buf, &tmp);

			if (r != LZO_E_OK || dst_len != tmp) {
				error("Compressed data violation");
				goto exit_2;
			}
		}

		if (((flags & 1) && sums[0] != sevenzip_frame_checksum(0, out_buf, dst_len)) ||
		    ((flags & 0x100) && sums[1] != sevenzip_frame_checksum(1, out_buf, dst_len))) {
			error("Output checksum violation");
			goto exit_2;
		}
		if (flush && flush(out_buf, dst_len) != dst_len)
			goto exit_2;
		if (output)
			out_buf += dst_len;
		if (posp)
			*posp += src_len + 8 + ((flags & 1) ? 4 : 0) + ((flags & 0x100) ? 4 : 0) +
				(src_len < dst_len ? (((flags & 2) ? 4 : 0) + ((flags & 0x200) ? 4 : 0)) : 0);

		in_buf += src_len;
		in_len -= src_len;
		if (fill) {
			/*
			 * If there happens to still be unused data left in
			 * in_buf, move it to the beginning of the buffer.
			 * Use a loop to avoid memmove() dependency.
			 */
			if (in_len > 0)
				for (skip = 0; skip < in_len; ++skip)
					in_buf_save[skip] = in_buf[skip];
			in_buf = in_buf_save;
		}
	}

	ret = 0;
exit_2:
	if (!input)
		free(in_buf_save);
exit_1:
	if (!output)
		free(out_buf);
exit:
	return ret;
}

#ifdef PREBOOT
STATIC int INIT __decompress(unsigned char *buf, long len,
			   long (*fill)(void*, unsigned long),
			   long (*flush)(void*, unsigned long),
			   unsigned char *out_buf, long olen,
			   long *pos,
			   void (*error)(char *x))
{
	return unlzo(buf, len, fill, flush, out_buf, pos, error);
}
#endif
