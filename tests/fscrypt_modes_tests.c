/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../src/crypto/skciphercompat.h"
#include <stdio.h>

struct mode_vector {
	unsigned keylen, len;
	const char *key, *iv, *plain, *cipher, *iv_out;
};
static const struct mode_vector hctr2_vectors[] = {
#include "fixtures/fscrypt/hctr2-vectors.h"
};
static const struct mode_vector essiv_vectors[] = {
#include "fixtures/fscrypt/essiv-vectors.h"
};
static const struct mode_vector sm4_cbc_vectors[] = {
#include "fixtures/fscrypt/sm4_cbc-vectors.h"
};
static const struct mode_vector sm4_cts_vectors[] = {
#include "fixtures/fscrypt/sm4_cts-vectors.h"
};
static const struct mode_vector sm4_xts_vectors[] = {
#include "fixtures/fscrypt/sm4_xts-vectors.h"
};
static const struct mode_vector adiantum12_vectors[] = {
#include "fixtures/fscrypt/adiantum_xchacha12_aes-vectors.h"
};
static const struct mode_vector adiantum20_vectors[] = {
#include "fixtures/fscrypt/adiantum_xchacha20_aes-vectors.h"
};
static const struct mode_vector xchacha12_vectors[] = {
#include "fixtures/fscrypt/xchacha12-vectors.h"
};
static const struct mode_vector xchacha20_vectors[] = {
#include "fixtures/fscrypt/xchacha20-vectors.h"
};
static const struct mode_vector chacha20_vectors[] = {
#include "fixtures/fscrypt/chacha20-vectors.h"
};

static int check_mode(const char *name, const struct mode_vector *v, unsigned index)
{
	struct crypto_skcipher *tfm = crypto_alloc_skcipher(name, 0, 0);
	if (IS_ERR(tfm)) return 1;
	int result = 1;
	struct skcipher_request *req = skcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) goto out;
	for (unsigned flags = 0; flags < 8; ++flags) {
		u8 input[4097], output[4097], iv[32] = {0};
		unsigned offset = flags & 1;
		bool in_place = (flags & 2) != 0;
		bool decrypt = (flags & 4) != 0;
		struct scatterlist src, dst;
		memcpy(input + offset, decrypt ? v->cipher : v->plain, v->len);
		if (v->iv) memcpy(iv, v->iv, tfm->ivsize);
		sg_init_one(&src, input + offset, v->len);
		sg_init_one(&dst, in_place ? input + offset : output + offset, v->len);
		if (offset) {
			struct page input_page = {input};
			struct page output_page = {in_place ? input : output};
			sg_set_page(&src, &input_page, v->len, offset);
			sg_set_page(&dst, &output_page, v->len, offset);
		}
		skcipher_request_set_crypt(req, &src, &dst, v->len, iv);
		if (crypto_skcipher_decrypt(req) != -ENOKEY) goto out_req;
		if (crypto_skcipher_setkey(tfm, (const u8 *)v->key, v->keylen)) goto out_req;
		int err = decrypt ? crypto_skcipher_decrypt(req) : crypto_skcipher_encrypt(req);
		if (err || memcmp(dst.data, decrypt ? v->plain : v->cipher, v->len)) {
			fprintf(stderr, "%s vector=%u flags=%u err=%d\n", name, index, flags, err);
			goto out_req;
		}
		if (v->iv_out && memcmp(iv, v->iv_out, tfm->ivsize)) goto out_req;
		if (!strcmp(name, "xts(sm4)") && memcmp(iv, v->iv, tfm->ivsize)) goto out_req;
		/* Reject requests that cannot hold a first block or exceed their
		 * source/destination bounds, before touching those buffers. */
		if (tfm->blocksize > 1) {
			req->cryptlen = 15;
			if (crypto_skcipher_decrypt(req) != -EINVAL) goto out_req;
		}
		req->cryptlen = v->len + 1;
		if (crypto_skcipher_decrypt(req) != -EINVAL) goto out_req;
		if (crypto_skcipher_setkey(tfm, (const u8 *)v->key, 17) != -EINVAL)
			goto out_req;
		req->cryptlen = v->len;
		if (crypto_skcipher_decrypt(req) != -ENOKEY) goto out_req;
	}
	result = 0;
out_req:
	skcipher_request_free(req);
out:
	crypto_free_skcipher(tfm);
	return result;
}

int fscrypt_modes_tests(void)
{
	/* fscrypt asks the Crypto API to reject identical XTS key halves. */
	const char *xts_names[] = {"xts(aes)", "xts(sm4)"};
	for (unsigned i = 0; i < ARRAY_SIZE(xts_names); ++i) {
		struct crypto_skcipher *xts = crypto_alloc_skcipher(xts_names[i], 0, 0);
		if (IS_ERR(xts)) return 1;
		u8 weak_key[32] = {0};
		crypto_skcipher_set_flags(xts, CRYPTO_TFM_REQ_FORBID_WEAK_KEYS);
		int weak_error = crypto_skcipher_setkey(xts, weak_key, sizeof(weak_key));
		crypto_free_skcipher(xts);
		if (weak_error != -EINVAL) return 1;
	}
	for (unsigned i = 0; i < ARRAY_SIZE(hctr2_vectors); ++i)
		if (check_mode("hctr2(aes)", &hctr2_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(essiv_vectors); ++i)
		if (check_mode("essiv(cbc(aes),sha256)", &essiv_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(sm4_cbc_vectors); ++i)
		if (check_mode("cbc(sm4)", &sm4_cbc_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(sm4_cts_vectors); ++i)
		if (check_mode("cts(cbc(sm4))", &sm4_cts_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(sm4_xts_vectors); ++i)
		if (check_mode("xts(sm4)", &sm4_xts_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(adiantum12_vectors); ++i)
		if (check_mode("adiantum(xchacha12,aes)", &adiantum12_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(adiantum20_vectors); ++i)
		if (check_mode("adiantum(xchacha20,aes)", &adiantum20_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(xchacha12_vectors); ++i)
		if (check_mode("xchacha12", &xchacha12_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(xchacha20_vectors); ++i)
		if (check_mode("xchacha20", &xchacha20_vectors[i], i)) return 1;
	for (unsigned i = 0; i < ARRAY_SIZE(chacha20_vectors); ++i)
		if (check_mode("chacha20", &chacha20_vectors[i], i)) return 1;
	return 0;
}
