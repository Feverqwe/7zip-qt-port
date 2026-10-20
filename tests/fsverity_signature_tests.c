/* SPDX-License-Identifier: GPL-2.0-only */
/* Real PKCS#7 signatures over Linux's formatted fs-verity digest exercise
 * mandatory/optional signature policy and explicit per-inode trust. */
#include "fs/verity/fsverity_private.h"
#include "fs/verity/signaturecompat.h"
#include <stdio.h>
#include "fixtures/asn1/trust-chain.h"

static int check_policy(unsigned algorithm, const u8 *signature, size_t size)
{
	struct inode inode = {0};
	struct fsverity_info info = {0};
	info.inode = &inode;
	info.tree_params.hash_alg = &fsverity_hash_algs[algorithm];
	for (unsigned i = 0; i < info.tree_params.hash_alg->digest_size; ++i) info.file_digest[i] = i;
	if (fsverity_verify_signature(&info, NULL, 0)) return 1;
	inode.verity_require_signatures = true;
	if (fsverity_verify_signature(&info, NULL, 0) != -EPERM) return 1;
	/* Empty trust rejects a present signature before reaching the parser. */
	if (fsverity_verify_signature(&info, signature, size) != -ENOKEY ||
	    fsverity_verify_signature(&info, (const u8 *)"invalid", 7) != -ENOKEY) return 1;
	struct sevenzip_certificate certificate = { trust_chain_root, sizeof(trust_chain_root) - 1 };
	struct sevenzip_asymmetric_trust *trust = NULL;
	if (sevenzip_asymmetric_trust_create(&certificate, 1, &trust)) return 1;
	inode.verity_trust = trust;
	int ret = 1;
	const char *step = "known formatted digest";
	if (fsverity_verify_signature(&info, signature, size)) goto out;
	inode.verity_require_signatures = false;
	step = "optional signatures still verify";
	if (fsverity_verify_signature(&info, signature, size)) goto out;
	info.file_digest[0] ^= 1;
	step = "wrong file digest";
	if (fsverity_verify_signature(&info, signature, size) >= 0) goto out;
	info.file_digest[0] ^= 1;
	step = "malformed signature with configured trust";
	if (fsverity_verify_signature(&info, (const u8 *)"invalid", 7) >= 0) goto out;
	step = "unsigned allowed by optional policy";
	if (fsverity_verify_signature(&info, NULL, 0)) goto out;
	ret = 0;
out:
	sevenzip_asymmetric_trust_destroy(trust);
	if (ret) fprintf(stderr, "fsverity algorithm %u: %s failed\n", algorithm, step);
	return ret;
}

int main(void)
{
	return check_policy(1, trust_chain_verity_1, sizeof(trust_chain_verity_1) - 1) ||
		check_policy(2, trust_chain_verity_2, sizeof(trust_chain_verity_2) - 1);
}
