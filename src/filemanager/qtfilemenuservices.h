#pragma once
#include "Common/MyString.h"

// Menu presentation and platform services needed by the original CFileMenu.
// The application command owner supplies source items and observes enabled IDs;
// widgets do not implement a second copy of the source capability rules.
struct QtFileMenuItem {
    unsigned wID = 0;
    UString StringValue;
    bool separator = false;
    bool IsSeparator() const { return separator; }
};

class IQtFileMenuServices {
  public:
    virtual ~IQtFileMenuServices() = default;
    virtual void ReadRegDiff(UString &path) const = 0;
    virtual void ReadReg_VerCtrlPath(UString &path) const = 0;
    virtual bool IsDialogSizeOK(unsigned width, unsigned height) const = 0;
    virtual bool GetItem(unsigned index, QtFileMenuItem &item) const = 0;
    virtual void CopyPopMenu_IfRequired(QtFileMenuItem &item) = 0;
    virtual bool InsertItem(unsigned position, const QtFileMenuItem &item) = 0;
    virtual void EnableItem(unsigned position, bool enabled) = 0;
    virtual bool AppendItem(unsigned id, const UString &text) = 0;
    virtual void RemoveAllItemsFrom(unsigned position) = 0;
};
