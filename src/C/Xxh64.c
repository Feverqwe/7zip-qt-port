/* Xxh64.c -- XXH64 hash calculation
original code: Copyright (c) Yann Collet.
modified by Igor Pavlov.
This source code is licensed under BSD 2-Clause License.
*/

/* Preserve SDK block/tail streaming; the pinned libzstd xxHash supplies hashing. */
#include "Precomp.h"

#include "Xxh64.h"
#include <string.h>
#define XXH_STATIC_LINKING_ONLY
#include "../zstd/lib/common/xxhash.h"

/* SDK exposes four accumulator lanes and supplies the total size at digest time.
   Translate that state to the pinned static library without changing SDK layouts. */
static void Xxh64_Import(XXH64_state_t *state, const CXxh64State *p) {
    XXH64_reset(state, 0);
    memcpy(state->v, p->v, sizeof(p->v));
}

void Xxh64State_Init(CXxh64State *p) {
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    memcpy(p->v, state.v, sizeof(p->v));
}

void Z7_FASTCALL Xxh64State_UpdateBlocks(CXxh64State *p, const void *data, const void *end) {
    XXH64_state_t state;
    Xxh64_Import(&state, p);
    XXH64_update(&state, data, (const Byte *)end - (const Byte *)data);
    memcpy(p->v, state.v, sizeof(p->v));
}

UInt64 Xxh64State_Digest(const CXxh64State *p, const void *data, UInt64 count) {
    XXH64_state_t state;
    Xxh64_Import(&state, p);
    state.total_len = count;
    state.memsize = (unsigned)(count & (Z7_XXH64_BLOCK_SIZE - 1));
    if (state.memsize)
        memcpy(state.mem64, data, state.memsize);
    return XXH64_digest(&state);
}

void Xxh64_Init(CXxh64 *p) {
    Xxh64State_Init(&p->state);
    p->count = 0;
    p->buf64[0] = 0;
    p->buf64[1] = 0;
    p->buf64[2] = 0;
    p->buf64[3] = 0;
}

void Xxh64_Update(CXxh64 *p, const void *_data, size_t size) {
    const Byte *data = (const Byte *)_data;
    unsigned cnt;
    if (size == 0)
        return;
    cnt = (unsigned)p->count;
    p->count += size;

    if (cnt &= 31) {
        unsigned rem = 32 - cnt;
        Byte *dest = (Byte *)p->buf64 + cnt;
        if (rem > size)
            rem = (unsigned)size;
        size -= rem;
        cnt += rem;
        do
            *dest++ = *data++;
        while (--rem);
        if (cnt != 32)
            return;
        Xxh64State_UpdateBlocks(&p->state, p->buf64, &p->buf64[4]);
    }

    if (size &= ~(size_t)31) {
        Xxh64State_UpdateBlocks(&p->state, data, data + size);
        data += size;
    }

    cnt = (unsigned)p->count & 31;
    if (cnt) {
        Byte *dest = (Byte *)p->buf64;
        do
            *dest++ = *data++;
        while (--cnt);
    }
}
