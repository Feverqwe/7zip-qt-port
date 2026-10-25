#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

uint32_t crc32_le(uint32_t, const void *, size_t);
uint32_t crc32_be(uint32_t, const void *, size_t);
uint32_t crc32c(uint32_t, const void *, size_t);

struct vector {
    size_t size;
    uint32_t states[3];
};

/* Frozen raw-state outputs of the original pinned Linux implementation. */
static const struct vector vectors[] = {
    {0, {UINT32_C(0x00000000), UINT32_C(0xffffffff), UINT32_C(0x12345678)}},
    {1, {UINT32_C(0x97d2d988), UINT32_C(0xba2fc9fa), UINT32_C(0xc91e14d0)}},
    {3, {UINT32_C(0xc8127c69), UINT32_C(0xc8ac5a84), UINT32_C(0x5fa54bf4)}},
    {7, {UINT32_C(0x7203e44f), UINT32_C(0x1090c4ce), UINT32_C(0xd9e210bb)}},
    {8, {UINT32_C(0x01a972e2), UINT32_C(0x9b745274), UINT32_C(0xbbd2a513)}},
    {9, {UINT32_C(0xf1d54b30), UINT32_C(0xe823a061), UINT32_C(0x3bd5f26d)}},
    {15, {UINT32_C(0x84b35d04), UINT32_C(0xac9fa11c), UINT32_C(0xb54c3fde)}},
    {16, {UINT32_C(0xc853e2dd), UINT32_C(0xdb175677), UINT32_C(0xae642675)}},
    {17, {UINT32_C(0x041375f7), UINT32_C(0x32037bb5), UINT32_C(0xdc7869e9)}},
    {257, {UINT32_C(0x998a109c), UINT32_C(0xd1ca4fad), UINT32_C(0xc3225a52)}},
    {4097, {UINT32_C(0xfa355ad1), UINT32_C(0xbdbf7651), UINT32_C(0x50e66272)}},
    {65537, {UINT32_C(0x5d69bec9), UINT32_C(0x479b02c5), UINT32_C(0x441d7272)}},
};

static int check(uint32_t actual, uint32_t expected) {
    if (actual == expected)
        return 0;
    fprintf(stderr, "CRC state: got %08x, expected %08x\n", (unsigned)actual, (unsigned)expected);
    return 1;
}

int main(void) {
    uint8_t data[65537 + 8];
    const uint32_t seeds[] = {0, UINT32_MAX, UINT32_C(0x12345678)};
    const size_t chunks[] = {1, 8, 31, 4096};
    for (size_t offset = 0; offset < 8; ++offset) {
        for (size_t i = 0; i < 65537; ++i)
            data[offset + i] = (uint8_t)(i * 37 + 11);
        for (size_t v = 0; v < sizeof(vectors) / sizeof(vectors[0]); ++v) {
            for (size_t s = 0; s < sizeof(seeds) / sizeof(seeds[0]); ++s) {
                if (check(crc32_le(seeds[s], data + offset, vectors[v].size), vectors[v].states[s]))
                    return 1;
                for (size_t c = 0; c < sizeof(chunks) / sizeof(chunks[0]); ++c) {
                    uint32_t crc = seeds[s];
                    if (check(crc32_le(crc, NULL, 0), crc))
                        return 1;
                    for (size_t pos = 0; pos < vectors[v].size;) {
                        size_t count = vectors[v].size - pos;
                        if (count > chunks[c])
                            count = chunks[c];
                        crc = crc32_le(crc, data + offset + pos, count);
                        if (check(crc32_le(crc, NULL, 0), crc))
                            return 1;
                        pos += count;
                    }
                    if (check(crc, vectors[v].states[s]))
                        return 1;
                }
            }
        }
    }
    /* Guard against confusing the IEEE and Castagnoli polynomials or bit order. */
    return check(crc32_le(UINT32_MAX, "123456789", 9), UINT32_C(0x340bc6d9)) ||
           check(crc32_be(UINT32_MAX, "123456789", 9), UINT32_C(0x0376e6e7)) ||
           check(crc32c(UINT32_MAX, "123456789", 9), UINT32_C(0x1cf96d7c));
}
