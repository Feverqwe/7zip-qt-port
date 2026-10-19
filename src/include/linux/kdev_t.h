/* SPDX-License-Identifier: GPL-2.0 */
/* Selected internal dev_t read definitions; encoders/VFS printing omitted. */
#ifndef SEVENZIP_LINUX_KDEV_T_H
#define SEVENZIP_LINUX_KDEV_T_H
#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)
#define MAJOR(dev) ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev) ((unsigned int) ((dev) & MINORMASK))
#endif
