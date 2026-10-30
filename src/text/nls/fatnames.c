/* SPDX-License-Identifier: GPL-2.0-only */
/* FAT integration formerly colocated with the Linux CP437 charset table. */
#include "text/nls/nls.h"

int fat_linux_charset_available(void) { return fat_cp437_table() != NULL; }

int fat_linux_oem_name(const unsigned char *bytes, unsigned int length, char *output,
                       int capacity) {
    fat_wchar_t name[16];
    struct nls_table *table = fat_cp437_table();
    if (!table || length > ARRAY_SIZE(name))
        return -EINVAL;
    for (unsigned i = 0; i < length; ++i)
        if (table->char2uni(bytes + i, 1, name + i) < 0)
            name[i] = '?';
    return utf16s_to_utf8s(name, (int)length, UTF16_HOST_ENDIAN, (u8 *)output, capacity);
}
