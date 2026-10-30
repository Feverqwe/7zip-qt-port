/* SPDX-License-Identifier: GPL-2.0-only */
/* Published Linux certificates exercise parser + hashing + self-signature,
 * including rejection of corruption, independent of any system trust store. */
#include "crypto/certificates/x509_parser.h"
#include <stdio.h>
#include <crypto/pkcs7.h>
#include "crypto/fscrypt/asymmetrictrust.h"
#include "fixtures/asn1/selftest_rsa.h"
#include "fixtures/asn1/selftest_ecdsa.h"
#include "fixtures/asn1/trust-chain.h"

static int check_certificate(const u8 *data, unsigned size)
{
	struct x509_certificate *cert = x509_cert_parse(data, size);
	if (IS_ERR(cert)) {
		fprintf(stderr, "X509 known certificate rejected: %d\n", PTR_ERR(cert));
		return 1;
	}
	int ret = !cert->self_signed || cert->unsupported_sig ||
		public_key_verify_signature(cert->pub, cert->sig);
	x509_free_certificate(cert);
	if (ret) return 1;
	u8 *changed = malloc(size);
	if (!changed) return 1;
	memcpy(changed, data, size);
	/* Last byte belongs to the signature in these published DER certificates. */
	changed[size - 1] ^= 1;
	cert = x509_cert_parse(changed, size);
	ret = !IS_ERR(cert);
	if (!IS_ERR(cert)) x509_free_certificate(cert);
	free(changed);
	if (ret) { fprintf(stderr, "X509 altered self-signature accepted\n"); return 1; }
	cert = x509_cert_parse(data, size - 1);
	ret = !IS_ERR(cert);
	if (!IS_ERR(cert)) x509_free_certificate(cert);
	if (ret) fprintf(stderr, "X509 truncated certificate accepted\n");
	return ret;
}

static int check_message(const u8 *signature, unsigned signature_size,
			 const u8 *data, unsigned data_size)
{
	struct pkcs7_message *message = pkcs7_parse_message(signature, signature_size);
	if (IS_ERR(message)) { fprintf(stderr, "PKCS7 parse: %d\n", PTR_ERR(message)); return 1; }
	int ret = pkcs7_supply_detached_data(message, data, data_size) ||
		pkcs7_supply_detached_data(message, data, data_size) != -EINVAL ||
		pkcs7_verify(message, VERIFYING_UNSPECIFIED_SIGNATURE);
	pkcs7_free_message(message);
	if (ret) { fprintf(stderr, "PKCS7 known signature rejected\n"); return 1; }
	u8 changed[128];
	if (data_size > sizeof(changed)) return 1;
	memcpy(changed, data, data_size); changed[0] ^= 1;
	message = pkcs7_parse_message(signature, signature_size);
	if (IS_ERR(message)) return 1;
	ret = pkcs7_supply_detached_data(message, changed, data_size) ||
		pkcs7_verify(message, VERIFYING_UNSPECIFIED_SIGNATURE) >= 0;
	pkcs7_free_message(message);
	if (ret) fprintf(stderr, "PKCS7 changed signed content accepted\n");
	return ret;
}

static int check_trust(const u8 *certificate, unsigned certificate_size,
		       const u8 *signature, unsigned signature_size,
		       const u8 *embedded, unsigned embedded_size,
		       const u8 *data, unsigned data_size,
		       const u8 *foreign_certificate, unsigned foreign_size)
{
	u8 *input = malloc(certificate_size);
	if (!input) return 1;
	memcpy(input, certificate, certificate_size);
	struct sevenzip_certificate anchor = { input, certificate_size };
	struct sevenzip_asymmetric_trust *trust = NULL, *empty = NULL, *foreign = NULL;
	int ret = sevenzip_asymmetric_trust_create(&anchor, 1, &trust);
	memset(input, 0, certificate_size); free(input);
	if (ret) { fprintf(stderr, "Owned trust creation: %d\n", ret); return 1; }
	ret = 1;
	const char *step = "owned trust, detached signature";
	if (sevenzip_asymmetric_trust_verify(trust, data, data_size, signature, signature_size)) goto out;
	step = "owned trust, embedded certificate";
	if (sevenzip_asymmetric_trust_verify(trust, data, data_size, embedded, embedded_size)) goto out;
	u8 changed[128];
	if (data_size > sizeof(changed)) goto out;
	memcpy(changed, data, data_size); changed[0] ^= 1;
	step = "altered data";
	if (sevenzip_asymmetric_trust_verify(trust, changed, data_size, signature, signature_size) >= 0) goto out;
	step = "empty trust, detached signature";
	if (sevenzip_asymmetric_trust_create(NULL, 0, &empty) ||
	    sevenzip_asymmetric_trust_verify(empty, data, data_size, signature, signature_size) != -ENOKEY) goto out;
	step = "empty trust, embedded certificate";
	if (sevenzip_asymmetric_trust_verify(empty, data, data_size, embedded, embedded_size) != -ENOKEY) goto out;
	anchor.data = foreign_certificate; anchor.size = foreign_size;
	step = "foreign trust";
	if (sevenzip_asymmetric_trust_create(&anchor, 1, &foreign) ||
	    sevenzip_asymmetric_trust_verify(foreign, data, data_size, signature, signature_size) != -ENOKEY ||
	    sevenzip_asymmetric_trust_verify(foreign, data, data_size, embedded, embedded_size) != -ENOKEY) goto out;
	struct sevenzip_certificate anchors[] = {{ certificate, certificate_size }, { certificate, certificate_size - 1 }};
	struct sevenzip_asymmetric_trust *invalid = NULL;
	step = "partial trust-configuration failure";
	if (sevenzip_asymmetric_trust_create(anchors, 2, &invalid) >= 0 || invalid) {
		sevenzip_asymmetric_trust_destroy(invalid); goto out;
	}
	ret = 0;
out:
	if (ret) fprintf(stderr, "%s failed\n", step);
	sevenzip_asymmetric_trust_destroy(foreign);
	sevenzip_asymmetric_trust_destroy(empty);
	sevenzip_asymmetric_trust_destroy(trust);
	return ret;
}

