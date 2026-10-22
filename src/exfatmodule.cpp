// SPDX-License-Identifier: GPL-2.0-or-later
// Original 7-Zip DLL entrypoints reduced to the archive-only module boundary.
// clang-format off
#include "Common/MyInitGuid.h"
// clang-format on
#include "../third_party/7zip/C/7zVersion.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/ICoder.h"
#include "Windows/PropVariant.h"

Z7_DEFINE_GUID(CLSID_CArchiveHandler, k_7zip_GUID_Data1, k_7zip_GUID_Data2,
               k_7zip_GUID_Data3_Common, 0x10, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00);

STDAPI CreateArchiver(const GUID *, const GUID *, void **);
STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **object) {
    return CreateArchiver(clsid, iid, object);
}
// Copied from upstream Compress/CodecExports.cpp::GetModuleProp.
STDAPI GetModuleProp(PROPID propID, PROPVARIANT *value) {
    ::VariantClear((VARIANTARG *)value);
    switch (propID) {
    case NModulePropID::kInterfaceType:
        NWindows::NCOM::PropVarEm_Set_UInt32(
            value, NModuleInterfaceType::k_IUnknown_VirtDestructor_ThisModule);
        break;
    case NModulePropID::kVersion:
        NWindows::NCOM::PropVarEm_Set_UInt32(value, (MY_VER_MAJOR << 16) | MY_VER_MINOR);
        break;
    }
    return S_OK;
}
