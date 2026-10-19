// SPDX-License-Identifier: GPL-2.0-only
/* Static service boundary for the original Linux NLS charset tables. */
#include "nls.h"
#include "../../isonames.h"

struct nls_table *sevenzip_mac_celtic(void);
struct nls_table *sevenzip_mac_centeuro(void);
struct nls_table *sevenzip_mac_croatian(void);
struct nls_table *sevenzip_mac_cyrillic(void);
struct nls_table *sevenzip_mac_gaelic(void);
struct nls_table *sevenzip_mac_greek(void);
struct nls_table *sevenzip_mac_iceland(void);
struct nls_table *sevenzip_mac_inuit(void);
struct nls_table *sevenzip_mac_roman(void);
struct nls_table *sevenzip_mac_romanian(void);
struct nls_table *sevenzip_mac_turkish(void);
struct nls_table *sevenzip_nls_ascii(void);
struct nls_table *sevenzip_nls_cp1250(void);
struct nls_table *sevenzip_nls_cp1251(void);
struct nls_table *sevenzip_nls_cp1255(void);
struct nls_table *sevenzip_nls_cp737(void);
struct nls_table *sevenzip_nls_cp775(void);
struct nls_table *sevenzip_nls_cp850(void);
struct nls_table *sevenzip_nls_cp852(void);
struct nls_table *sevenzip_nls_cp855(void);
struct nls_table *sevenzip_nls_cp857(void);
struct nls_table *sevenzip_nls_cp860(void);
struct nls_table *sevenzip_nls_cp861(void);
struct nls_table *sevenzip_nls_cp862(void);
struct nls_table *sevenzip_nls_cp863(void);
struct nls_table *sevenzip_nls_cp864(void);
struct nls_table *sevenzip_nls_cp865(void);
struct nls_table *sevenzip_nls_cp866(void);
struct nls_table *sevenzip_nls_cp869(void);
struct nls_table *sevenzip_nls_cp874(void);
struct nls_table *sevenzip_nls_cp932(void);
struct nls_table *sevenzip_nls_cp936(void);
struct nls_table *sevenzip_nls_cp949(void);
struct nls_table *sevenzip_nls_cp950(void);
struct nls_table *sevenzip_nls_euc_jp(void);
struct nls_table *sevenzip_nls_iso8859_1(void);
struct nls_table *sevenzip_nls_iso8859_13(void);
struct nls_table *sevenzip_nls_iso8859_14(void);
struct nls_table *sevenzip_nls_iso8859_15(void);
struct nls_table *sevenzip_nls_iso8859_2(void);
struct nls_table *sevenzip_nls_iso8859_3(void);
struct nls_table *sevenzip_nls_iso8859_4(void);
struct nls_table *sevenzip_nls_iso8859_5(void);
struct nls_table *sevenzip_nls_iso8859_6(void);
struct nls_table *sevenzip_nls_iso8859_7(void);
struct nls_table *sevenzip_nls_iso8859_9(void);
struct nls_table *sevenzip_nls_koi8_r(void);
struct nls_table *sevenzip_nls_koi8_ru(void);
struct nls_table *sevenzip_nls_koi8_u(void);
struct nls_table *sevenzip_nls_default(void);

static struct nls_table *(*const tables[])(void) = {
    fat_cp437_table,
    sevenzip_mac_celtic,
    sevenzip_mac_centeuro,
    sevenzip_mac_croatian,
    sevenzip_mac_cyrillic,
    sevenzip_mac_gaelic,
    sevenzip_mac_greek,
    sevenzip_mac_iceland,
    sevenzip_mac_inuit,
    sevenzip_mac_roman,
    sevenzip_mac_romanian,
    sevenzip_mac_turkish,
    sevenzip_nls_ascii,
    sevenzip_nls_cp1250,
    sevenzip_nls_cp1251,
    sevenzip_nls_cp1255,
    sevenzip_nls_cp737,
    sevenzip_nls_cp775,
    sevenzip_nls_cp850,
    sevenzip_nls_cp852,
    sevenzip_nls_cp855,
    sevenzip_nls_cp857,
    sevenzip_nls_cp860,
    sevenzip_nls_cp861,
    sevenzip_nls_cp862,
    sevenzip_nls_cp863,
    sevenzip_nls_cp864,
    sevenzip_nls_cp865,
    sevenzip_nls_cp866,
    sevenzip_nls_cp869,
    sevenzip_nls_cp874,
    sevenzip_nls_cp932,
    sevenzip_nls_cp936,
    sevenzip_nls_cp949,
    sevenzip_nls_cp950,
    sevenzip_nls_euc_jp,
    sevenzip_nls_iso8859_1,
    sevenzip_nls_iso8859_13,
    sevenzip_nls_iso8859_14,
    sevenzip_nls_iso8859_15,
    sevenzip_nls_iso8859_2,
    sevenzip_nls_iso8859_3,
    sevenzip_nls_iso8859_4,
    sevenzip_nls_iso8859_5,
    sevenzip_nls_iso8859_6,
    sevenzip_nls_iso8859_7,
    sevenzip_nls_iso8859_9,
    sevenzip_nls_koi8_r,
    sevenzip_nls_koi8_ru,
    sevenzip_nls_koi8_u,
    sevenzip_nls_default,
};

struct nls_table *sevenzip_nls_table(int charset)
{
    return charset > 0 && (unsigned)charset <= ARRAY_SIZE(tables)
        ? tables[charset - 1]() : NULL;
}

/* find_nls charset/alias comparison, using linked-in tables instead of
 * module loading, linked-list mutation and module-reference lifetime. */
int sevenzip_iso_charset(const char *charset)
{
    unsigned i;
    if (!strcmp(charset, "utf8")) return 0;
    for (i = 0; i < ARRAY_SIZE(tables); ++i) {
        struct nls_table *nls = tables[i]();
        if (!strcmp(nls->charset, charset)) return (int)i + 1;
        if (nls->alias && !strcmp(nls->alias, charset)) return (int)i + 1;
    }
    return -EINVAL;
}
