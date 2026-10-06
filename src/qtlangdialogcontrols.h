#pragma once
#include "Common/MyTypes.h"

// Native GetDlgItem/SetWindowText services for the original language mapping.
class IQtLangDialogControls {
  public:
    virtual ~IQtLangDialogControls() = default;
    virtual bool HasItem(UInt32 controlID) const = 0;
    virtual void SetItemText(UInt32 controlID, const wchar_t *text) = 0;
};

class IQtLangWindowText {
  public:
    virtual ~IQtLangWindowText() = default;
    virtual void SetText(const wchar_t *text) = 0;
};
