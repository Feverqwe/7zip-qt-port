/* SPDX-License-Identifier: GPL-2.0 */
/* Selected internal dev_t read definitions; encoders/VFS printing omitted.
 * Kernel dev_t/u32 use portable uint32_t at this userspace boundary. */
#ifndef SEVENZIP_LINUX_KDEV_T_H
#define SEVENZIP_LINUX_KDEV_T_H
#include <stdint.h>
#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)
#define MAJOR(dev) ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev) ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi) (((ma) << MINORBITS) | (mi))

static inline uint32_t new_decode_dev(uint32_t dev)
{
	unsigned major = (dev & 0xfff00) >> 8;
	unsigned minor = (dev & 0xff) | ((dev >> 12) & 0xfff00);
	return MKDEV(major, minor);
}
#endif
