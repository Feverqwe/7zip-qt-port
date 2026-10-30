// Test-only module advertising the other upstream IUnknown ABI.
#include "7zip/ICoder.h"

STDAPI GetModuleProp(PROPID propID, PROPVARIANT *value) {
    *value = PROPVARIANT{};
    if (propID == NModulePropID::kInterfaceType) {
        value->vt = VT_UI4;
        value->ulVal = NModuleInterfaceType::k_IUnknown_VirtDestructor_ThisModule ^ 1;
    }
    return S_OK;
}
