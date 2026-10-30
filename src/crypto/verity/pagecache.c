// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 Google LLC
 */
#include "crypto/verity/fsverity_private.h"

/**
 * generic_read_merkle_tree_page - generic ->read_merkle_tree_page helper
 * @inode:	inode containing the Merkle tree
 * @index:	0-based index of the Merkle tree page in the inode
 *
 * The caller needs to adjust @index from the Merkle-tree relative index passed
 * to ->read_merkle_tree_page to the actual index where the Merkle tree is
 * stored in the page cache for @inode.
 */
struct page *generic_read_merkle_tree_page(struct inode *inode, pgoff_t index)
{
	struct folio *folio;

	folio = read_mapping_folio(inode->i_mapping, index, NULL);
	if (IS_ERR(folio))
		return ERR_CAST(folio);
	return folio_file_page(folio, index);
}
