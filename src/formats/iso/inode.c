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

#include "formats/iso/isofs.h"

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

/* isofs_read_level3_size: retain the kernel scan and expose bounded section
 * records to the archive reader instead of VFS page mapping. */
int isofs_read_level3_size(struct inode *inode, struct sevenzip_iso_section *sections,
                           unsigned *count)
{
	unsigned long bufsize = ISOFS_BUFFER_SIZE(inode);
	struct buffer_head *bh = NULL;
	unsigned long block, offset, block_saved, offset_saved;
	int i = 0;
	int more_entries = 0;
	sevenzip_iso_disk_record *tmpde = NULL;
	struct inode *ei = ISOFS_I(inode);

	*count = 0;
	inode->i_size = 0;

	/* The first 16 blocks are reserved as the System Area.  Thus,
	 * no inodes can appear in block 0.  We use this to flag that
	 * this is the last section. */
	ei->i_next_section_block = 0;
	ei->i_next_section_offset = 0;

	block = ei->i_iget5_block;
	offset = ei->i_iget5_offset;

	do {
		sevenzip_iso_disk_record *de;
		unsigned int de_len;

		if (!bh) {
			bh = sb_bread(inode->i_sb, block);
			if (!bh)
				goto out_noread;
		}
		de = (sevenzip_iso_disk_record *) (bh->b_data + offset);
		de_len = *(unsigned char *) de;

		if (de_len == 0) {
			brelse(bh);
			bh = NULL;
			++block;
			offset = 0;
			continue;
		}

		if (de_len < sizeof(*de))
			goto out_corrupt;

		block_saved = block;
		offset_saved = offset;
		offset += de_len;

		/* Make sure we have a full directory entry */
		if (offset >= bufsize) {
			int slop = bufsize - offset + de_len;
			if (!tmpde) {
				tmpde = kmalloc(256, GFP_KERNEL);
				if (!tmpde)
					goto out_nomem;
			}
			memcpy(tmpde, de, slop);
			offset &= bufsize - 1;
			block++;
			brelse(bh);
			bh = NULL;
			if (offset) {
				bh = sb_bread(inode->i_sb, block);
				if (!bh)
					goto out_noread;
				memcpy((unsigned char *)tmpde+slop, bh->b_data, offset);
			}
			de = tmpde;
		}

		if (de->name_len[0] != 1 || de_len < sizeof(*de) + 1 || de->name[0] != 0 ||
		    !(isonum_711(de->flags) & 2))
			goto out_corrupt;
		if ((isonum_711(de->flags) & 0x80) &&
		    (isonum_733(de->size) & (bufsize - 1)))
			goto out_corrupt;
		if (sections) {
			sections[i].block = (uint64_t)isonum_733(de->extent) + de->ext_attr_length[0];
			sections[i].size = isonum_733(de->size);
			sections[i].gap = de->interleave[0];
		}
		inode->i_size += isonum_733(de->size);
		if (i == 1) {
			ei->i_next_section_block = block_saved;
			ei->i_next_section_offset = offset_saved;
		}

		more_entries = isonum_711(de->flags) & 0x80;

		i++;
		if (i >= SEVENZIP_ISO_MAX_SECTIONS && more_entries)
			goto out_toomany;
	} while (more_entries);
	*count = (unsigned)i;
	kfree(tmpde);
	brelse(bh);
	return 0;

out_nomem:
	brelse(bh);
	return -ENOMEM;

out_noread:
	printk(KERN_INFO "ISOFS: unable to read i-node block %lu\n", block);
	kfree(tmpde);
	return -EIO;

out_toomany:
	printk(KERN_INFO "%s: More than 100 file sections ?!?, aborting...\n"
		"isofs_read_level3_size: inode=%llu\n",
		__func__, inode->i_ino);
	goto out_corrupt;

out_corrupt:
	kfree(tmpde);
	brelse(bh);
	return -EIO;
}
