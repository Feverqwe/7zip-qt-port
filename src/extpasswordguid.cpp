// Password callback GUID definitions used only by the ext format module.
// clang-format off
#include "Common/MyWindows.h"
// IUnknown is initialized by the shared module entrypoint; initialize only the
// password interfaces here, using upstream's INITGUID protocol.
#define INITGUID
#include "7zip/IPassword.h"
// clang-format on