static int check_chain(void)
{
	/* A matching issuer/serial and even a matching SKID do not establish
	 * trust unless that trusted variant verifies the descendant signature. */
	const struct {
		const u8 *certificate, *message;
		size_t certificate_size, message_size;
		int expected;
	} cases[] = {
		{ trust_chain_root, trust_chain_full, sizeof(trust_chain_root) - 1, sizeof(trust_chain_full) - 1, 0 },
		{ trust_chain_root, trust_chain_no_root, sizeof(trust_chain_root) - 1, sizeof(trust_chain_no_root) - 1, 0 },
		{ trust_chain_root, trust_chain_detached, sizeof(trust_chain_root) - 1, sizeof(trust_chain_detached) - 1, -ENOKEY },
		{ trust_chain_leaf, trust_chain_detached, sizeof(trust_chain_leaf) - 1, sizeof(trust_chain_detached) - 1, 0 },
		{ trust_chain_leaf, trust_chain_full, sizeof(trust_chain_leaf) - 1, sizeof(trust_chain_full) - 1, 0 },
		{ trust_chain_intermediate, trust_chain_full, sizeof(trust_chain_intermediate) - 1, sizeof(trust_chain_full) - 1, 0 },
		{ trust_chain_collision, trust_chain_full, sizeof(trust_chain_collision) - 1, sizeof(trust_chain_full) - 1, -EKEYREJECTED },
		{ trust_chain_collision, trust_chain_no_root, sizeof(trust_chain_collision) - 1, sizeof(trust_chain_no_root) - 1, -EKEYREJECTED },
		{ trust_chain_wrong_id, trust_chain_no_root, sizeof(trust_chain_wrong_id) - 1, sizeof(trust_chain_no_root) - 1, -EKEYREJECTED },
	};
	for (size_t i = 0; i < ARRAY_SIZE(cases); ++i) {
		struct sevenzip_certificate certificate = { cases[i].certificate, cases[i].certificate_size };
		struct sevenzip_asymmetric_trust *trust = NULL;
		int ret = sevenzip_asymmetric_trust_create(&certificate, 1, &trust);
		if (ret) { fprintf(stderr, "Chain trust setup %zu: %d\n", i, ret); return 1; }
		ret = sevenzip_asymmetric_trust_verify(trust, trust_chain_data, sizeof(trust_chain_data) - 1,
			cases[i].message, cases[i].message_size);
		sevenzip_asymmetric_trust_destroy(trust);
		if (ret != cases[i].expected) {
			fprintf(stderr, "Chain trust %zu: %d, expected %d\n", i, ret, cases[i].expected); return 1;
		}
	}
	return 0;
}

int main(void)
{
	return check_certificate(certs_selftest_rsa_keys, sizeof(certs_selftest_rsa_keys) - 1) ||
		check_certificate(certs_selftest_ecdsa_keys, sizeof(certs_selftest_ecdsa_keys) - 1) ||
		check_message(certs_selftest_rsa_embedded, sizeof(certs_selftest_rsa_embedded) - 1,
			certs_selftest_rsa_data, sizeof(certs_selftest_rsa_data) - 1) ||
		check_message(certs_selftest_ecdsa_embedded, sizeof(certs_selftest_ecdsa_embedded) - 1,
			certs_selftest_ecdsa_data, sizeof(certs_selftest_ecdsa_data) - 1) ||
		check_trust(certs_selftest_rsa_keys, sizeof(certs_selftest_rsa_keys) - 1,
			certs_selftest_rsa_sig, sizeof(certs_selftest_rsa_sig) - 1,
			certs_selftest_rsa_embedded, sizeof(certs_selftest_rsa_embedded) - 1,
			certs_selftest_rsa_data, sizeof(certs_selftest_rsa_data) - 1,
			certs_selftest_ecdsa_keys, sizeof(certs_selftest_ecdsa_keys) - 1) ||
		check_trust(certs_selftest_ecdsa_keys, sizeof(certs_selftest_ecdsa_keys) - 1,
			certs_selftest_ecdsa_sig, sizeof(certs_selftest_ecdsa_sig) - 1,
			certs_selftest_ecdsa_embedded, sizeof(certs_selftest_ecdsa_embedded) - 1,
			certs_selftest_ecdsa_data, sizeof(certs_selftest_ecdsa_data) - 1,
			certs_selftest_rsa_keys, sizeof(certs_selftest_rsa_keys) - 1) || check_chain();
}
