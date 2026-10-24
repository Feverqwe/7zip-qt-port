// ComTry.h

#ifndef ZIP7_INC_COM_TRY_H
#define ZIP7_INC_COM_TRY_H

#include "Common/MyException.h"
#include "Common/MyWindows.h"

#define COM_TRY_BEGIN try {
#define COM_TRY_END                                                                                \
    }                                                                                              \
    catch (const CSystemException &e) {                                                            \
        return e.ErrorCode;                                                                        \
    }                                                                                              \
    catch (...) {                                                                                  \
        return E_OUTOFMEMORY;                                                                      \
    }

#endif
