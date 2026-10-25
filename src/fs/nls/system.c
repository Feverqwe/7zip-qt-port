/* SPDX-License-Identifier: GPL-2.0-only */
/* Local OS-service boundary; charset mappings belong to the operating system. */
#include "system.h"
#include <errno.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iconv.h>
#include <locale.h>
#include <wctype.h>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#endif

static int valid_unit(fat_wchar_t value) { return value && (value < 0xd800 || value > 0xdfff); }

#ifndef _WIN32
static iconv_t open_converter(const struct nls_system_charset *charset, int encode) {
    const char *name = charset->iconv_name;
    iconv_t converter = iconv_open(encode ? name : "UTF-16LE", encode ? "UTF-16LE" : name);
    if (converter == (iconv_t)-1 && charset->iconv_alias) {
        name = charset->iconv_alias;
        converter = iconv_open(encode ? name : "UTF-16LE", encode ? "UTF-16LE" : name);
    }
    return converter;
}

static int convert(const struct nls_system_charset *charset, int encode,
                   const unsigned char *source, size_t size, unsigned char *target,
                   size_t capacity) {
    iconv_t converter = open_converter(charset, encode);
    if (converter == (iconv_t)-1)
        return -EINVAL;
    char *input = (char *)source;
    char *output = (char *)target;
    size_t remaining = capacity;
    size_t result = iconv(converter, &input, &size, &output, &remaining);
    /* glibc's Hebrew converter can retain a final unit for composition.
     * Flush before closing, even for these otherwise stateless legacy charsets. */
    if (result != (size_t)-1 && result == 0 && !size)
        result = iconv(converter, NULL, NULL, &output, &remaining);
    iconv_close(converter);
    if (result == (size_t)-1 || result != 0 || size)
        return -EINVAL;
    return (int)(capacity - remaining);
}
#endif

int nls_system_available(const struct nls_system_charset *charset) {
#ifdef _WIN32
    return charset->windows_codepage && IsValidCodePage(charset->windows_codepage);
#else
    iconv_t encode = open_converter(charset, 1);
    iconv_t decode = open_converter(charset, 0);
    int available = encode != (iconv_t)-1 && decode != (iconv_t)-1;
    if (encode != (iconv_t)-1)
        iconv_close(encode);
    if (decode != (iconv_t)-1)
        iconv_close(decode);
    return available;
#endif
}

static int decode_unit(const struct nls_system_charset *charset, const unsigned char *input,
                       int size, fat_wchar_t *unit) {
#ifdef _WIN32
    WCHAR output[2];
    int count = MultiByteToWideChar(charset->windows_codepage, MB_ERR_INVALID_CHARS,
                                    (const char *)input, size, output, 2);
    if (count != 1 || !valid_unit(output[0]))
        return -EINVAL;
    *unit = output[0];
#else
    unsigned char output[2];
    if (convert(charset, 0, input, (size_t)size, output, sizeof(output)) != 2)
        return -EINVAL;
    fat_wchar_t value = (fat_wchar_t)(output[0] | (unsigned)output[1] << 8);
    if (!valid_unit(value))
        return -EINVAL;
    *unit = value;
#endif
    return size;
}

int nls_system_decode(const struct nls_system_charset *charset, const unsigned char *input,
                      int capacity, fat_wchar_t *unit) {
    if (capacity <= 0)
        return -ENAMETOOLONG;
    /* The exposed legacy encodings have at most three bytes per BMP unit.
     * Return one character and leave the next character for the upstream caller. */
    for (int count = 1; count <= capacity && count <= 3; ++count)
        if (decode_unit(charset, input, count, unit) > 0)
            return count;
    return -EINVAL;
}

int nls_system_encode(const struct nls_system_charset *charset, fat_wchar_t unit,
                      unsigned char *output, int capacity) {
    unsigned char converted[NLS_MAX_CHARSET_SIZE];
    int count;
    if (capacity <= 0)
        return -ENAMETOOLONG;
    if (!valid_unit(unit))
        return -EINVAL;
#ifdef _WIN32
    WCHAR input = unit;
    BOOL used_default = FALSE;
    count = WideCharToMultiByte(charset->windows_codepage, 0, &input, 1, (char *)converted,
                                sizeof(converted), NULL, &used_default);
    if (!count || used_default)
        return -EINVAL;
#else
    unsigned char input[2] = {(unsigned char)unit, (unsigned char)(unit >> 8)};
    count = convert(charset, 1, input, sizeof(input), converted, sizeof(converted));
    if (count <= 0)
        return -EINVAL;
#endif
    /* Some system converters silently approximate or expand characters even
     * without requesting transliteration. Do not publish those as exact names. */
    fat_wchar_t check;
    if (nls_system_decode(charset, converted, count, &check) != count || check != unit)
        return -EINVAL;
    if (count > capacity)
        return -ENAMETOOLONG;
    memcpy(output, converted, (size_t)count);
    return count;
}

unsigned char fat_cp437_lower(unsigned char byte) {
    static const struct nls_system_charset cp437 = {"CP437", NULL, 437};
    fat_wchar_t unit;
    unsigned char result;
    if (nls_system_decode(&cp437, &byte, 1, &unit) != 1)
        return byte;
#ifdef _WIN32
    WCHAR input = unit, output[2];
    if (LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, &input, 1, output, 2, NULL, NULL,
                      0) != 1)
        return byte;
    unit = output[0];
#else
    /* Explicit locale keeps filename conversion independent of the UI/user locale. */
#ifdef __APPLE__
    locale_t locale = newlocale(LC_CTYPE_MASK, "en_US.UTF-8", (locale_t)0);
#else
    locale_t locale = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
#endif
    if (!locale)
        return byte;
    wint_t lower = towlower_l(unit, locale);
    freelocale(locale);
    if (lower > 0xffff)
        return byte;
    unit = (fat_wchar_t)lower;
#endif
    return nls_system_encode(&cp437, unit, &result, 1) == 1 ? result : byte;
}
