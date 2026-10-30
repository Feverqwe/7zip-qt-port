/* fshelp.c -- Filesystem helper functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2005,2006,2007,2008  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dependencies/grub/include/grub/fs/xfs_compat.h"

grub_ssize_t
grub_fshelp_read_file (grub_disk_t disk, grub_fshelp_node_t node,
		       grub_disk_read_hook_t read_hook, void *read_hook_data,
		       grub_off_t pos, grub_size_t len, char *buf,
		       grub_disk_addr_t (*get_block) (grub_fshelp_node_t node,
                                                      grub_disk_addr_t block),
		       grub_off_t filesize, int log2blocksize,
		       grub_disk_addr_t blocks_start)
{
  grub_disk_addr_t i, blockcnt;
  int blocksize = 1 << (log2blocksize + GRUB_DISK_SECTOR_BITS);

  /*
   * Catch blatantly invalid log2blocksize. We could be a lot stricter, but
   * this is the most permissive we can be before we start to see integer
   * overflow/underflow issues.
   */
  if (log2blocksize < 0 || log2blocksize + GRUB_DISK_SECTOR_BITS >= 31)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE,
		  N_("blocksize too large"));
      return -1;
    }

  if (pos > filesize)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE,
		  N_("attempt to read past the end of file"));
      return -1;
    }

  /* Adjust LEN so it we can't read past the end of the file.  */
  if (len > filesize - pos)
    len = filesize - pos;

  blockcnt = ((len + pos) + blocksize - 1) >> (log2blocksize + GRUB_DISK_SECTOR_BITS);

  for (i = pos >> (log2blocksize + GRUB_DISK_SECTOR_BITS); i < blockcnt; i++)
    {
      if (disk->source.poll && disk->source.poll(disk->source.context))
        { grub_error(GRUB_ERR_READ_ERROR, "cancelled"); return -1; }
      grub_disk_addr_t blknr;
      int blockoff = pos & (blocksize - 1);
      int blockend = blocksize;

      int skipfirst = 0;

      blknr = get_block (node, i);
      if (grub_errno)
	return -1;

      blknr = blknr << log2blocksize;

      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) & (blocksize - 1);

	  /* The last portion is exactly blocksize.  */
	  if (! blockend)
	    blockend = blocksize;
	}

      /* First block.  */
      if (i == (pos >> (log2blocksize + GRUB_DISK_SECTOR_BITS)))
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      /* If the block number is 0 this block is not stored on disk but
	 is zero filled instead.  */
      if (blknr)
	{
	  disk->read_hook = read_hook;
	  disk->read_hook_data = read_hook_data;

	  grub_disk_read (disk, blknr + blocks_start, skipfirst,
			  blockend, buf);
	  disk->read_hook = 0;
	  if (grub_errno)
	    return -1;
	}
      else
	grub_memset (buf, 0, blockend);

      buf += blockend;
    }

  return len;
}
