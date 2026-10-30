// IsoHandler.h

#ifndef ZIP7_INC_ISO_HANDLER_H
#define ZIP7_INC_ISO_HANDLER_H

#include "Common/MyCom.h"

#include "7zip/Archive/IArchive.h"

#include "formats/iso/IsoIn.h"

namespace NArchive {
namespace NIso {

Z7_CLASS_IMP_CHandler_IInArchive_2(IInArchiveGetStream,
                                   ISetProperties) CMyComPtr<IInStream> _stream;
CInArchive _archive;
};

}
}

#endif
