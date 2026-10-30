/* SPDX-License-Identifier: GPL-2.0-only */
#include "formats/mbr/userspace.h"
#include <errno.h>

void *read_part_sector(struct parsed_partitions *state, sector_t sector, Sector *owner)
{
    owner->data = NULL;
    if (state->error || sector >= state->bytes / 512)
        return NULL;
    owner->data = malloc(512);
    if (!owner->data) {
        state->error = -ENOMEM;
        return NULL;
    }
    state->error = state->read(state->context, sector * 512, owner->data, 512);
    if (state->error) {
        free(owner->data);
        owner->data = NULL;
    }
    return owner->data;
}

void sevenzip_msdos_log(void *buffer, const char *text, ...)
{
    (void)buffer;
    (void)text;
}

int32_t sevenzip_msdos_scan(void *context, sevenzip_msdos_read read, uint64_t bytes,
    const struct sevenzip_msdos_parent *parents, unsigned count, unsigned logical_count,
    struct sevenzip_msdos_part *parts, unsigned *part_count)
{
    struct parsed_partitions state = {0};
    unsigned first, i;
    int result;
    *part_count = 0;
    if (!read || !parents || !parts || count > 4 || logical_count > 251)
        return -EINVAL;
    state.context = context;
    state.read = read;
    state.bytes = bytes;
    state.next = (int)(first = 5 + logical_count);
    state.limit = 256;
    result = sevenzip_msdos_subtypes(&state, parents, count);
    if (result)
        return result;
    for (i = first; i < (unsigned)state.next; ++i)
        parts[(*part_count)++] = state.parts[i];
    return 0;
}
