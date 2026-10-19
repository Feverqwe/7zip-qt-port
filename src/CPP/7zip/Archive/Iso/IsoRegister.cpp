// IsoRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "IsoHandler.h"

namespace NArchive {
namespace NIso {

#if defined(SEVENZIP_LINUX_ISO) || !defined(Z7_EXTERNAL_CODECS)
#ifdef SEVENZIP_LINUX_ISO
// A fixed signature offset cannot describe both ISO9660 and High Sierra.
// Keep the original handler identity and let the normal IsArc callback probe
// their descriptor headers; Open performs the bounded descriptor scan.
static UInt32 IsArcIso(const Byte *data, size_t size) {
    for (unsigned sector = 16; sector < 100; ++sector) {
        const size_t offset = (size_t)sector * 2048;
        if (size < offset + 15)
            return k_IsArc_Res_NEED_MORE;
        if (memcmp(data + offset + 1, "CD001", 5) == 0 ||
            memcmp(data + offset + 9, "CDROM", 5) == 0)
            return k_IsArc_Res_YES;
    }
    return k_IsArc_Res_NO;
}

REGISTER_ARC_I_NO_SIG("Iso", "iso img", NULL, 0xE7, 0, 0, IsArcIso)
#else
static const Byte k_Signature[] = {'C', 'D', '0', '0', '1'};

REGISTER_ARC_I("Iso", "iso img", NULL, 0xE7, k_Signature, NArchive::NIso::kStartPos + 1, 0, NULL)
#endif
#endif

} // namespace NIso
} // namespace NArchive
