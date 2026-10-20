// SPDX-License-Identifier: GPL-2.0-only
/* Scatterlist page/mapping iterators from Linux lib/scatterlist.c.
 * Copyright (C) 2007 Jens Axboe <jens.axboe@oracle.com>
 * Selected synchronous memory-mapping path. */
#include "scatterread.h"

void __sg_page_iter_start(struct sg_page_iter *piter,
			  struct scatterlist *sglist, unsigned int nents,
			  unsigned long pgoffset)
{
	piter->__pg_advance = 0;
	piter->__nents = nents;

	piter->sg = sglist;
	piter->sg_pgoffset = pgoffset;
}
EXPORT_SYMBOL(__sg_page_iter_start);

static int sg_page_count(struct scatterlist *sg)
{
	return PAGE_ALIGN(sg->offset + sg->length) >> PAGE_SHIFT;
}

bool __sg_page_iter_next(struct sg_page_iter *piter)
{
	if (!piter->__nents || !piter->sg)
		return false;

	piter->sg_pgoffset += piter->__pg_advance;
	piter->__pg_advance = 1;

	while (piter->sg_pgoffset >= sg_page_count(piter->sg)) {
		piter->sg_pgoffset -= sg_page_count(piter->sg);
		piter->sg = sg_next(piter->sg);
		if (!--piter->__nents || !piter->sg)
			return false;
	}

	return true;
}
EXPORT_SYMBOL(__sg_page_iter_next);

void sg_miter_start(struct sg_mapping_iter *miter, struct scatterlist *sgl,
		    unsigned int nents, unsigned int flags)
{
	memset(miter, 0, sizeof(struct sg_mapping_iter));

	__sg_page_iter_start(&miter->piter, sgl, nents, 0);
	WARN_ON(!(flags & (SG_MITER_TO_SG | SG_MITER_FROM_SG)));
	miter->__flags = flags;
}
EXPORT_SYMBOL(sg_miter_start);

static bool sg_miter_get_next_page(struct sg_mapping_iter *miter)
{
	if (!miter->__remaining) {
		struct scatterlist *sg;

		if (!__sg_page_iter_next(&miter->piter))
			return false;

		sg = miter->piter.sg;

		miter->__offset = miter->piter.sg_pgoffset ? 0 : sg->offset;
		miter->piter.sg_pgoffset += miter->__offset >> PAGE_SHIFT;
		miter->__offset &= PAGE_SIZE - 1;
		miter->__remaining = sg->offset + sg->length -
				     (miter->piter.sg_pgoffset << PAGE_SHIFT) -
				     miter->__offset;
		miter->__remaining = min_t(unsigned long, miter->__remaining,
					   PAGE_SIZE - miter->__offset);
	}

	return true;
}

bool sg_miter_next(struct sg_mapping_iter *miter)
{
	sg_miter_stop(miter);

	/*
	 * Get to the next page if necessary.
	 * __remaining, __offset is adjusted by sg_miter_stop
	 */
	if (!sg_miter_get_next_page(miter))
		return false;

	miter->mapped_page.data = miter->piter.sg->page.data +
		(miter->piter.sg_pgoffset << PAGE_SHIFT);
	miter->page = &miter->mapped_page;
	miter->consumed = miter->length = miter->__remaining;

	if (miter->__flags & SG_MITER_ATOMIC)
		miter->addr = (u8 *)kmap_atomic(miter->page) + miter->__offset;
	else if (miter->__flags & SG_MITER_LOCAL)
		miter->addr = (u8 *)kmap_local_page(miter->page) + miter->__offset;
	else
		miter->addr = (u8 *)kmap(miter->page) + miter->__offset;

	return true;
}
EXPORT_SYMBOL(sg_miter_next);

/**
 * sg_miter_stop - stop mapping iteration
 * @miter: sg mapping iter to be stopped
 *
 * Description:
 *   Stops mapping iterator @miter.  @miter should have been started
 *   using sg_miter_start().  A stopped iteration can be resumed by
 *   calling sg_miter_next() on it.  This is useful when resources (kmap)
 *   need to be released during iteration.
 *
 * Context:
 *   Don't care otherwise.
 */
void sg_miter_stop(struct sg_mapping_iter *miter)
{
	WARN_ON(miter->consumed > miter->length);

	/* drop resources from the last iteration */
	if (miter->addr) {
		miter->__offset += miter->consumed;
		miter->__remaining -= miter->consumed;

		if (miter->__flags & SG_MITER_TO_SG)
			flush_dcache_page(miter->page);

		if (miter->__flags & SG_MITER_ATOMIC) {
			WARN_ON_ONCE(!pagefault_disabled());
			kunmap_atomic(miter->addr);
		} else if (miter->__flags & SG_MITER_LOCAL)
			kunmap_local(miter->addr);
		else
			kunmap(miter->page);

		miter->page = NULL;
		miter->addr = NULL;
		miter->length = 0;
		miter->consumed = 0;
	}
}
EXPORT_SYMBOL(sg_miter_stop);
