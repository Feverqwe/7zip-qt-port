#pragma once

#include "Common/MyString.h"
#include "Common/MyWindows.h"
#include <functional>
#include <memory>
class CProgressSync;

// QThread cannot own the QWidget-bearing CProgressThreadVirt base.
// CProgressSync remains the sole owner of progress and stop state.
class QtVolumeProgress final {
    struct State;
    std::unique_ptr<State> state_;

  public:
    QtVolumeProgress();
    ~QtVolumeProgress();
    void attach(CProgressSync &sync);
    void cancel();
    void Set_NumBytesTotal(UInt64 value);
    void Set_NumFilesCur(UInt64 value);
    void Set_FilePath(const UString &path);
    HRESULT Set_NumBytesCur(UInt64 value);
    std::function<void(UInt64)> advanced;
};
