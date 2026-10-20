/* SPDX-License-Identifier: GPL-2.0-or-later */
/* DER/BER input and callback errors, through the original compiled grammar. */
#include <linux/asn1_decoder.h>
#include <linux/asn1_ber_bytecode.h>
#include <linux/oid_registry.h>
#include "spki.asn1.h"
#include <stdio.h>

struct result { const u8 *oid, *params, *key; size_t oid_size, params_size, key_size; int reject; };
int capture_oid(void *context, size_t header, unsigned char tag, const void *value, size_t size)
{
    struct result *r = context;
    if (r->reject) return r->reject;
    r->oid = value; r->oid_size = size;
    return 0;
}
int capture_params(void *context, size_t header, unsigned char tag, const void *value, size_t size)
{
    struct result *r = context;
    r->params = value; r->params_size = size;
    return 0;
}
int capture_key(void *context, size_t header, unsigned char tag, const void *value, size_t size)
{
    struct result *r = context;
    r->key = value; r->key_size = size;
    return 0;
}
static const u8 oid[] = {0x2a,0x86,0x48,0x86,0xf7,0x0d,1,1,1}; /* rsaEncryption */
static int check_oids(void)
{
    const u8 rsa[] = {6,9,0x2a,0x86,0x48,0x86,0xf7,0x0d,1,1,1};
    const u8 mldsa[] = {0x60,0x86,0x48,1,0x65,3,4,3,19};
    const u8 unknown[] = {0x2b,6,1,4,1,0xff,0x7f};
    const u8 wide[] = {0x2a,0x90,0x80,0x80,0x80,1};
    enum OID result;
    char text[64];
    if (parse_OID(rsa, sizeof(rsa), &result) || result != OID_rsaEncryption ||
        look_up_OID(mldsa, sizeof(mldsa)) != OID_id_ml_dsa_87 ||
        look_up_OID(unknown, sizeof(unknown)) != OID__NR ||
        sprint_oid(wide, sizeof(wide), text, sizeof(text)) < 0 || strcmp(text, "1.2.4294967297"))
        return 1;
    if (sprint_oid(wide, sizeof(wide), text, 4) != -ENOBUFS ||
        sprint_oid(wide, sizeof(wide) - 1, text, sizeof(text)) != -EBADMSG)
        return 1;
    for (size_t n = 0; n < sizeof(rsa); ++n)
        if (parse_OID(rsa, n, &result) != -EBADMSG) return 1;
    return 0;
}
static int check(const u8 *bytes, size_t size, bool params)
{
    struct result r = {0};
    if (asn1_ber_decoder(&spki_decoder, &r, bytes, size) ||
        r.oid_size != sizeof(oid) || memcmp(r.oid, oid, sizeof(oid)) ||
        r.key_size != 4 || memcmp(r.key, "\0key", 4) ||
        (r.params != NULL) != params || r.params_size)
        return 1;
    for (size_t n = 0; n < size; ++n) {
        struct result truncated = {0};
        if (asn1_ber_decoder(&spki_decoder, &truncated, bytes, n) != -EBADMSG)
            return 1;
    }
    r.reject = -ECANCELED;
    return asn1_ber_decoder(&spki_decoder, &r, bytes, size) != -ECANCELED;
}
int main(void)
{
    if (check_oids()) { fprintf(stderr, "OID lookup/length/render mismatch\n"); return 1; }
    const u8 der[] = {0x30,0x15,0x30,0x0d,0x06,9,0x2a,0x86,0x48,0x86,0xf7,0x0d,1,1,1,
                      0x05,0,0x03,4,0,'k','e','y'};
    const u8 no_params[] = {0x30,0x13,0x30,0x0b,0x06,9,0x2a,0x86,0x48,0x86,0xf7,0x0d,1,1,1,
                            0x03,4,0,'k','e','y'};
    const u8 ber[] = {0x30,0x80,0x30,0x80,0x06,9,0x2a,0x86,0x48,0x86,0xf7,0x0d,1,1,1,
                      0x05,0,0,0,0x03,4,0,'k','e','y',0,0};
    if (check(der, sizeof(der), true) || check(no_params, sizeof(no_params), false) ||
        check(ber, sizeof(ber), true)) {
        fprintf(stderr, "ASN.1 envelope/callback/truncation mismatch\n");
        return 1;
    }
    u8 bad[sizeof(der)];
    memcpy(bad, der, sizeof(bad)); bad[2] = ASN1_SET | ASN1_CONS_BIT;
    struct result r = {0};
    if (asn1_ber_decoder(&spki_decoder, &r, bad, sizeof(bad)) != -EBADMSG)
        return 1;
    memcpy(bad, der, sizeof(bad)); bad[5] = 0x80; /* Primitive indefinite length. */
    if (asn1_ber_decoder(&spki_decoder, &r, bad, sizeof(bad)) != -EBADMSG)
        return 1;
    return asn1_ber_decoder(&spki_decoder, &r, der, 65536) != -EMSGSIZE;
}
