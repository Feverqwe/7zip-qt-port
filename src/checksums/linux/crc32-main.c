// SPDX-License-Identifier: GPL-2.0-only
/*
 * Aug 8, 2011 Bob Pearson with help from Joakim Tjernlund and George Spelvin
 * cleaned up code to current version of sparse and added the slicing-by-8
 * algorithm to the closely similar existing slicing-by-4 algorithm.
 *
 * Oct 15, 2000 Matt Domsch <Matt_Domsch@dell.com>
 * Nicer crc32 functions/docs submitted by linux@horizon.com.  Thanks!
 * Code was from the public domain, copyright abandoned.  Code was
 * subsequently included in the kernel, thus was re-licensed under the
 * GNU GPL v2.
 *
 * Oct 12, 2000 Matt Domsch <Matt_Domsch@dell.com>
 * Same crc32 function was used in 5 other places in the kernel.
 * I made one version, and deleted the others.
 * There are various incantations of crc32().  Some use a seed of 0 or ~0.
 * Some xor at the end with ~0.  The generic crc32() function takes
 * seed as an argument, and doesn't xor at the end.  Then individual
 * users can do whatever they need.
 *   drivers/net/smc9194.c uses seed ~0, doesn't xor with ~0.
 *   fs/jffs2 uses seed 0, doesn't xor with ~0.
 *   fs/partitions/efi.c uses seed ~0, xor's with ~0.
 */

/* see: Documentation/staging/crc32.rst for a description of algorithms */

#include <stdint.h>
#include <stddef.h>
#include <lzma.h>
#include "formats/vhdx/VhdxCrc.h"
typedef uint8_t u8;
typedef uint32_t u32;
#define __maybe_unused
#define ____cacheline_aligned
#define EXPORT_SYMBOL(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)

#include "checksums/linux/crc32table.h"

static inline u32 __maybe_unused
crc32_be_base(u32 crc, const u8 *p, size_t len)
{
	while (len--)
		crc = (crc << 8) ^ crc32table_be[(crc >> 24) ^ *p++];
	return crc;
}

#ifdef CONFIG_CRC32_ARCH
#include "crc32.h" /* $(SRCARCH)/crc32.h */

u32 crc32_optimizations(void)
{
	return crc32_optimizations_arch();
}
EXPORT_SYMBOL(crc32_optimizations);
#else
#define crc32_be_arch crc32_be_base
#endif

u32 crc32_le(u32 crc, const void *p, size_t len)
{
	/* Linux exposes raw state; liblzma accepts and returns complemented state. */
	return len ? ~lzma_crc32(p, len, ~crc) : crc;
}
EXPORT_SYMBOL(crc32_le);

u32 crc32_be(u32 crc, const void *p, size_t len)
{
	return crc32_be_arch(crc, p, len);
}
EXPORT_SYMBOL(crc32_be);

u32 crc32c(u32 crc, const void *p, size_t len)
{
	return sevenzip_crc32c_update(crc, p, len);
}
EXPORT_SYMBOL(crc32c);

#ifdef crc32_mod_init_arch
static int __init crc32_mod_init(void)
{
	crc32_mod_init_arch();
	return 0;
}
subsys_initcall(crc32_mod_init);

static void __exit crc32_mod_exit(void)
{
}
module_exit(crc32_mod_exit);
#endif

MODULE_DESCRIPTION("CRC32 library functions");
MODULE_LICENSE("GPL");
