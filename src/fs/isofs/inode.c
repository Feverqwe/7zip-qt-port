// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/fs/isofs/inode.c
 *
 *  (C) 1991  Linus Torvalds - minix filesystem
 *      1992, 1993, 1994  Eric Youngdale Modified for ISO 9660 filesystem.
 *      1994  Eberhard Mönkeberg - multi session handling.
 *      1995  Mark Dobie - allow mounting of some weird VideoCDs and PhotoCDs.
 *	1997  Gordon Chaffee - Joliet CDs
 *	1998  Eric Lammerts - ISO 9660 Level 3
 *	2004  Paul Serice - Inode Support pushed out from 4GB to 128GB
 *	2004  Paul Serice - NFS Export Operations
 */

#include "isofs.h"

static int rootdir_empty(struct super_block *sb, unsigned long block)
{
	int offset = 0, files = 0, de_len;
	sevenzip_iso_disk_record *de;
	struct buffer_head *bh;

	bh = sb_bread(sb, block);
	if (!bh)
		return 1;
	while (files < 3) {
		if ((unsigned)offset >= sb->block_size)
			break;
		de = (sevenzip_iso_disk_record *) (bh->b_data + offset);
		de_len = *(unsigned char *) de;
		if (de_len == 0)
			break;
		files++;
		offset += de_len;
	}
	brelse(bh);
	return files < 3;
}

/* isofs_fill_super root-choice portion. The archive caller supplies the result
 * of the existing bounded record reader instead of creating a VFS root inode.
 * Return primary (0), Joliet (1), or the root read error. */
int isofs_fill_super(struct sevenzip_iso_state *state, int root_error,
                     int joliet_level, int roots_differ, uint32_t root_block)
{
    struct super_block super;
    struct super_block *s = &super;
    struct super_block *sbi = s;
    memset(s, 0, sizeof(*s));
    s->state = state;
    s->s_rock = state->rock;
    s->s_nzones = state->volume_blocks;
    s->block_size = state->block_size;
    if (root_error) {
        if (joliet_level && roots_differ) {
            sbi->s_rock = 0;
        } else {
            return root_error;
        }
    }
    if (sbi->s_rock == 1 && joliet_level && rootdir_empty(s, root_block))
        sbi->s_rock = 0;
    if (sbi->s_rock == 1) {
        joliet_level = 0;
    } else if (joliet_level) {
        sbi->s_rock = 0;
    }
    state->rock = sbi->s_rock;
    return joliet_level ? 1 : 0;
}
