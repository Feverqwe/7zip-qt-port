// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 * TODO: Merge attr_set_size/attr_data_get_block/attr_allocate_frame?
 */

#include "ntfswofcompat.h"

int attr_wof_frame_info(struct ntfs_inode *ni, struct ATTRIB *attr,
			struct runs_tree *run, u64 frame, u64 frames,
			u8 frame_bits, u32 *ondisk_size, u64 *vbo_data)
{
	struct ntfs_sb_info *sbi = ni->mi.sbi;
	u64 vbo[2], off[2], wof_size;
	u32 voff;
	u8 bytes_per_off;
	char *addr;
	struct folio *folio;
	int i, err = 0;
	__le32 *off32;
	__le64 *off64;

	if (ni->vfs_inode.i_size < 0x100000000ull) {
		/* File starts with array of 32 bit offsets. */
		bytes_per_off = sizeof(__le32);
		vbo[1] = frame << 2;
		*vbo_data = frames << 2;
	} else {
		/* File starts with array of 64 bit offsets. */
		bytes_per_off = sizeof(__le64);
		vbo[1] = frame << 3;
		*vbo_data = frames << 3;
	}

	/*
	 * Read 4/8 bytes at [vbo - 4(8)] == offset where compressed frame starts.
	 * Read 4/8 bytes at [vbo] == offset where compressed frame ends.
	 */
	if (!attr->non_res) {
		if (vbo[1] + bytes_per_off > le32_to_cpu(attr->res.data_size)) {
			_ntfs_bad_inode(&ni->vfs_inode);
			return -EINVAL;
		}
		addr = resident_data(attr);

		if (bytes_per_off == sizeof(__le32)) {
			off32 = Add2Ptr(addr, vbo[1]);
			off[0] = vbo[1] ? le32_to_cpu(off32[-1]) : 0;
			off[1] = le32_to_cpu(off32[0]);
		} else {
			off64 = Add2Ptr(addr, vbo[1]);
			off[0] = vbo[1] ? le64_to_cpu(off64[-1]) : 0;
			off[1] = le64_to_cpu(off64[0]);
		}

		if (ntfs_wof_bounds(off[0], off[1], frame, frames, frame_bits,
				    le32_to_cpu(attr->res.data_size), *vbo_data))
			return -EINVAL;
		*vbo_data += off[0];
		*ondisk_size = off[1] - off[0];
		return 0;
	}

	wof_size = le64_to_cpu(attr->nres.data_size);
	down_write(&ni->file.run_lock);
	folio = ni->file.offs_folio;
	if (!folio) {
		folio = folio_alloc(GFP_KERNEL, 0);
		if (!folio) {
			err = -ENOMEM;
			goto out;
		}
		folio->index = -1;
		ni->file.offs_folio = folio;
	}
	folio_lock(folio);
	addr = folio_address(folio);

	if (vbo[1]) {
		voff = vbo[1] & (PAGE_SIZE - 1);
		vbo[0] = vbo[1] - bytes_per_off;
		i = 0;
	} else {
		voff = 0;
		vbo[0] = 0;
		off[0] = 0;
		i = 1;
	}

	do {
		pgoff_t index = vbo[i] >> PAGE_SHIFT;

		if (index != folio->index) {
			u64 from = vbo[i] & ~(u64)(PAGE_SIZE - 1);
			u64 to = min(from + PAGE_SIZE, wof_size);

			if (from >= wof_size) {
				_ntfs_bad_inode(&ni->vfs_inode);
				err = -EINVAL;
				goto out1;
			}

			err = attr_load_runs_range(ni, ATTR_DATA, WOF_NAME,
						   ARRAY_SIZE(WOF_NAME), run,
						   from, to);
			if (err)
				goto out1;

			err = ntfs_read_run(sbi, run, addr, from, to - from);
			if (err) {
				folio->index = -1;
				goto out1;
			}
			folio->index = index;
		}

		if (i) {
			if (bytes_per_off == sizeof(__le32)) {
				off32 = Add2Ptr(addr, voff);
				off[1] = le32_to_cpu(*off32);
			} else {
				off64 = Add2Ptr(addr, voff);
				off[1] = le64_to_cpu(*off64);
			}
		} else if (!voff) {
			if (bytes_per_off == sizeof(__le32)) {
				off32 = Add2Ptr(addr, PAGE_SIZE - sizeof(u32));
				off[0] = le32_to_cpu(*off32);
			} else {
				off64 = Add2Ptr(addr, PAGE_SIZE - sizeof(u64));
				off[0] = le64_to_cpu(*off64);
			}
		} else {
			/* Two values in one page. */
			if (bytes_per_off == sizeof(__le32)) {
				off32 = Add2Ptr(addr, voff);
				off[0] = le32_to_cpu(off32[-1]);
				off[1] = le32_to_cpu(off32[0]);
			} else {
				off64 = Add2Ptr(addr, voff);
				off[0] = le64_to_cpu(off64[-1]);
				off[1] = le64_to_cpu(off64[0]);
			}
			break;
		}
	} while (++i < 2);

	err = ntfs_wof_bounds(off[0], off[1], frame, frames, frame_bits,
			      wof_size, *vbo_data);
	if (err)
		goto out1;
	*vbo_data += off[0];
	*ondisk_size = off[1] - off[0];

out1:
	folio_unlock(folio);
out:
	up_write(&ni->file.run_lock);
	return err;
}
