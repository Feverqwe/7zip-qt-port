// Adapted from 7-Zip CPP/7zip/UI/Common/OpenArchive.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Error-property helper only; full archive opening remains mandatory P0-03/04.
#ifndef ZIP7_INC_OPEN_ARCHIVE_H
#define ZIP7_INC_OPEN_ARCHIVE_H

#include "Windows/PropVariant.h"

UInt32 GetOpenArcErrorFlags(const NWindows::NCOM::CPropVariant &prop, bool *isDefinedProp = NULL);

#endif
