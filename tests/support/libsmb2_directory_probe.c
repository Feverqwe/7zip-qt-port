// Diagnostic for the external server encoder, not a product SMB implementation.
// MS-FSCC FILE_ID_FULL_DIR_INFORMATION: NextEntryOffset is relative to this entry.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>

// These external headers are order dependent.
// clang-format off
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include <smb2/libsmb2-raw.h>
#include "compat.h"
#include "libsmb2/include/libsmb2-private.h"
// clang-format on

static int check_directory(uint32_t room, uint8_t info_class, uint32_t fixed) {
    const char *names[] = {"one1.txt", "two2.txt", "end3.txt"};
    struct smb2_context *context = smb2_init_context();
    if (!context)
        return 2;
    const size_t stride = PAD_TO_64BIT(sizeof(struct smb2_fileidbothdirectoryinformation));
    uint8_t *entries = calloc(3, stride);
    if (!entries) {
        smb2_destroy_context(context);
        return 2;
    }
    for (size_t i = 0; i < 3; ++i) {
        struct smb2_fileidbothdirectoryinformation *entry = (void *)(entries + i * stride);
        entry->name = names[i];
        entry->file_index = (uint32_t)i;
        entry->file_id = i + 1;
    }
    struct smb2_query_directory_request request = {0};
    request.file_information_class = info_class;
    request.output_buffer_length = room;
    struct smb2_query_directory_reply reply = {0};
    reply.output_buffer = entries;
    reply.output_buffer_length = (uint32_t)(3 * stride);
    struct smb2_pdu *pdu =
        smb2_cmd_query_directory_reply_async(context, &request, &reply, NULL, NULL);
    free(entries);
    if (!pdu) {
        fprintf(stderr, "encoder failed: %s\n", smb2_get_error(context));
        smb2_destroy_context(context);
        return 2;
    }
    // Private vectors are inspected only to obtain the library's emitted bytes.
    // Decoding uses the library's own decoder; the probe never services requests.
    size_t size = pdu->out.total_size;
    uint8_t *wire = malloc(size);
    if (!wire) {
        smb2_free_pdu(context, pdu);
        smb2_destroy_context(context);
        return 2;
    }
    size_t copied = 0;
    for (int i = 0; i < pdu->out.niov; ++i) {
        memcpy(wire + copied, pdu->out.iov[i].buf, pdu->out.iov[i].len);
        copied += pdu->out.iov[i].len;
    }
    struct smb2_iovec header = {.buf = wire + SMB2_HEADER_SIZE, .len = size - SMB2_HEADER_SIZE};
    uint16_t offset;
    uint32_t length;
    offset = (uint16_t)(header.buf[2] | ((uint16_t)header.buf[3] << 8));
    smb2_get_uint32(&header, 4, &length);
    int valid = offset <= size && length <= size - offset && length <= room;
    size_t cursor = offset;
    size_t count = 0;
    while (cursor < size && count < 3) {
        struct smb2_iovec buffer = {.buf = wire + cursor, .len = size - cursor};
        uint32_t name_length, next;
        if (buffer.len < fixed) {
            valid = 0;
            break;
        }
        smb2_get_uint32(&buffer, 0, &next);
        smb2_get_uint32(&buffer, 60, &name_length);
        if ((name_length & 1) || name_length > buffer.len - fixed) {
            valid = 0;
            break;
        }
        const char *name =
            smb2_utf16_to_utf8((const uint16_t *)(void *)(buffer.buf + fixed), name_length / 2);
        valid &= name && strcmp(name, names[count]) == 0;
        free((void *)name);
        ++count;
        if (!next)
            break;
        if (next % 8 || next > size - cursor) {
            valid = 0;
            break;
        }
        cursor += next;
    }
    const uint32_t entry_size = PAD_TO_64BIT(fixed + 16);
    valid &= count == (room >= 3 * entry_size ? 3 : 1);
    printf("class=%u room=%u emitted=%u decoded=%zu result=%s\n", info_class, room, length, count,
           valid ? "PASS" : "FAIL");
    free(wire);
    smb2_free_pdu(context, pdu);
    smb2_destroy_context(context);
    return valid ? 0 : 1;
}

static int check_timestamps(void) {
    const uint64_t dates[] = {0, UINT64_C(116444736000000000), UINT64_C(133800000000000000)};
    struct smb2_context *context = smb2_init_context();
    if (!context)
        return 2;
    struct smb2_server role = {0};
    context->owning_server = &role;
    int valid = 1;
    for (size_t i = 0; i < sizeof(dates) / sizeof(dates[0]); ++i) {
        struct smb2_file_basic_info info = {0};
        smb2_win_to_timeval(dates[i], &info.creation_time);
        valid &= smb2_timeval_to_win(&info.creation_time) == dates[i];
        if (!i)
            valid &= info.creation_time.tv_sec < 0;
        uint8_t bytes[40] = {0};
        struct smb2_iovec wire = {.buf = bytes, .len = sizeof(bytes)};
        valid &= smb2_encode_file_basic_info(context, &info, &wire) >= 0;
        uint64_t encoded = 0;
        smb2_get_uint64(&wire, 0, &encoded);
        valid &= encoded == dates[i];
    }
    // A client's zero SET_INFO time still means "leave unchanged".
    context->owning_server = NULL;
    struct smb2_file_basic_info unchanged = {0};
    uint8_t bytes[40] = {0};
    struct smb2_iovec wire = {.buf = bytes, .len = sizeof(bytes)};
    valid &= smb2_encode_file_basic_info(context, &unchanged, &wire) >= 0;
    uint64_t encoded = 1;
    smb2_get_uint64(&wire, 0, &encoded);
    valid &= encoded == 0;
    smb2_destroy_context(context);
    printf("query timestamps and SET_INFO sentinel result=%s\n", valid ? "PASS" : "FAIL");
    return valid ? 0 : 1;
}

int main(void) {
    // Fixed offsets are MS-FSCC wire contracts, independent of C struct layout.
    const struct {
        uint8_t type;
        uint32_t fixed;
    } formats[] = {{1, 64}, {2, 68}, {3, 94}, {0x25, 104}, {0x26, 80}};
    int failed = check_timestamps();
    for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
        failed |= check_directory(4096, formats[i].type, formats[i].fixed);
        failed |=
            check_directory(PAD_TO_64BIT(formats[i].fixed + 16), formats[i].type, formats[i].fixed);
    }
    return failed;
}
