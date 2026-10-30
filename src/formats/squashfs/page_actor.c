// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013
 * Phillip Lougher <phillip@squashfs.org.uk>
 */

#include "formats/squashfs/squashfscompat.h"
#include "formats/squashfs/page_actor.h"

/* Implementation of page_actor for decompressing into intermediate buffer */
static void *cache_first_page(struct squashfs_page_actor *actor)
{
	actor->next_page = 1;
	return actor->buffer[0];
}

static void *cache_next_page(struct squashfs_page_actor *actor)
{
	if (actor->next_page == actor->pages)
		return NULL;

	return actor->buffer[actor->next_page++];
}

static void cache_finish_page(struct squashfs_page_actor *actor)
{
	/* empty */
}

struct squashfs_page_actor *squashfs_page_actor_init(void **buffer,
	int pages, int length)
{
	struct squashfs_page_actor *actor = kmalloc_obj(*actor);

	if (actor == NULL)
		return NULL;

	actor->length = length ? length : pages * PAGE_SIZE;
	actor->buffer = buffer;
	actor->pages = pages;
	actor->next_page = 0;
	actor->tmp_buffer = NULL;
	actor->squashfs_first_page = cache_first_page;
	actor->squashfs_next_page = cache_next_page;
	actor->squashfs_finish_page = cache_finish_page;
	return actor;
}
