/* SPDX-License-Identifier: GPL-2.0-only */
/* Local registration boundary for the original Linux NLS caller contract. */
#include "formats/iso/isonames.h"
#include "text/nls/nls.h"
#include "text/nls/system.h"

#define NLS_CHARSET(key, name, alias, iconv, alternate, windows)                                   \
    static const struct nls_system_charset system_##key = {iconv, alternate, windows};             \
    static int encode_##key(fat_wchar_t unit, unsigned char *output, int capacity) {               \
        return nls_system_encode(&system_##key, unit, output, capacity);                           \
    }                                                                                              \
    static int decode_##key(const unsigned char *input, int capacity, fat_wchar_t *unit) {         \
        return nls_system_decode(&system_##key, input, capacity, unit);                            \
    }
#include "text/nls/charsets.inc"
#undef NLS_CHARSET

static struct nls_table tables[] = {
#define NLS_CHARSET(key, name, alias, iconv, alternate, windows)                                   \
    {name, encode_##key, decode_##key, NULL, NULL, alias},
#include "text/nls/charsets.inc"
#undef NLS_CHARSET
};

static const struct nls_system_charset *const services[] = {
#define NLS_CHARSET(key, name, alias, iconv, alternate, windows) &system_##key,
#include "text/nls/charsets.inc"
#undef NLS_CHARSET
};

struct nls_table *sevenzip_nls_table(int charset) {
    if (charset <= 0 || (unsigned)charset > ARRAY_SIZE(tables) ||
        !nls_system_available(services[charset - 1]))
        return NULL;
    return &tables[charset - 1];
}

struct nls_table *fat_cp437_table(void) { return sevenzip_nls_table(1); }

int sevenzip_iso_charset(const char *charset) {
    if (!strcmp(charset, "utf8"))
        return 0;
    for (unsigned i = 0; i < ARRAY_SIZE(tables); ++i) {
        struct nls_table *table = &tables[i];
        if (!strcmp(table->charset, charset) || (table->alias && !strcmp(table->alias, charset)))
            return nls_system_available(services[i]) ? (int)i + 1 : -EINVAL;
    }
    return -EINVAL;
}
