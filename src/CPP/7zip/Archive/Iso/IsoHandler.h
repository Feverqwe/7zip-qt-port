// IsoHandler.h

#ifndef ZIP7_INC_ISO_HANDLER_H
#define ZIP7_INC_ISO_HANDLER_H

#include "../../../Common/MyCom.h"

#include "../IArchive.h"

#include "IsoIn.h"

namespace NArchive {
namespace NIso {

#ifdef SEVENZIP_LINUX_ISO
Z7_CLASS_IMP_CHandler_IInArchive_2(IInArchiveGetStream, ISetProperties)
#else
Z7_CLASS_IMP_CHandler_IInArchive_1(IInArchiveGetStream)
#endif
    CMyComPtr<IInStream> _stream;
CInArchive _archive;
};

}
}

#endif
