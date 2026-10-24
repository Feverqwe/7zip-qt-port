// Extracted from the corresponding 7-Zip handler; retained LGPL notices apply.
#include "StdAfx.h"

#include "../../../C/CpuArch.h"
#include "../../Common/MyBuffer.h"
#include "../Common/StreamUtils.h"
#include "FatHeader.h"

namespace NArchive {
namespace NMbr {
static const Byte k_Ntfs_Signature[] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' ', 0};

static bool Is_Ntfs(const Byte *p) {
    if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
        return false;
    switch (p[0]) {
    case 0xE9: /* codeOffset = 3 + (Int16)Get16(p + 1); */
        break;
    case 0xEB:
        if (p[2] != 0x90)
            return false; /* codeOffset = 2 + (int)(signed char)p[1]; */
        break;
    default:
        return false;
    }
    return memcmp(p + 3, k_Ntfs_Signature, Z7_ARRAY_SIZE(k_Ntfs_Signature)) == 0;
}

static const Byte k_ExFat_Signature[] = {'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '};

static bool Is_ExFat(const Byte *p) {
    if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
        return false;
    if (p[0] != 0xEB || p[1] != 0x76 || p[2] != 0x90)
        return false;
    return memcmp(p + 3, k_ExFat_Signature, Z7_ARRAY_SIZE(k_ExFat_Signature)) == 0;
}

static bool AllAreZeros(const Byte *p, size_t size) {
    for (size_t i = 0; i < size; i++)
        if (p[i] != 0)
            return false;
    return true;
}

static const UInt32 k_Udf_StartPos = 0x8000;
static const Byte k_Udf_Signature[] = {0, 'B', 'E', 'A', '0', '1', 1, 0};

static bool Is_Udf(const Byte *p) {
    return memcmp(p + k_Udf_StartPos, k_Udf_Signature, sizeof(k_Udf_Signature)) == 0;
}

const char *GetFileSystem(ISequentialInStream *stream, UInt64 partitionSize, HRESULT &status) {
    status = S_OK;
    const size_t kHeaderSize = 1 << 9;
    if (partitionSize >= kHeaderSize) {
        Byte buf[kHeaderSize];
        status = ReadStream_FAIL(stream, buf, kHeaderSize);
        if (status == S_OK) {
            // NTFS is expected default filesystem for (Type == 7)
            if (Is_Ntfs(buf))
                return "NTFS";
            if (Is_ExFat(buf))
                return "exFAT";
            if (NFat::IsArc_Fat(buf, kHeaderSize))
                return "FAT";
            const size_t kHeaderSize2 = k_Udf_StartPos + (1 << 9);
            if (partitionSize >= kHeaderSize2) {
                if (AllAreZeros(buf, kHeaderSize)) {
                    CByteBuffer buffer(kHeaderSize2);
                    // memcpy(buffer, buf, kHeaderSize);
                    status =
                        ReadStream_FAIL(stream, buffer + kHeaderSize, kHeaderSize2 - kHeaderSize);
                    if (status == S_OK)
                        if (Is_Udf(buffer))
                            return "UDF";
                }
            }
        }
    }
    return NULL;
}

} // namespace NMbr
} // namespace NArchive
