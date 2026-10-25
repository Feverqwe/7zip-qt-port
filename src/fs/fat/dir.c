// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/fs/fat/dir.c
 *
 *  directory handling functions for fat-based filesystems
 *
 *  Written 1992,1993 by Werner Almesberger
 *
 *  Hidden files 1995 by Albert Cahalan <albert@ccs.neu.edu> <adc@coe.neu.edu>
 *
 *  VFAT extensions by Gordon Chaffee <chaffee@plateau.cs.berkeley.edu>
 *  Merged with msdos fs by Henrik Storner <storner@osiris.ping.dk>
 *  Rewritten for constant inumbers. Plugged buffer overrun in readdir(). AV
 *  Short name translation 1999, 2001 by Wolfram Pienkoss <wp@bszh.de>
 */

#include "fat.h"
#include "../nls/system.h"

/*
 * Maximum buffer size of short name.
 * [(MSDOS_NAME + '.') * max one char + nul]
 * For msdos style, ['.' (hidden) + MSDOS_NAME + '.' + nul]
 */
#define FAT_MAX_SHORT_SIZE	((MSDOS_NAME + 1) * NLS_MAX_CHARSET_SIZE + 1)
/*
 * Maximum buffer size of unicode chars from slots.
 * [(max longname slots * 13 (size in a slot) + nul) * sizeof(fat_wchar_t)]
 */
#define FAT_MAX_UNI_CHARS	((MSDOS_SLOTS - 1) * 13 + 1)
#define FAT_MAX_UNI_SIZE	(FAT_MAX_UNI_CHARS * sizeof(fat_wchar_t))

static inline unsigned char fat_tolower(unsigned char c)
{
	return ((c >= 'A') && (c <= 'Z')) ? c+32 : c;
}

static int uni16_to_x8(struct super_block *sb, unsigned char *ascii,
		       const fat_wchar_t *uni, int len, struct nls_table *nls)
{
	int uni_xlate = MSDOS_SB(sb)->options.unicode_xlate;
	const fat_wchar_t *ip;
	fat_wchar_t ec;
	unsigned char *op;
	int charlen;

	ip = uni;
	op = ascii;

	while (*ip && ((len - NLS_MAX_CHARSET_SIZE) > 0)) {
		ec = *ip++;
		charlen = nls->uni2char(ec, op, NLS_MAX_CHARSET_SIZE);
		if (charlen > 0) {
			op += charlen;
			len -= charlen;
		} else {
			if (uni_xlate == 1) {
				*op++ = ':';
				op = hex_byte_pack(op, ec >> 8);
				op = hex_byte_pack(op, ec);
				len -= 5;
			} else {
				*op++ = '?';
				len--;
			}
		}
	}

	if (unlikely(*ip)) {
		fat_msg(sb, KERN_WARNING,
			"filename was truncated while converting.");
	}

	*op = 0;
	return op - ascii;
}

static inline int fat_uni_to_x8(struct super_block *sb, const fat_wchar_t *uni,
				unsigned char *buf, int size)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	if (sbi->options.utf8)
		return utf16s_to_utf8s(uni, FAT_MAX_UNI_CHARS,
				UTF16_HOST_ENDIAN, buf, size);
	else
		return uni16_to_x8(sb, buf, uni, size, sbi->nls_io);
}

static inline int
fat_short2uni(struct nls_table *t, unsigned char *c, int clen, fat_wchar_t *uni)
{
	int charlen;

	charlen = t->char2uni(c, clen, uni);
	if (charlen < 0) {
		*uni = 0x003f;	/* a question mark */
		charlen = 1;
	}
	return charlen;
}

static inline int
fat_short2lower_uni(struct nls_table *t, unsigned char *c,
		    int clen, fat_wchar_t *uni)
{
	int charlen;
	fat_wchar_t wc;

	charlen = t->char2uni(c, clen, &wc);
	if (charlen < 0) {
		*uni = 0x003f;	/* a question mark */
		charlen = 1;
	} else if (charlen <= 1) {
		unsigned char nc = fat_cp437_lower(*c);

		if (!nc)
			nc = *c;

		charlen = t->char2uni(&nc, 1, uni);
		if (charlen < 0) {
			*uni = 0x003f;	/* a question mark */
			charlen = 1;
		}
	} else
		*uni = wc;

	return charlen;
}

