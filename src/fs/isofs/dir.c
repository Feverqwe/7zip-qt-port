// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/isofs/dir.c
 *
 *  (C) 1992, 1993, 1994  Eric Youngdale Modified for ISO 9660 filesystem.
 *
 *  (C) 1991  Linus Torvalds - minix filesystem
 *
 *  Steve Beynon		       : Missing last directory entries fixed
 *  (stephen@askone.demon.co.uk)      : 21st June 1996
 *
 *  isofs directory handling functions
 */
#include "namecompat.h"

/* Read-service adapter: reconstruct the already bounded directory record for
 * the original Linux naming functions. Extra zero bytes bound their lookahead. */
int sevenzip_iso_name(const unsigned char *name, size_t name_size,
                      const unsigned char *system_use, size_t system_size,
                      unsigned char flags, int joliet, char mapping, int charset,
                      char output[1024])
{
    unsigned char storage[512] = { 0 };
    unsigned char converted[1024];
    struct iso_directory_record *de = (struct iso_directory_record *)storage;
    struct isofs_name_super super = { charset ? fat_cp437_table() : NULL };
    struct inode node = { &super };
    size_t std = sizeof(*de) + name_size;
    int length;
    std += std & 1;
    if (name_size > 222 || std > 255 || system_size > 255 - std)
        return -EIO;
    de->length[0] = (u8)(std + system_size);
    de->name_len[0] = (u8)name_size;
    de->flags[0] = flags;
    if (name_size)
        memcpy(de->name, name, name_size);
    if (system_size)
        memcpy(storage + std, system_use, system_size);
    if (joliet) {
        length = get_joliet_filename(de, converted, &node);
        if (charset) {
            fat_wchar_t unicode[256];
            int i;
            for (i = 0; i < length; ++i)
                if (super.s_nls_iocharset->char2uni(converted + i, 1, unicode + i) < 0)
                    unicode[i] = '?';
            length = utf16s_to_utf8s(unicode, length, UTF16_HOST_ENDIAN, (u8 *)output, 1023);
        } else if (length)
            memcpy(output, converted, (size_t)length);
    } else if (mapping == 'a') {
        length = get_acorn_filename(de, output, &node);
    } else if (mapping == 'n') {
        length = isofs_name_translate(de, output, &node);
    } else {
        length = (int)name_size;
        if (name_size)
            memcpy(output, name, name_size);
    }
    output[length] = 0;
    return length;
}

int isofs_name_translate(struct iso_directory_record *de, char *new, struct inode *inode)
{
	char * old = de->name;
	int len = de->name_len[0];
	int i;

	for (i = 0; i < len; i++) {
		unsigned char c = old[i];
		if (!c)
			break;

		if (c >= 'A' && c <= 'Z')
			c |= 0x20;	/* lower case */

		/* Drop trailing '.;1' (ISO 9660:1988 7.5.1 requires period) */
		if (c == '.' && i == len - 3 && old[i + 1] == ';' && old[i + 2] == '1')
			break;

		/* Drop trailing ';1' */
		if (c == ';' && i == len - 2 && old[i + 1] == '1')
			break;

		/* Convert remaining ';' to '.' */
		/* Also '/' to '.' (broken Acorn-generated ISO9660 images) */
		if (c == ';' || c == '/')
			c = '.';

		new[i] = c;
	}
	return i;
}

/* Acorn extensions written by Matthew Wilcox <willy@infradead.org> 1998 */
int get_acorn_filename(struct iso_directory_record *de,
			    char *retname, struct inode *inode)
{
	int std;
	unsigned char *chr;
	int retnamlen = isofs_name_translate(de, retname, inode);

	if (retnamlen == 0)
		return 0;
	std = sizeof(struct iso_directory_record) + de->name_len[0];
	if (std & 1)
		std++;
	if (de->length[0] - std != 32)
		return retnamlen;
	chr = ((unsigned char *) de) + std;
	if (strncmp((const char *)chr, "ARCHIMEDES", 10))
		return retnamlen;
	if ((*retname == '_') && ((chr[19] & 1) == 1))
		*retname = '!';
	if (((de->flags[0] & 2) == 0) && (chr[13] == 0xff)
		&& ((chr[12] & 0xf0) == 0xf0)) {
		retname[retnamlen] = ',';
		sprintf(retname+retnamlen+1, "%3.3x",
			((chr[12] & 0xf) << 8) | chr[11]);
		retnamlen += 4;
	}
	return retnamlen;
}
