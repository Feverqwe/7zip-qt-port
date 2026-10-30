#pragma once

#include "Common/MyString.h"
#include <functional>

class CThreadSplit;
class CThreadCombine;

// CApp owns the source workflow. Qt launches the joined asynchronous worker
// and presents its progress; plans contain the original native worker fields.
class IQtVolumeServices {
  public:
    virtual ~IQtVolumeServices() = default;
    virtual void StartSplit(const CThreadSplit &plan, const UString &title,
                            std::function<void()> completion) = 0;
    virtual void StartCombine(const CThreadCombine &plan, const UString &title,
                              std::function<void()> completion) = 0;
};
