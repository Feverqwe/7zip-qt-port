/* SPDX-License-Identifier: GPL-2.0-only */
/* Aligned owned memory replaces kernel page mapping for a flat archive request. */
#include "crypto/modes/skciphercompat.h"

int skcipher_walk_virt(struct skcipher_walk *walk, struct skcipher_request *req, bool atomic)
{
	(void)atomic;
	memset(walk, 0, sizeof(*walk));
	if (req->cryptlen > req->src->len || req->cryptlen > req->dst->len)
		return -EINVAL;
	if (!req->cryptlen) return 0;
	u8 *input = malloc(req->cryptlen);
	u8 *output = malloc(req->cryptlen);
	if (!input || !output) { free(input); free(output); return -ENOMEM; }
	memcpy(input, req->src->data, req->cryptlen);
	walk->input = input;
	walk->output = output;
	walk->src.virt.addr = input;
	walk->dst.virt.addr = output;
	walk->nbytes = req->cryptlen;
	walk->total = req->cryptlen;
	walk->req = req;
	return 0;
}

int skcipher_walk_done(struct skcipher_walk *walk, int remaining)
{
	int err = 0;
	if (remaining < 0) err = remaining;
	else if ((unsigned)remaining > walk->nbytes) err = -EINVAL;
	else memcpy(walk->req->dst->data, walk->output, walk->nbytes - (unsigned)remaining);
	memzero_explicit(walk->input, walk->nbytes);
	memzero_explicit(walk->output, walk->nbytes);
	free(walk->input);
	free(walk->output);
	walk->nbytes = 0;
	return err;
}
