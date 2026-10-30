#include "7zip/Common/CreateCoder.h"

// Storage for the original UI/Agent/Agent.cpp external-codec ABI fallback.
// The standalone source-handler library also serves consumers without Agent.
// Keep it empty: handlers retain their own immutable per-instance codec lease.
#ifdef Z7_EXTERNAL_CODECS
extern CExternalCodecs g_ExternalCodecs;
CExternalCodecs g_ExternalCodecs;
#endif
