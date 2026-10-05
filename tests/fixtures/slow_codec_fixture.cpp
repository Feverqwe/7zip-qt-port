// Test-only module: delay the original LoadCodecs compatibility query.
#include "7zip/ICoder.h"
#include <QFile>
#include <QThread>

namespace {
void Mark(const char *name) {
    QFile file(qEnvironmentVariable(name));
    if (file.open(QIODevice::WriteOnly))
        file.write("observed");
}
} // namespace

STDAPI GetModuleProp(PROPID propID, PROPVARIANT *value) {
    *value = PROPVARIANT{};
    if (propID == NModulePropID::kInterfaceType) {
        Mark("SEVENZIP_SLOW_CODEC_ENTERED");
        QThread::msleep(qEnvironmentVariableIntValue("SEVENZIP_SLOW_CODEC_DELAY"));
        Mark("SEVENZIP_SLOW_CODEC_FINISHED");
        value->vt = VT_UI4;
        value->ulVal = NModuleInterfaceType::k_IUnknown_VirtDestructor_ThisModule;
    }
    return S_OK;
}
