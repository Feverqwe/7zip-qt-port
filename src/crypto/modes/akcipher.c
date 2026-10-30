// SPDX-License-Identifier: GPL-2.0-or-later
/* Public Key Encryption: selected synchronous memory request path.
 * Copyright (c) 2015, Intel Corporation
 * Authors: Tadeusz Struk <tadeusz.struk@intel.com> */
#include <crypto/akcipher.h>

struct crypto_akcipher_sync_data {
	struct crypto_akcipher *tfm;
	const void *src;
	void *dst;
	unsigned int slen;
	unsigned int dlen;

	struct akcipher_request *req;
	struct crypto_wait cwait;
	struct scatterlist sg;
	u8 *buf;
};

static int crypto_akcipher_sync_prep(struct crypto_akcipher_sync_data *data)
{
	unsigned int reqsize = crypto_akcipher_reqsize(data->tfm);
	struct akcipher_request *req;
	struct scatterlist *sg;
	unsigned int mlen;
	unsigned int len;
	u8 *buf;

	mlen = max(data->slen, data->dlen);

	len = sizeof(*req) + reqsize + mlen;
	if (len < mlen)
		return -EOVERFLOW;

	req = kzalloc(len, GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	data->req = req;
	akcipher_request_set_tfm(req, data->tfm);

	buf = (u8 *)(req + 1) + reqsize;
	data->buf = buf;
	memcpy(buf, data->src, data->slen);

	sg = &data->sg;
	sg_init_one(sg, buf, mlen);
	akcipher_request_set_crypt(req, sg, sg, data->slen, data->dlen);

	crypto_init_wait(&data->cwait);
	akcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_SLEEP,
				      crypto_req_done, &data->cwait);

	return 0;
}

static int crypto_akcipher_sync_post(struct crypto_akcipher_sync_data *data,
				     int err)
{
	err = crypto_wait_req(err, &data->cwait);
	memcpy(data->dst, data->buf, data->dlen);
	data->dlen = data->req->dst_len;
	kfree_sensitive(data->req);
	return err;
}

int crypto_akcipher_sync_encrypt(struct crypto_akcipher *tfm,
				 const void *src, unsigned int slen,
				 void *dst, unsigned int dlen)
{
	struct crypto_akcipher_sync_data data = {
		.tfm = tfm,
		.src = src,
		.dst = dst,
		.slen = slen,
		.dlen = dlen,
	};

	int err = crypto_akcipher_sync_prep(&data);
	return err ? err : crypto_akcipher_sync_post(&data,
					 crypto_akcipher_encrypt(data.req));
}
EXPORT_SYMBOL_GPL(crypto_akcipher_sync_encrypt);

int crypto_akcipher_sync_decrypt(struct crypto_akcipher *tfm,
				 const void *src, unsigned int slen,
				 void *dst, unsigned int dlen)
{
	struct crypto_akcipher_sync_data data = {
		.tfm = tfm,
		.src = src,
		.dst = dst,
		.slen = slen,
		.dlen = dlen,
	};

	int err = crypto_akcipher_sync_prep(&data);
	if (err)
		return err;
	err = crypto_akcipher_sync_post(&data, crypto_akcipher_decrypt(data.req));
	return err ? err : (int)data.dlen;
}
EXPORT_SYMBOL_GPL(crypto_akcipher_sync_decrypt);
