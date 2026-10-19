// SPDX-License-Identifier: GPL-2.0-only
#include "fs/hfsplus/hfscompat.h"

int sevenzip_hfsplus_compare(const void *a, const void *b, int binary)
{
    return binary ? hfsplus_strcmp(a, b) : hfsplus_strcasecmp(a, b);
}

int sevenzip_hfsplus_private_name(const void *name, int binary)
{
    /* HFSP_HIDDENDIR_NAME, looked up by hfsplus_fill_super/get_hidden_dir_entry. */
    static const char suffix[] = "HFS+ Private Data";
    struct hfsplus_unistr key = {0};
    ((u8 *)&key.length)[1] = 4 + sizeof(suffix) - 1;
    for (unsigned i = 0; i < sizeof(suffix) - 1; i++)
        ((u8 *)&key.unicode[4 + i])[1] = (u8)suffix[i];
    return sevenzip_hfsplus_compare(name, &key, binary) == 0;
}

/* Keep the engine's UTF-16 representation, including surrogate pairs. NLS
 * encoding is a platform boundary; composition and disk-name mapping stay in
 * the original Linux converter. */
static int utf16_unit(u16 value, unsigned char *output, int capacity)
{
    if (capacity < 2)
        return -ENAMETOOLONG;
    output[0] = (u8)(value >> 8);
    output[1] = (u8)value;
    return 2;
}

int sevenzip_hfsplus_name(const void *data, unsigned length, int xattr,
                         uint16_t *output, unsigned *output_length)
{
    struct hfsplus_unistr name;
    struct nls_table nls = {utf16_unit};
    struct super_block sb = {&nls, 0};
    unsigned char encoded[HFSPLUS_MAX_STRLEN * 2];
    int size = sizeof(encoded), result;
    if (!output || !output_length || (!data && length) ||
        length > (xattr ? HFSPLUS_ATTR_MAX_STRLEN : HFSPLUS_MAX_STRLEN))
        return -EINVAL;
    ((u8 *)&name.length)[0] = (u8)(length >> 8);
    ((u8 *)&name.length)[1] = (u8)length;
    if (length)
        memcpy(name.unicode, data, length * 2);
    result = xattr ? hfsplus_uni2asc_xattr_str(&sb, (const struct hfsplus_attr_unistr *)&name,
                                             (char *)encoded, &size)
                   : hfsplus_uni2asc_str(&sb, &name, (char *)encoded, &size);
    if (result)
        return result;
    if ((unsigned)size / 2 > *output_length)
        return -ENAMETOOLONG;
    *output_length = (unsigned)size / 2;
    for (unsigned i = 0; i < *output_length; i++)
        output[i] = (u16)((u16)encoded[i * 2] << 8 | encoded[i * 2 + 1]);
    return 0;
}
