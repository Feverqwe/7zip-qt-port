#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

uint32_t crc32_le(uint32_t, const void *, size_t);
uint32_t crc32_be(uint32_t, const void *, size_t);
uint32_t crc32c(uint32_t, const void *, size_t);

struct vector {
    size_t size;
    uint32_t states[3];
    uint32_t castagnoli[3];
};

/* Frozen raw-state outputs of the original pinned Linux implementation. */
static const struct vector vectors[] = {
    {0,
     {UINT32_C(0x00000000), UINT32_C(0xffffffff), UINT32_C(0x12345678)},
     {UINT32_C(0x00000000), UINT32_C(0xffffffff), UINT32_C(0x12345678)}},
    {1,
     {UINT32_C(0x97d2d988), UINT32_C(0xba2fc9fa), UINT32_C(0xc91e14d0)},
     {UINT32_C(0x9989ab3b), UINT32_C(0x340b0795), UINT32_C(0x62da93af)}},
    {3,
     {UINT32_C(0xc8127c69), UINT32_C(0xc8ac5a84), UINT32_C(0x5fa54bf4)},
     {UINT32_C(0xf6e522fa), UINT32_C(0x697e7e7f), UINT32_C(0xc653a273)}},
    {7,
     {UINT32_C(0x7203e44f), UINT32_C(0x1090c4ce), UINT32_C(0xd9e210bb)},
     {UINT32_C(0x69cbb07d), UINT32_C(0x2d0a25ef), UINT32_C(0x79ec37a6)}},
    {8,
     {UINT32_C(0x01a972e2), UINT32_C(0x9b745274), UINT32_C(0xbbd2a513)},
     {UINT32_C(0x62a16c49), UINT32_C(0x1176213c), UINT32_C(0x28eb015e)}},
    {9,
     {UINT32_C(0xf1d54b30), UINT32_C(0xe823a061), UINT32_C(0x3bd5f26d)},
     {UINT32_C(0x1a18dd59), UINT32_C(0x5e024a05), UINT32_C(0xdec634b0)}},
    {15,
     {UINT32_C(0x84b35d04), UINT32_C(0xac9fa11c), UINT32_C(0xb54c3fde)},
     {UINT32_C(0xca229cba), UINT32_C(0x66d3b755), UINT32_C(0x9d3e7e65)}},
    {16,
     {UINT32_C(0xc853e2dd), UINT32_C(0xdb175677), UINT32_C(0xae642675)},
     {UINT32_C(0xcf7fd634), UINT32_C(0x72f0b321), UINT32_C(0x42e81759)}},
    {17,
     {UINT32_C(0x041375f7), UINT32_C(0x32037bb5), UINT32_C(0xdc7869e9)},
     {UINT32_C(0x3f1ad090), UINT32_C(0x1a088c86), UINT32_C(0xe17998e0)}},
    {257,
     {UINT32_C(0x998a109c), UINT32_C(0xd1ca4fad), UINT32_C(0xc3225a52)},
     {UINT32_C(0xf0d57902), UINT32_C(0xcf475b0a), UINT32_C(0x5a99f366)}},
    {4097,
     {UINT32_C(0xfa355ad1), UINT32_C(0xbdbf7651), UINT32_C(0x50e66272)},
     {UINT32_C(0x2f202131), UINT32_C(0x787e946a), UINT32_C(0x5a36a734)}},
    {65537,
     {UINT32_C(0x5d69bec9), UINT32_C(0x479b02c5), UINT32_C(0x441d7272)},
     {UINT32_C(0x77b30c99), UINT32_C(0xbf92424a), UINT32_C(0xab8a1e88)}},
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
    uint32_t (*const algorithms[])(uint32_t, const void *, size_t) = {crc32_le, crc32c};
    for (size_t offset = 0; offset < 8; ++offset) {
        for (size_t i = 0; i < 65537; ++i)
            data[offset + i] = (uint8_t)(i * 37 + 11);
        for (size_t v = 0; v < sizeof(vectors) / sizeof(vectors[0]); ++v) {
            for (size_t s = 0; s < sizeof(seeds) / sizeof(seeds[0]); ++s) {
                for (size_t a = 0; a < sizeof(algorithms) / sizeof(algorithms[0]); ++a) {
                    const uint32_t expected = a ? vectors[v].castagnoli[s] : vectors[v].states[s];
                    if (check(algorithms[a](seeds[s], data + offset, vectors[v].size), expected))
                        return 1;
                    for (size_t c = 0; c < sizeof(chunks) / sizeof(chunks[0]); ++c) {
                        uint32_t crc = seeds[s];
                        if (check(algorithms[a](crc, NULL, 0), crc))
                            return 1;
                        for (size_t pos = 0; pos < vectors[v].size;) {
                            size_t count = vectors[v].size - pos;
                            if (count > chunks[c])
                                count = chunks[c];
                            crc = algorithms[a](crc, data + offset + pos, count);
                            if (check(algorithms[a](crc, NULL, 0), crc))
                                return 1;
                            pos += count;
                        }
                        if (check(crc, expected))
                            return 1;
                    }
                }
            }
        }
    }
    /* Guard against confusing the IEEE and Castagnoli polynomials or bit order. */
    return check(crc32_le(UINT32_MAX, "123456789", 9), UINT32_C(0x340bc6d9)) ||
           check(crc32_be(UINT32_MAX, "123456789", 9), UINT32_C(0x0376e6e7)) ||
           check(crc32c(UINT32_MAX, "123456789", 9), UINT32_C(0x1cf96d7c));
}
