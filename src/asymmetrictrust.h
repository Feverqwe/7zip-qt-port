/* SPDX-License-Identifier: GPL-2.0-only */
/* Immutable explicit archive trust, isolated from host/kernel key stores. */
#ifndef SEVENZIP_ASYMMETRIC_TRUST_H
#define SEVENZIP_ASYMMETRIC_TRUST_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_asymmetric_trust;
enum sevenzip_certificate_encoding { SEVENZIP_CERTIFICATE_DER, SEVENZIP_CERTIFICATE_HEX };
struct sevenzip_certificate {
	const void *data;
	size_t size;
	enum sevenzip_certificate_encoding encoding;
};
/* Copies all DER or ASCII hexadecimal inputs. On failure no partially configured trust
 * escapes. The immutable result can be borrowed by concurrent verifications;
 * its owner must retain it until those synchronous calls finish. */
int sevenzip_asymmetric_trust_create(const struct sevenzip_certificate *, size_t,
				    struct sevenzip_asymmetric_trust **);
void sevenzip_asymmetric_trust_destroy(struct sevenzip_asymmetric_trust *);
size_t sevenzip_asymmetric_trust_count(const struct sevenzip_asymmetric_trust *);
/* Verifies detached PKCS#7 data and its chain against this object's anchors. */
int sevenzip_asymmetric_trust_verify(const struct sevenzip_asymmetric_trust *,
				    const void *, size_t, const void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
