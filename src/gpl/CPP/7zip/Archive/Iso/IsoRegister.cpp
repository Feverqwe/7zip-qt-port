// IsoRegister.cpp

#include "7zip/Archive/Iso/StdAfx.h"

#include "7zip/Common/RegisterArc.h"

#include "7zip/Archive/Iso/IsoHandler.h"

namespace NArchive {
namespace NIso {

static UInt32 IsArcIso(const Byte *data, size_t size) {
    for (unsigned sector = 16; sector < 100; ++sector) {
        const size_t offset = (size_t)sector * 2048;
        if (size < offset + 15)
            return k_IsArc_Res_NEED_MORE;
        if (memcmp(data + offset + 1, "CD001", 5) == 0)
            return k_IsArc_Res_YES;
    }
    return k_IsArc_Res_NO;
}

REGISTER_ARC_I_NO_SIG("Iso", "iso img", NULL, 0xE7, 0, 0, IsArcIso)

} // namespace NIso
} // namespace NArchive