static inline int
fat_shortname2uni(struct nls_table *nls, unsigned char *buf, int buf_size,
		  fat_wchar_t *uni_buf, unsigned short opt, int lower)
{
	int len = 0;

	if (opt & VFAT_SFN_DISPLAY_LOWER)
		len =  fat_short2lower_uni(nls, buf, buf_size, uni_buf);
	else if (opt & VFAT_SFN_DISPLAY_WIN95)
		len = fat_short2uni(nls, buf, buf_size, uni_buf);
	else if (opt & VFAT_SFN_DISPLAY_WINNT) {
		if (lower)
			len = fat_short2lower_uni(nls, buf, buf_size, uni_buf);
		else
			len = fat_short2uni(nls, buf, buf_size, uni_buf);
	} else
		len = fat_short2uni(nls, buf, buf_size, uni_buf);

	return len;
}

static int fat_parse_short(struct super_block *sb,
			   const struct msdos_dir_entry *de,
			   unsigned char *name, int dot_hidden)
{
	const struct msdos_sb_info *sbi = MSDOS_SB(sb);
	int isvfat = sbi->options.isvfat;
	int nocase = sbi->options.nocase;
	unsigned short opt_shortname = sbi->options.shortname;
	struct nls_table *nls_disk = sbi->nls_disk;
	fat_wchar_t uni_name[14];
	unsigned char c, work[MSDOS_NAME];
	unsigned char *ptname = name;
	int chi, chl, i, j, k;
	int dotoffset = 0;
	int name_len = 0, uni_len = 0;

	if (!isvfat && dot_hidden && (de->attr & ATTR_HIDDEN)) {
		*ptname++ = '.';
		dotoffset = 1;
	}

	memcpy(work, de->name, sizeof(work));
	/* For an explanation of the special treatment of 0x05 in
	 * filenames, see msdos_format_name in namei_msdos.c
	 */
	if (work[0] == 0x05)
		work[0] = 0xE5;

	/* Filename */
	for (i = 0, j = 0; i < 8;) {
		c = work[i];
		if (!c)
			break;
		chl = fat_shortname2uni(nls_disk, &work[i], 8 - i,
					&uni_name[j++], opt_shortname,
					de->lcase & CASE_LOWER_BASE);
		if (chl <= 1) {
			if (!isvfat)
				ptname[i] = nocase ? c : fat_tolower(c);
			i++;
			if (c != ' ') {
				name_len = i;
				uni_len  = j;
			}
		} else {
			uni_len = j;
			if (isvfat)
				i += min(chl, 8-i);
			else {
				for (chi = 0; chi < chl && i < 8; chi++, i++)
					ptname[i] = work[i];
			}
			if (chl)
				name_len = i;
		}
	}

	i = name_len;
	j = uni_len;
	fat_short2uni(nls_disk, (unsigned char *)".", 1, &uni_name[j++]);
	if (!isvfat)
		ptname[i] = '.';
	i++;

	/* Extension */
	for (k = 8; k < MSDOS_NAME;) {
		c = work[k];
		if (!c)
			break;
		chl = fat_shortname2uni(nls_disk, &work[k], MSDOS_NAME - k,
					&uni_name[j++], opt_shortname,
					de->lcase & CASE_LOWER_EXT);
		if (chl <= 1) {
			k++;
			if (!isvfat)
				ptname[i] = nocase ? c : fat_tolower(c);
			i++;
			if (c != ' ') {
				name_len = i;
				uni_len  = j;
			}
		} else {
			uni_len = j;
			if (isvfat) {
				int offset = min(chl, MSDOS_NAME-k);
				k += offset;
				i += offset;
			} else {
				for (chi = 0; chi < chl && k < MSDOS_NAME;
				     chi++, i++, k++) {
						ptname[i] = work[k];
				}
			}
			if (chl)
				name_len = i;
		}
	}

	if (name_len > 0) {
		name_len += dotoffset;

		if (sbi->options.isvfat) {
			uni_name[uni_len] = 0x0000;
			name_len = fat_uni_to_x8(sb, uni_name, name,
						 FAT_MAX_SHORT_SIZE);
		}
	}

	return name_len;
}

int fat_linux_short_name(const unsigned char alias[11], unsigned char flags, char output[73])
{
	struct msdos_sb_info sbi = {0};
	struct super_block sb = {0};
	struct msdos_dir_entry de = {0};
	sbi.options.isvfat = 1;
	sbi.options.utf8 = 1;
	sbi.options.shortname = VFAT_SFN_DISPLAY_WINNT;
	sbi.nls_disk = fat_cp437_table();
	if (!sbi.nls_disk)
		return -EINVAL;
	sb.s_fs_info = &sbi;
	memcpy(de.name, alias, MSDOS_NAME);
	de.lcase = flags;
	return fat_parse_short(&sb, &de, (unsigned char *)output, 0);
}
